#include "jsi.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsrun.h"

#include "regexp.h"

static void jsG_markobject(mjs_State *J, int mark, mjs_Object *obj);

static void jsG_freeenvironment(mjs_State *J, mjs_Environment *env)
{
	mjs_free(J, env);
}

static void jsG_freefunction(mjs_State *J, mjs_Function *fun)
{
	mjs_free(J, fun->funtab);
	mjs_free(J, fun->numtab);
	mjs_free(J, fun->strtab);
	mjs_free(J, fun->vartab);
	mjs_free(J, fun->code);
	mjs_free(J, fun);
}

static void jsG_freeproperty(mjs_State *J, mjs_Property *node)
{
	if (node->left->level) jsG_freeproperty(J, node->left);
	if (node->right->level) jsG_freeproperty(J, node->right);
	mjs_free(J, node);
}

static void jsG_freeiterator(mjs_State *J, mjs_Iterator *node)
{
	while (node) {
		mjs_Iterator *next = node->next;
		mjs_free(J, node);
		node = next;
	}
}

static void jsG_freeobject(mjs_State *J, mjs_Object *obj)
{
	if (obj->properties->level)
		jsG_freeproperty(J, obj->properties);
	if (obj->type == JS_CREGEXP) {
		mjs_free(J, obj->u.r.source);
		mjs_regfreex(J->alloc, J->actx, obj->u.r.prog);
	}
	if (obj->type == JS_CITERATOR)
		jsG_freeiterator(J, obj->u.iter.head);
	if (obj->type == JS_CUSERDATA && obj->u.user.finalize)
		obj->u.user.finalize(J, obj->u.user.data);
	mjs_free(J, obj);
}

static void jsG_markfunction(mjs_State *J, int mark, mjs_Function *fun)
{
	int i;
	fun->gcmark = mark;
	for (i = 0; i < fun->funlen; ++i)
		if (fun->funtab[i]->gcmark != mark)
			jsG_markfunction(J, mark, fun->funtab[i]);
}

static void jsG_markenvironment(mjs_State *J, int mark, mjs_Environment *env)
{
	do {
		env->gcmark = mark;
		if (env->variables->gcmark != mark)
			jsG_markobject(J, mark, env->variables);
		env = env->outer;
	} while (env && env->gcmark != mark);
}

static void jsG_markproperty(mjs_State *J, int mark, mjs_Property *node)
{
	if (node->left->level) jsG_markproperty(J, mark, node->left);
	if (node->right->level) jsG_markproperty(J, mark, node->right);

	if (node->value.type == JS_TMEMSTR && node->value.u.memstr->gcmark != mark)
		node->value.u.memstr->gcmark = mark;
	if (node->value.type == JS_TOBJECT && node->value.u.object->gcmark != mark)
		jsG_markobject(J, mark, node->value.u.object);
	if (node->getter && node->getter->gcmark != mark)
		jsG_markobject(J, mark, node->getter);
	if (node->setter && node->setter->gcmark != mark)
		jsG_markobject(J, mark, node->setter);
}

static void jsG_markobject(mjs_State *J, int mark, mjs_Object *obj)
{
	obj->gcmark = mark;
	if (obj->properties->level)
		jsG_markproperty(J, mark, obj->properties);
	if (obj->prototype && obj->prototype->gcmark != mark)
		jsG_markobject(J, mark, obj->prototype);
	if (obj->type == JS_CITERATOR) {
		jsG_markobject(J, mark, obj->u.iter.target);
	}
	if (obj->type == JS_CFUNCTION || obj->type == JS_CSCRIPT || obj->type == JS_CEVAL) {
		if (obj->u.f.scope && obj->u.f.scope->gcmark != mark)
			jsG_markenvironment(J, mark, obj->u.f.scope);
		if (obj->u.f.function && obj->u.f.function->gcmark != mark)
			jsG_markfunction(J, mark, obj->u.f.function);
	}
}

static void jsG_markstack(mjs_State *J, int mark)
{
	mjs_Value *v = J->stack;
	int n = J->top;
	while (n--) {
		if (v->type == JS_TMEMSTR && v->u.memstr->gcmark != mark)
			v->u.memstr->gcmark = mark;
		if (v->type == JS_TOBJECT && v->u.object->gcmark != mark)
			jsG_markobject(J, mark, v->u.object);
		++v;
	}
}

