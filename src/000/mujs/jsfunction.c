#include "jsi.h"
#include "jsparse.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static void jsB_Function(mjs_State *J)
{
	int i, top = mjs_gettop(J);
	mjs_Buffer *sb = NULL;
	const char *body;
	mjs_Ast *parse;
	mjs_Function *fun;

	if (mjs_try(J)) {
		mjs_free(J, sb);
		jsP_freeparse(J);
		mjs_throw(J);
	}

	/* p1, p2, ..., pn */
	if (top > 2) {
		for (i = 1; i < top - 1; ++i) {
			if (i > 1)
				mjs_putc(J, &sb, ',');
			mjs_puts(J, &sb, mjs_tostring(J, i));
		}
		mjs_putc(J, &sb, ')');
		mjs_putc(J, &sb, 0);
	}

	/* body */
	body = mjs_isdefined(J, top - 1) ? mjs_tostring(J, top - 1) : "";

	parse = jsP_parsefunction(J, "[string]", sb ? sb->s : NULL, body);
	fun = jsC_compilefunction(J, parse);

	mjs_endtry(J);
	mjs_free(J, sb);
	jsP_freeparse(J);

	mjs_newfunction(J, fun, J->GE);
}

static void jsB_Function_prototype(mjs_State *J)
{
	mjs_pushundefined(J);
}

static void Fp_toString(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	mjs_Buffer *sb = NULL;
	int i;

	if (!mjs_iscallable(J, 0))
		mjs_typeerror(J, "not a function");

	if (self->type == JS_CFUNCTION || self->type == JS_CSCRIPT || self->type == JS_CEVAL) {
		mjs_Function *F = self->u.f.function;

		if (mjs_try(J)) {
			mjs_free(J, sb);
			mjs_throw(J);
		}

		mjs_puts(J, &sb, "function ");
		mjs_puts(J, &sb, F->name);
		mjs_putc(J, &sb, '(');
		for (i = 0; i < F->numparams; ++i) {
			if (i > 0) mjs_putc(J, &sb, ',');
			mjs_puts(J, &sb, F->vartab[i]);
		}
		mjs_puts(J, &sb, ") { [byte code] }");
		mjs_putc(J, &sb, 0);

		mjs_pushstring(J, sb->s);
		mjs_endtry(J);
		mjs_free(J, sb);
	} else if (self->type == JS_CCFUNCTION) {
		if (mjs_try(J)) {
			mjs_free(J, sb);
			mjs_throw(J);
		}

		mjs_puts(J, &sb, "function ");
		mjs_puts(J, &sb, self->u.c.name);
		mjs_puts(J, &sb, "() { [native code] }");
		mjs_putc(J, &sb, 0);

		mjs_pushstring(J, sb->s);
		mjs_endtry(J);
		mjs_free(J, sb);
	} else {
		mjs_pushliteral(J, "function () { }");
	}
}

static void Fp_apply(mjs_State *J)
{
	int i, n;

	if (!mjs_iscallable(J, 0))
		mjs_typeerror(J, "not a function");

	mjs_copy(J, 0);
	mjs_copy(J, 1);

	if (mjs_isnull(J, 2) || mjs_isundefined(J, 2)) {
		n = 0;
	} else {
		n = mjs_getlength(J, 2);
		for (i = 0; i < n; ++i)
			mjs_getindex(J, 2, i);
	}

	mjs_call(J, n);
}

static void Fp_call(mjs_State *J)
{
	int i, top = mjs_gettop(J);

	if (!mjs_iscallable(J, 0))
		mjs_typeerror(J, "not a function");

	for (i = 0; i < top; ++i)
		mjs_copy(J, i);

	mjs_call(J, top - 2);
}

static void callbound(mjs_State *J)
{
	int top = mjs_gettop(J);
	int i, fun, args, n;

	fun = mjs_gettop(J);
	mjs_currentfunction(J);
	mjs_getproperty(J, fun, "__TargetFunction__");
	mjs_getproperty(J, fun, "__BoundThis__");

	args = mjs_gettop(J);
	mjs_getproperty(J, fun, "__BoundArguments__");
	n = mjs_getlength(J, args);
	for (i = 0; i < n; ++i)
		mjs_getindex(J, args, i);
	mjs_remove(J, args);

	for (i = 1; i < top; ++i)
		mjs_copy(J, i);

	mjs_call(J, n + top - 1);
}

static void constructbound(mjs_State *J)
{
	int top = mjs_gettop(J);
	int i, fun, args, n;

	fun = mjs_gettop(J);
	mjs_currentfunction(J);
	mjs_getproperty(J, fun, "__TargetFunction__");

	args = mjs_gettop(J);
	mjs_getproperty(J, fun, "__BoundArguments__");
	n = mjs_getlength(J, args);
	for (i = 0; i < n; ++i)
		mjs_getindex(J, args, i);
	mjs_remove(J, args);

	for (i = 1; i < top; ++i)
		mjs_copy(J, i);

	mjs_construct(J, n + top - 1);
}

static void Fp_bind(mjs_State *J)
{
	int i, top = mjs_gettop(J);
	int n;

	if (!mjs_iscallable(J, 0))
		mjs_typeerror(J, "not a function");

	n = mjs_getlength(J, 0);
	if (n > top - 2)
		n -= top - 2;
	else
		n = 0;

	/* Reuse target function's prototype for HasInstance check. */
	mjs_getproperty(J, 0, "prototype");
	mjs_newcconstructor(J, callbound, constructbound, "[bind]", n);

	/* target function */
	mjs_copy(J, 0);
	mjs_defproperty(J, -2, "__TargetFunction__", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	/* bound this */
	mjs_copy(J, 1);
	mjs_defproperty(J, -2, "__BoundThis__", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	/* bound arguments */
	mjs_newarray(J);
	for (i = 2; i < top; ++i) {
		mjs_copy(J, i);
		mjs_setindex(J, -2, i - 2);
	}
	mjs_defproperty(J, -2, "__BoundArguments__", JS_READONLY | JS_DONTENUM | JS_DONTCONF);
}

void jsB_initfunction(mjs_State *J)
{
	J->Function_prototype->u.c.name = "Function.prototype";
	J->Function_prototype->u.c.function = jsB_Function_prototype;
	J->Function_prototype->u.c.constructor = NULL;
	J->Function_prototype->u.c.length = 0;

	mjs_pushobject(J, J->Function_prototype);
	{
		jsB_propf(J, "Function.prototype.toString", Fp_toString, 2);
		jsB_propf(J, "Function.prototype.apply", Fp_apply, 2);
		jsB_propf(J, "Function.prototype.call", Fp_call, 1);
		jsB_propf(J, "Function.prototype.bind", Fp_bind, 1);
	}
	mjs_newcconstructor(J, jsB_Function, jsB_Function, "Function", 1);
	mjs_defglobal(J, "Function", JS_DONTENUM);
}