void mjs_gc(mjs_State *J, int report)
{
	mjs_Function *fun, *nextfun, **prevnextfun;
	mjs_Object *obj, *nextobj, **prevnextobj;
	mjs_String *str, *nextstr, **prevnextstr;
	mjs_Environment *env, *nextenv, **prevnextenv;
	int nenv = 0, nfun = 0, nobj = 0, nstr = 0;
	int genv = 0, gfun = 0, gobj = 0, gstr = 0;
	int mark;
	int i;

	if (J->gcpause) {
		if (report)
			mjs_report(J, "garbage collector is paused");
		return;
	}

	J->gccounter = 0;

	mark = J->gcmark = J->gcmark == 1 ? 2 : 1;

	jsG_markobject(J, mark, J->Object_prototype);
	jsG_markobject(J, mark, J->Array_prototype);
	jsG_markobject(J, mark, J->Function_prototype);
	jsG_markobject(J, mark, J->Boolean_prototype);
	jsG_markobject(J, mark, J->Number_prototype);
	jsG_markobject(J, mark, J->String_prototype);
	jsG_markobject(J, mark, J->RegExp_prototype);
	jsG_markobject(J, mark, J->Date_prototype);

	jsG_markobject(J, mark, J->Error_prototype);
	jsG_markobject(J, mark, J->EvalError_prototype);
	jsG_markobject(J, mark, J->RangeError_prototype);
	jsG_markobject(J, mark, J->ReferenceError_prototype);
	jsG_markobject(J, mark, J->SyntaxError_prototype);
	jsG_markobject(J, mark, J->TypeError_prototype);
	jsG_markobject(J, mark, J->URIError_prototype);

	jsG_markobject(J, mark, J->R);
	jsG_markobject(J, mark, J->G);

	jsG_markstack(J, mark);

	jsG_markenvironment(J, mark, J->E);
	jsG_markenvironment(J, mark, J->GE);
	for (i = 0; i < J->envtop; ++i)
		jsG_markenvironment(J, mark, J->envstack[i]);

	prevnextenv = &J->gcenv;
	for (env = J->gcenv; env; env = nextenv) {
		nextenv = env->gcnext;
		if (env->gcmark != mark) {
			*prevnextenv = nextenv;
			jsG_freeenvironment(J, env);
			++genv;
		} else {
			prevnextenv = &env->gcnext;
		}
		++nenv;
	}

	prevnextfun = &J->gcfun;
	for (fun = J->gcfun; fun; fun = nextfun) {
		nextfun = fun->gcnext;
		if (fun->gcmark != mark) {
			*prevnextfun = nextfun;
			jsG_freefunction(J, fun);
			++gfun;
		} else {
			prevnextfun = &fun->gcnext;
		}
		++nfun;
	}

	prevnextobj = &J->gcobj;
	for (obj = J->gcobj; obj; obj = nextobj) {
		nextobj = obj->gcnext;
		if (obj->gcmark != mark) {
			*prevnextobj = nextobj;
			jsG_freeobject(J, obj);
			++gobj;
		} else {
			prevnextobj = &obj->gcnext;
		}
		++nobj;
	}

	prevnextstr = &J->gcstr;
	for (str = J->gcstr; str; str = nextstr) {
		nextstr = str->gcnext;
		if (str->gcmark != mark) {
			*prevnextstr = nextstr;
			mjs_free(J, str);
			++gstr;
		} else {
			prevnextstr = &str->gcnext;
		}
		++nstr;
	}

	if (report) {
		char buf[256];
		snprintf(buf, sizeof buf, "garbage collected: %d/%d envs, %d/%d funs, %d/%d objs, %d/%d strs",
			genv, nenv, gfun, nfun, gobj, nobj, gstr, nstr);
		mjs_report(J, buf);
	}
}

void mjs_freestate(mjs_State *J)
{
	mjs_Function *fun, *nextfun;
	mjs_Object *obj, *nextobj;
	mjs_Environment *env, *nextenv;
	mjs_String *str, *nextstr;

	if (!J)
		return;

	for (env = J->gcenv; env; env = nextenv)
		nextenv = env->gcnext, jsG_freeenvironment(J, env);
	for (fun = J->gcfun; fun; fun = nextfun)
		nextfun = fun->gcnext, jsG_freefunction(J, fun);
	for (obj = J->gcobj; obj; obj = nextobj)
		nextobj = obj->gcnext, jsG_freeobject(J, obj);
	for (str = J->gcstr; str; str = nextstr)
		nextstr = str->gcnext, mjs_free(J, str);

	jsS_freestrings(J);

	mjs_free(J, J->lexbuf.text);
	J->alloc(J->actx, J->stack, 0);
	J->alloc(J->actx, J, 0);
}
