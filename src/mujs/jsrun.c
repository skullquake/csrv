#include "jsi.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsrun.h"

#include "utf.h"

static void jsR_run(mjs_State *J, mjs_Function *F);

/* Push values on stack */

#define STACK (J->stack)
#define TOP (J->top)
#define BOT (J->bot)

static void mjs_stackoverflow(mjs_State *J)
{
	STACK[TOP].type = JS_TLITSTR;
	STACK[TOP].u.litstr = "stack overflow";
	++TOP;
	mjs_throw(J);
}

static void mjs_outofmemory(mjs_State *J)
{
	STACK[TOP].type = JS_TLITSTR;
	STACK[TOP].u.litstr = "out of memory";
	++TOP;
	mjs_throw(J);
}

void *mjs_malloc(mjs_State *J, int size)
{
	void *ptr = J->alloc(J->actx, NULL, size);
	if (!ptr)
		mjs_outofmemory(J);
	return ptr;
}

void *mjs_realloc(mjs_State *J, void *ptr, int size)
{
	ptr = J->alloc(J->actx, ptr, size);
	if (!ptr)
		mjs_outofmemory(J);
	return ptr;
}

char *mjs_strdup(mjs_State *J, const char *s)
{
	int n = strlen(s) + 1;
	char *p = mjs_malloc(J, n);
	memcpy(p, s, n);
	return p;
}

void mjs_free(mjs_State *J, void *ptr)
{
	J->alloc(J->actx, ptr, 0);
}

mjs_String *jsV_newmemstring(mjs_State *J, const char *s, int n)
{
	mjs_String *v = mjs_malloc(J, soffsetof(mjs_String, p) + n + 1);
	memcpy(v->p, s, n);
	v->p[n] = 0;
	v->gcmark = 0;
	v->gcnext = J->gcstr;
	J->gcstr = v;
	++J->gccounter;
	return v;
}

#define CHECKSTACK(n) if (TOP + n >= JS_STACKSIZE) mjs_stackoverflow(J)

void mjs_pushvalue(mjs_State *J, mjs_Value v)
{
	CHECKSTACK(1);
	STACK[TOP] = v;
	++TOP;
}

void mjs_pushundefined(mjs_State *J)
{
	CHECKSTACK(1);
	STACK[TOP].type = JS_TUNDEFINED;
	++TOP;
}

void mjs_pushnull(mjs_State *J)
{
	CHECKSTACK(1);
	STACK[TOP].type = JS_TNULL;
	++TOP;
}

void mjs_pushboolean(mjs_State *J, int v)
{
	CHECKSTACK(1);
	STACK[TOP].type = JS_TBOOLEAN;
	STACK[TOP].u.boolean = !!v;
	++TOP;
}

void mjs_pushnumber(mjs_State *J, double v)
{
	CHECKSTACK(1);
	STACK[TOP].type = JS_TNUMBER;
	STACK[TOP].u.number = v;
	++TOP;
}

void mjs_pushstring(mjs_State *J, const char *v)
{
	int n = strlen(v);
	CHECKSTACK(1);
	if (n <= soffsetof(mjs_Value, type)) {
		char *s = STACK[TOP].u.shrstr;
		while (n--) *s++ = *v++;
		*s = 0;
		STACK[TOP].type = JS_TSHRSTR;
	} else {
		STACK[TOP].type = JS_TMEMSTR;
		STACK[TOP].u.memstr = jsV_newmemstring(J, v, n);
	}
	++TOP;
}

void mjs_pushlstring(mjs_State *J, const char *v, int n)
{
	CHECKSTACK(1);
	if (n <= soffsetof(mjs_Value, type)) {
		char *s = STACK[TOP].u.shrstr;
		while (n--) *s++ = *v++;
		*s = 0;
		STACK[TOP].type = JS_TSHRSTR;
	} else {
		STACK[TOP].type = JS_TMEMSTR;
		STACK[TOP].u.memstr = jsV_newmemstring(J, v, n);
	}
	++TOP;
}

void mjs_pushliteral(mjs_State *J, const char *v)
{
	CHECKSTACK(1);
	STACK[TOP].type = JS_TLITSTR;
	STACK[TOP].u.litstr = v;
	++TOP;
}

void mjs_pushobject(mjs_State *J, mjs_Object *v)
{
	CHECKSTACK(1);
	STACK[TOP].type = JS_TOBJECT;
	STACK[TOP].u.object = v;
	++TOP;
}

void mjs_pushglobal(mjs_State *J)
{
	mjs_pushobject(J, J->G);
}

void mjs_currentfunction(mjs_State *J)
{
	CHECKSTACK(1);
	STACK[TOP] = STACK[BOT-1];
	++TOP;
}

/* Read values from stack */

static mjs_Value *stackidx(mjs_State *J, int idx)
{
	static mjs_Value undefined = { {0}, {0}, JS_TUNDEFINED };
	idx = idx < 0 ? TOP + idx : BOT + idx;
	if (idx < 0 || idx >= TOP)
		return &undefined;
	return STACK + idx;
}

mjs_Value *mjs_tovalue(mjs_State *J, int idx)
{
	return stackidx(J, idx);
}

int mjs_isdefined(mjs_State *J, int idx) { return stackidx(J, idx)->type != JS_TUNDEFINED; }
int mjs_isundefined(mjs_State *J, int idx) { return stackidx(J, idx)->type == JS_TUNDEFINED; }
int mjs_isnull(mjs_State *J, int idx) { return stackidx(J, idx)->type == JS_TNULL; }
int mjs_isboolean(mjs_State *J, int idx) { return stackidx(J, idx)->type == JS_TBOOLEAN; }
int mjs_isnumber(mjs_State *J, int idx) { return stackidx(J, idx)->type == JS_TNUMBER; }
int mjs_isstring(mjs_State *J, int idx) { enum mjs_Type t = stackidx(J, idx)->type; return t == JS_TSHRSTR || t == JS_TLITSTR || t == JS_TMEMSTR; }
int mjs_isprimitive(mjs_State *J, int idx) { return stackidx(J, idx)->type != JS_TOBJECT; }
int mjs_isobject(mjs_State *J, int idx) { return stackidx(J, idx)->type == JS_TOBJECT; }
int mjs_iscoercible(mjs_State *J, int idx) { mjs_Value *v = stackidx(J, idx); return v->type != JS_TUNDEFINED && v->type != JS_TNULL; }

int mjs_iscallable(mjs_State *J, int idx)
{
	mjs_Value *v = stackidx(J, idx);
	if (v->type == JS_TOBJECT)
		return v->u.object->type == JS_CFUNCTION ||
			v->u.object->type == JS_CSCRIPT ||
			v->u.object->type == JS_CEVAL ||
			v->u.object->type == JS_CCFUNCTION;
	return 0;
}

int mjs_isarray(mjs_State *J, int idx)
{
	mjs_Value *v = stackidx(J, idx);
	return v->type == JS_TOBJECT && v->u.object->type == JS_CARRAY;
}

int mjs_isregexp(mjs_State *J, int idx)
{
	mjs_Value *v = stackidx(J, idx);
	return v->type == JS_TOBJECT && v->u.object->type == JS_CREGEXP;
}

int mjs_isuserdata(mjs_State *J, int idx, const char *tag)
{
	mjs_Value *v = stackidx(J, idx);
	if (v->type == JS_TOBJECT && v->u.object->type == JS_CUSERDATA)
		return !strcmp(tag, v->u.object->u.user.tag);
	return 0;
}

int mjs_iserror(mjs_State *J, int idx)
{
	mjs_Value *v = stackidx(J, idx);
	return v->type == JS_TOBJECT && v->u.object->type == JS_CERROR;
}

const char *mjs_typeof(mjs_State *J, int idx)
{
	mjs_Value *v = stackidx(J, idx);
	switch (v->type) {
	default:
	case JS_TSHRSTR: return "string";
	case JS_TUNDEFINED: return "undefined";
	case JS_TNULL: return "object";
	case JS_TBOOLEAN: return "boolean";
	case JS_TNUMBER: return "number";
	case JS_TLITSTR: return "string";
	case JS_TMEMSTR: return "string";
	case JS_TOBJECT:
		if (v->u.object->type == JS_CFUNCTION || v->u.object->type == JS_CCFUNCTION)
			return "function";
		return "object";
	}
}

int mjs_toboolean(mjs_State *J, int idx)
{
	return jsV_toboolean(J, stackidx(J, idx));
}

double mjs_tonumber(mjs_State *J, int idx)
{
	return jsV_tonumber(J, stackidx(J, idx));
}

int mjs_tointeger(mjs_State *J, int idx)
{
	return jsV_numbertointeger(jsV_tonumber(J, stackidx(J, idx)));
}

int mjs_toint32(mjs_State *J, int idx)
{
	return jsV_numbertoint32(jsV_tonumber(J, stackidx(J, idx)));
}

unsigned int mjs_touint32(mjs_State *J, int idx)
{
	return jsV_numbertouint32(jsV_tonumber(J, stackidx(J, idx)));
}

short mjs_toint16(mjs_State *J, int idx)
{
	return jsV_numbertoint16(jsV_tonumber(J, stackidx(J, idx)));
}

unsigned short mjs_touint16(mjs_State *J, int idx)
{
	return jsV_numbertouint16(jsV_tonumber(J, stackidx(J, idx)));
}

const char *mjs_tostring(mjs_State *J, int idx)
{
	return jsV_tostring(J, stackidx(J, idx));
}

mjs_Object *mjs_toobject(mjs_State *J, int idx)
{
	return jsV_toobject(J, stackidx(J, idx));
}

void mjs_toprimitive(mjs_State *J, int idx, int hint)
{
	jsV_toprimitive(J, stackidx(J, idx), hint);
}

mjs_Regexp *mjs_toregexp(mjs_State *J, int idx)
{
	mjs_Value *v = stackidx(J, idx);
	if (v->type == JS_TOBJECT && v->u.object->type == JS_CREGEXP)
		return &v->u.object->u.r;
	mjs_typeerror(J, "not a regexp");
}

void *mjs_touserdata(mjs_State *J, int idx, const char *tag)
{
	mjs_Value *v = stackidx(J, idx);
	if (v->type == JS_TOBJECT && v->u.object->type == JS_CUSERDATA)
		if (!strcmp(tag, v->u.object->u.user.tag))
			return v->u.object->u.user.data;
	mjs_typeerror(J, "not a %s", tag);
}

static mjs_Object *jsR_tofunction(mjs_State *J, int idx)
{
	mjs_Value *v = stackidx(J, idx);
	if (v->type == JS_TUNDEFINED || v->type == JS_TNULL)
		return NULL;
	if (v->type == JS_TOBJECT)
		if (v->u.object->type == JS_CFUNCTION || v->u.object->type == JS_CCFUNCTION)
			return v->u.object;
	mjs_typeerror(J, "not a function");
}

/* Stack manipulation */

int mjs_gettop(mjs_State *J)
{
	return TOP - BOT;
}

void mjs_pop(mjs_State *J, int n)
{
	TOP -= n;
	if (TOP < BOT) {
		TOP = BOT;
		mjs_error(J, "stack underflow!");
	}
}

void mjs_remove(mjs_State *J, int idx)
{
	idx = idx < 0 ? TOP + idx : BOT + idx;
	if (idx < BOT || idx >= TOP)
		mjs_error(J, "stack error!");
	for (;idx < TOP - 1; ++idx)
		STACK[idx] = STACK[idx+1];
	--TOP;
}

void mjs_insert(mjs_State *J, int idx)
{
	mjs_error(J, "not implemented yet");
}

void mjs_replace(mjs_State* J, int idx)
{
	idx = idx < 0 ? TOP + idx : BOT + idx;
	if (idx < BOT || idx >= TOP)
		mjs_error(J, "stack error!");
	STACK[idx] = STACK[--TOP];
}

void mjs_copy(mjs_State *J, int idx)
{
	CHECKSTACK(1);
	STACK[TOP] = *stackidx(J, idx);
	++TOP;
}

void mjs_dup(mjs_State *J)
{
	CHECKSTACK(1);
	STACK[TOP] = STACK[TOP-1];
	++TOP;
}

void mjs_dup2(mjs_State *J)
{
	CHECKSTACK(2);
	STACK[TOP] = STACK[TOP-2];
	STACK[TOP+1] = STACK[TOP-1];
	TOP += 2;
}

void mjs_rot2(mjs_State *J)
{
	/* A B -> B A */
	mjs_Value tmp = STACK[TOP-1];	/* A B (B) */
	STACK[TOP-1] = STACK[TOP-2];	/* A A */
	STACK[TOP-2] = tmp;		/* B A */
}

void mjs_rot3(mjs_State *J)
{
	/* A B C -> C A B */
	mjs_Value tmp = STACK[TOP-1];	/* A B C (C) */
	STACK[TOP-1] = STACK[TOP-2];	/* A B B */
	STACK[TOP-2] = STACK[TOP-3];	/* A A B */
	STACK[TOP-3] = tmp;		/* C A B */
}

void mjs_rot4(mjs_State *J)
{
	/* A B C D -> D A B C */
	mjs_Value tmp = STACK[TOP-1];	/* A B C D (D) */
	STACK[TOP-1] = STACK[TOP-2];	/* A B C C */
	STACK[TOP-2] = STACK[TOP-3];	/* A B B C */
	STACK[TOP-3] = STACK[TOP-4];	/* A A B C */
	STACK[TOP-4] = tmp;		/* D A B C */
}

void mjs_rot2pop1(mjs_State *J)
{
	/* A B -> B */
	STACK[TOP-2] = STACK[TOP-1];
	--TOP;
}

void mjs_rot3pop2(mjs_State *J)
{
	/* A B C -> C */
	STACK[TOP-3] = STACK[TOP-1];
	TOP -= 2;
}

void mjs_rot(mjs_State *J, int n)
{
	int i;
	mjs_Value tmp = STACK[TOP-1];
	for (i = 1; i < n; ++i)
		STACK[TOP-i] = STACK[TOP-i-1];
	STACK[TOP-i] = tmp;
}

/* Property access that takes care of attributes and getters/setters */

int mjs_isarrayindex(mjs_State *J, const char *p, int *idx)
{
	int n = 0;

	/* check for empty string */
	if (p[0] == 0)
		return 0;

	/* check for '0' and integers with leading zero */
	if (p[0] == '0')
		return (p[1] == 0) ? *idx = 0, 1 : 0;

	while (*p) {
		int c = *p++;
		if (c >= '0' && c <= '9') {
			if (n >= INT_MAX / 10)
				return 0;
			n = n * 10 + (c - '0');
		} else {
			return 0;
		}
	}
	return *idx = n, 1;
}

static void mjs_pushrune(mjs_State *J, Rune rune)
{
	char buf[UTFmax + 1];
	if (rune > 0) {
		buf[runetochar(buf, &rune)] = 0;
		mjs_pushstring(J, buf);
	} else {
		mjs_pushundefined(J);
	}
}

static int jsR_hasproperty(mjs_State *J, mjs_Object *obj, const char *name)
{
	mjs_Property *ref;
	int k;

	if (obj->type == JS_CARRAY) {
		if (!strcmp(name, "length")) {
			mjs_pushnumber(J, obj->u.a.length);
			return 1;
		}
	}

	else if (obj->type == JS_CSTRING) {
		if (!strcmp(name, "length")) {
			mjs_pushnumber(J, obj->u.s.length);
			return 1;
		}
		if (mjs_isarrayindex(J, name, &k)) {
			if (k >= 0 && k < obj->u.s.length) {
				mjs_pushrune(J, mjs_runeat(J, obj->u.s.string, k));
				return 1;
			}
		}
	}

	else if (obj->type == JS_CREGEXP) {
		if (!strcmp(name, "source")) {
			mjs_pushliteral(J, obj->u.r.source);
			return 1;
		}
		if (!strcmp(name, "global")) {
			mjs_pushboolean(J, obj->u.r.flags & JS_REGEXP_G);
			return 1;
		}
		if (!strcmp(name, "ignoreCase")) {
			mjs_pushboolean(J, obj->u.r.flags & JS_REGEXP_I);
			return 1;
		}
		if (!strcmp(name, "multiline")) {
			mjs_pushboolean(J, obj->u.r.flags & JS_REGEXP_M);
			return 1;
		}
		if (!strcmp(name, "lastIndex")) {
			mjs_pushnumber(J, obj->u.r.last);
			return 1;
		}
	}

	else if (obj->type == JS_CUSERDATA) {
		if (obj->u.user.has && obj->u.user.has(J, obj->u.user.data, name))
			return 1;
	}

	ref = jsV_getproperty(J, obj, name);
	if (ref) {
		if (ref->getter) {
			mjs_pushobject(J, ref->getter);
			mjs_pushobject(J, obj);
			mjs_call(J, 0);
		} else {
			mjs_pushvalue(J, ref->value);
		}
		return 1;
	}

	return 0;
}

static void jsR_getproperty(mjs_State *J, mjs_Object *obj, const char *name)
{
	if (!jsR_hasproperty(J, obj, name))
		mjs_pushundefined(J);
}

static void jsR_setproperty(mjs_State *J, mjs_Object *obj, const char *name)
{
	mjs_Value *value = stackidx(J, -1);
	mjs_Property *ref;
	int k;
	int own;

	if (obj->type == JS_CARRAY) {
		if (!strcmp(name, "length")) {
			double rawlen = jsV_tonumber(J, value);
			int newlen = jsV_numbertointeger(rawlen);
			if (newlen != rawlen || newlen < 0)
				mjs_rangeerror(J, "invalid array length");
			jsV_resizearray(J, obj, newlen);
			return;
		}
		if (mjs_isarrayindex(J, name, &k))
			if (k >= obj->u.a.length)
				obj->u.a.length = k + 1;
	}

	else if (obj->type == JS_CSTRING) {
		if (!strcmp(name, "length"))
			goto readonly;
		if (mjs_isarrayindex(J, name, &k))
			if (k >= 0 && k < obj->u.s.length)
				goto readonly;
	}

	else if (obj->type == JS_CREGEXP) {
		if (!strcmp(name, "source")) goto readonly;
		if (!strcmp(name, "global")) goto readonly;
		if (!strcmp(name, "ignoreCase")) goto readonly;
		if (!strcmp(name, "multiline")) goto readonly;
		if (!strcmp(name, "lastIndex")) {
			obj->u.r.last = jsV_tointeger(J, value);
			return;
		}
	}

	else if (obj->type == JS_CUSERDATA) {
		if (obj->u.user.put && obj->u.user.put(J, obj->u.user.data, name))
			return;
	}

	/* First try to find a setter in prototype chain */
	ref = jsV_getpropertyx(J, obj, name, &own);
	if (ref) {
		if (ref->setter) {
			mjs_pushobject(J, ref->setter);
			mjs_pushobject(J, obj);
			mjs_pushvalue(J, *value);
			mjs_call(J, 1);
			mjs_pop(J, 1);
			return;
		} else {
			if (J->strict)
				if (ref->getter)
					mjs_typeerror(J, "setting property '%s' that only has a getter", name);
		}
	}

	/* Property not found on this object, so create one */
	if (!ref || !own)
		ref = jsV_setproperty(J, obj, name);

	if (ref) {
		if (!(ref->atts & JS_READONLY))
			ref->value = *value;
		else
			goto readonly;
	}

	return;

readonly:
	if (J->strict)
		mjs_typeerror(J, "'%s' is read-only", name);
}

static void jsR_defproperty(mjs_State *J, mjs_Object *obj, const char *name,
	int atts, mjs_Value *value, mjs_Object *getter, mjs_Object *setter)
{
	mjs_Property *ref;
	int k;

	if (obj->type == JS_CARRAY) {
		if (!strcmp(name, "length"))
			goto readonly;
	}

	else if (obj->type == JS_CSTRING) {
		if (!strcmp(name, "length"))
			goto readonly;
		if (mjs_isarrayindex(J, name, &k))
			if (k >= 0 && k < obj->u.s.length)
				goto readonly;
	}

	else if (obj->type == JS_CREGEXP) {
		if (!strcmp(name, "source")) goto readonly;
		if (!strcmp(name, "global")) goto readonly;
		if (!strcmp(name, "ignoreCase")) goto readonly;
		if (!strcmp(name, "multiline")) goto readonly;
		if (!strcmp(name, "lastIndex")) goto readonly;
	}

	else if (obj->type == JS_CUSERDATA) {
		if (obj->u.user.put && obj->u.user.put(J, obj->u.user.data, name))
			return;
	}

	ref = jsV_setproperty(J, obj, name);
	if (ref) {
		if (value) {
			if (!(ref->atts & JS_READONLY))
				ref->value = *value;
			else if (J->strict)
				mjs_typeerror(J, "'%s' is read-only", name);
		}
		if (getter) {
			if (!(ref->atts & JS_DONTCONF))
				ref->getter = getter;
			else if (J->strict)
				mjs_typeerror(J, "'%s' is non-configurable", name);
		}
		if (setter) {
			if (!(ref->atts & JS_DONTCONF))
				ref->setter = setter;
			else if (J->strict)
				mjs_typeerror(J, "'%s' is non-configurable", name);
		}
		ref->atts |= atts;
	}

	return;

readonly:
	if (J->strict)
		mjs_typeerror(J, "'%s' is read-only or non-configurable", name);
}

static int jsR_delproperty(mjs_State *J, mjs_Object *obj, const char *name)
{
	mjs_Property *ref;
	int k;

	if (obj->type == JS_CARRAY) {
		if (!strcmp(name, "length"))
			goto dontconf;
	}

	else if (obj->type == JS_CSTRING) {
		if (!strcmp(name, "length"))
			goto dontconf;
		if (mjs_isarrayindex(J, name, &k))
			if (k >= 0 && k < obj->u.s.length)
				goto dontconf;
	}

	else if (obj->type == JS_CREGEXP) {
		if (!strcmp(name, "source")) goto dontconf;
		if (!strcmp(name, "global")) goto dontconf;
		if (!strcmp(name, "ignoreCase")) goto dontconf;
		if (!strcmp(name, "multiline")) goto dontconf;
		if (!strcmp(name, "lastIndex")) goto dontconf;
	}

	else if (obj->type == JS_CUSERDATA) {
		if (obj->u.user.delete && obj->u.user.delete(J, obj->u.user.data, name))
			return 1;
	}

	ref = jsV_getownproperty(J, obj, name);
	if (ref) {
		if (ref->atts & JS_DONTCONF)
			goto dontconf;
		jsV_delproperty(J, obj, name);
	}
	return 1;

dontconf:
	if (J->strict)
		mjs_typeerror(J, "'%s' is non-configurable", name);
	return 0;
}

/* Registry, global and object property accessors */

const char *mjs_ref(mjs_State *J)
{
	mjs_Value *v = stackidx(J, -1);
	const char *s;
	char buf[32];
	switch (v->type) {
	case JS_TUNDEFINED: s = "_Undefined"; break;
	case JS_TNULL: s = "_Null"; break;
	case JS_TBOOLEAN:
		s = v->u.boolean ? "_True" : "_False";
		break;
	case JS_TOBJECT:
		sprintf(buf, "%p", (void*)v->u.object);
		s = mjs_intern(J, buf);
		break;
	default:
		sprintf(buf, "%d", J->nextref++);
		s = mjs_intern(J, buf);
		break;
	}
	mjs_setregistry(J, s);
	return s;
}

void mjs_unref(mjs_State *J, const char *ref)
{
	mjs_delregistry(J, ref);
}

void mjs_getregistry(mjs_State *J, const char *name)
{
	jsR_getproperty(J, J->R, name);
}

void mjs_setregistry(mjs_State *J, const char *name)
{
	jsR_setproperty(J, J->R, name);
	mjs_pop(J, 1);
}

void mjs_delregistry(mjs_State *J, const char *name)
{
	jsR_delproperty(J, J->R, name);
}

void mjs_getglobal(mjs_State *J, const char *name)
{
	jsR_getproperty(J, J->G, name);
}

void mjs_setglobal(mjs_State *J, const char *name)
{
	jsR_setproperty(J, J->G, name);
	mjs_pop(J, 1);
}

void mjs_defglobal(mjs_State *J, const char *name, int atts)
{
	jsR_defproperty(J, J->G, name, atts, stackidx(J, -1), NULL, NULL);
	mjs_pop(J, 1);
}

void mjs_delglobal(mjs_State *J, const char *name)
{
	jsR_delproperty(J, J->G, name);
}

void mjs_getproperty(mjs_State *J, int idx, const char *name)
{
	jsR_getproperty(J, mjs_toobject(J, idx), name);
}

void mjs_setproperty(mjs_State *J, int idx, const char *name)
{
	jsR_setproperty(J, mjs_toobject(J, idx), name);
	mjs_pop(J, 1);
}

void mjs_defproperty(mjs_State *J, int idx, const char *name, int atts)
{
	jsR_defproperty(J, mjs_toobject(J, idx), name, atts, stackidx(J, -1), NULL, NULL);
	mjs_pop(J, 1);
}

void mjs_delproperty(mjs_State *J, int idx, const char *name)
{
	jsR_delproperty(J, mjs_toobject(J, idx), name);
}

void mjs_defaccessor(mjs_State *J, int idx, const char *name, int atts)
{
	jsR_defproperty(J, mjs_toobject(J, idx), name, atts, NULL, jsR_tofunction(J, -2), jsR_tofunction(J, -1));
	mjs_pop(J, 2);
}

int mjs_hasproperty(mjs_State *J, int idx, const char *name)
{
	return jsR_hasproperty(J, mjs_toobject(J, idx), name);
}

/* Iterator */

void mjs_pushiterator(mjs_State *J, int idx, int own)
{
	mjs_pushobject(J, jsV_newiterator(J, mjs_toobject(J, idx), own));
}

const char *mjs_nextiterator(mjs_State *J, int idx)
{
	return jsV_nextiterator(J, mjs_toobject(J, idx));
}

/* Environment records */

mjs_Environment *jsR_newenvironment(mjs_State *J, mjs_Object *vars, mjs_Environment *outer)
{
	mjs_Environment *E = mjs_malloc(J, sizeof *E);
	E->gcmark = 0;
	E->gcnext = J->gcenv;
	J->gcenv = E;
	++J->gccounter;

	E->outer = outer;
	E->variables = vars;
	return E;
}

static void mjs_initvar(mjs_State *J, const char *name, int idx)
{
	jsR_defproperty(J, J->E->variables, name, JS_DONTENUM | JS_DONTCONF, stackidx(J, idx), NULL, NULL);
}

static int mjs_hasvar(mjs_State *J, const char *name)
{
	mjs_Environment *E = J->E;
	do {
		mjs_Property *ref = jsV_getproperty(J, E->variables, name);
		if (ref) {
			if (ref->getter) {
				mjs_pushobject(J, ref->getter);
				mjs_pushobject(J, E->variables);
				mjs_call(J, 0);
			} else {
				mjs_pushvalue(J, ref->value);
			}
			return 1;
		}
		E = E->outer;
	} while (E);
	return 0;
}

static void mjs_setvar(mjs_State *J, const char *name)
{
	mjs_Environment *E = J->E;
	do {
		mjs_Property *ref = jsV_getproperty(J, E->variables, name);
		if (ref) {
			if (ref->setter) {
				mjs_pushobject(J, ref->setter);
				mjs_pushobject(J, E->variables);
				mjs_copy(J, -3);
				mjs_call(J, 1);
				mjs_pop(J, 1);
				return;
			}
			if (!(ref->atts & JS_READONLY))
				ref->value = *stackidx(J, -1);
			else if (J->strict)
				mjs_typeerror(J, "'%s' is read-only", name);
			return;
		}
		E = E->outer;
	} while (E);
	if (J->strict)
		mjs_referenceerror(J, "assignment to undeclared variable '%s'", name);
	jsR_setproperty(J, J->G, name);
}

static int mjs_delvar(mjs_State *J, const char *name)
{
	mjs_Environment *E = J->E;
	do {
		mjs_Property *ref = jsV_getownproperty(J, E->variables, name);
		if (ref) {
			if (ref->atts & JS_DONTCONF) {
				if (J->strict)
					mjs_typeerror(J, "'%s' is non-configurable", name);
				return 0;
			}
			jsV_delproperty(J, E->variables, name);
			return 1;
		}
		E = E->outer;
	} while (E);
	return jsR_delproperty(J, J->G, name);
}

/* Function calls */

static void jsR_savescope(mjs_State *J, mjs_Environment *newE)
{
	if (J->envtop + 1 >= JS_ENVLIMIT)
		mjs_stackoverflow(J);
	J->envstack[J->envtop++] = J->E;
	J->E = newE;
}

static void jsR_restorescope(mjs_State *J)
{
	J->E = J->envstack[--J->envtop];
}

static void jsR_calllwfunction(mjs_State *J, int n, mjs_Function *F, mjs_Environment *scope)
{
	mjs_Value v;
	int i;

	jsR_savescope(J, scope);

	if (n > F->numparams) {
		mjs_pop(J, n - F->numparams);
		n = F->numparams;
	}

	for (i = n; i < F->varlen; ++i)
		mjs_pushundefined(J);

	jsR_run(J, F);
	v = *stackidx(J, -1);
	TOP = --BOT; /* clear stack */
	mjs_pushvalue(J, v);

	jsR_restorescope(J);
}

static void jsR_callfunction(mjs_State *J, int n, mjs_Function *F, mjs_Environment *scope)
{
	mjs_Value v;
	int i;

	scope = jsR_newenvironment(J, jsV_newobject(J, JS_COBJECT, NULL), scope);

	jsR_savescope(J, scope);

	if (F->arguments) {
		mjs_newarguments(J);
		if (!J->strict) {
			mjs_currentfunction(J);
			mjs_defproperty(J, -2, "callee", JS_DONTENUM);
		}
		mjs_pushnumber(J, n);
		mjs_defproperty(J, -2, "length", JS_DONTENUM);
		for (i = 0; i < n; ++i) {
			mjs_copy(J, i + 1);
			mjs_setindex(J, -2, i);
		}
		mjs_initvar(J, "arguments", -1);
		mjs_pop(J, 1);
	}

	for (i = 0; i < n && i < F->numparams; ++i)
		mjs_initvar(J, F->vartab[i], i + 1);
	mjs_pop(J, n);

	for (; i < F->varlen; ++i) {
		mjs_pushundefined(J);
		mjs_initvar(J, F->vartab[i], -1);
		mjs_pop(J, 1);
	}

	jsR_run(J, F);
	v = *stackidx(J, -1);
	TOP = --BOT; /* clear stack */
	mjs_pushvalue(J, v);

	jsR_restorescope(J);
}

static void jsR_calleval(mjs_State *J, int n, mjs_Function *F, mjs_Environment *scope)
{
	mjs_Value v;
	int i;

	scope = jsR_newenvironment(J, jsV_newobject(J, JS_COBJECT, NULL), scope);

	jsR_savescope(J, scope);

	/* scripts take no arguments */
	mjs_pop(J, n);

	for (i = 0; i < F->varlen; ++i) {
		mjs_pushundefined(J);
		mjs_initvar(J, F->vartab[i], -1);
		mjs_pop(J, 1);
	}

	jsR_run(J, F);
	v = *stackidx(J, -1);
	TOP = --BOT; /* clear stack */
	mjs_pushvalue(J, v);

	jsR_restorescope(J);
}

static void jsR_callscript(mjs_State *J, int n, mjs_Function *F, mjs_Environment *scope)
{
	mjs_Value v;
	int i;

	if (scope)
		jsR_savescope(J, scope);

	/* scripts take no arguments */
	mjs_pop(J, n);

	for (i = 0; i < F->varlen; ++i) {
		mjs_pushundefined(J);
		mjs_initvar(J, F->vartab[i], -1);
		mjs_pop(J, 1);
	}

	jsR_run(J, F);
	v = *stackidx(J, -1);
	TOP = --BOT; /* clear stack */
	mjs_pushvalue(J, v);

	if (scope)
		jsR_restorescope(J);
}

static void jsR_callcfunction(mjs_State *J, int n, int min, mjs_CFunction F)
{
	int i;
	mjs_Value v;

	for (i = n; i < min; ++i)
		mjs_pushundefined(J);

	F(J);
	v = *stackidx(J, -1);
	TOP = --BOT; /* clear stack */
	mjs_pushvalue(J, v);
}

static void jsR_pushtrace(mjs_State *J, const char *name, const char *file, int line)
{
	if (J->tracetop + 1 == JS_ENVLIMIT)
		mjs_error(J, "call stack overflow");
	++J->tracetop;
	J->trace[J->tracetop].name = name;
	J->trace[J->tracetop].file = file;
	J->trace[J->tracetop].line = line;
}

void mjs_call(mjs_State *J, int n)
{
	mjs_Object *obj;
	int savebot;

	if (!mjs_iscallable(J, -n-2))
		mjs_typeerror(J, "%s is not callable", mjs_typeof(J, -n-2));

	obj = mjs_toobject(J, -n-2);

	savebot = BOT;
	BOT = TOP - n - 1;

	if (obj->type == JS_CFUNCTION) {
		jsR_pushtrace(J, obj->u.f.function->name, obj->u.f.function->filename, obj->u.f.function->line);
		if (obj->u.f.function->lightweight)
			jsR_calllwfunction(J, n, obj->u.f.function, obj->u.f.scope);
		else
			jsR_callfunction(J, n, obj->u.f.function, obj->u.f.scope);
		--J->tracetop;
	} else if (obj->type == JS_CSCRIPT) {
		jsR_pushtrace(J, obj->u.f.function->name, obj->u.f.function->filename, obj->u.f.function->line);
		jsR_callscript(J, n, obj->u.f.function, obj->u.f.scope);
		--J->tracetop;
	} else if (obj->type == JS_CEVAL) {
		jsR_pushtrace(J, obj->u.f.function->name, obj->u.f.function->filename, obj->u.f.function->line);
		jsR_calleval(J, n, obj->u.f.function, obj->u.f.scope);
		--J->tracetop;
	} else if (obj->type == JS_CCFUNCTION) {
		jsR_pushtrace(J, obj->u.c.name, "native", 0);
		jsR_callcfunction(J, n, obj->u.c.length, obj->u.c.function);
		--J->tracetop;
	}

	BOT = savebot;
}

void mjs_construct(mjs_State *J, int n)
{
	mjs_Object *obj;
	mjs_Object *prototype;
	mjs_Object *newobj;

	if (!mjs_iscallable(J, -n-1))
		mjs_typeerror(J, "%s is not callable", mjs_typeof(J, -n-1));

	obj = mjs_toobject(J, -n-1);

	/* built-in constructors create their own objects, give them a 'null' this */
	if (obj->type == JS_CCFUNCTION && obj->u.c.constructor) {
		int savebot = BOT;
		mjs_pushnull(J);
		if (n > 0)
			mjs_rot(J, n + 1);
		BOT = TOP - n - 1;

		jsR_pushtrace(J, obj->u.c.name, "native", 0);
		jsR_callcfunction(J, n, obj->u.c.length, obj->u.c.constructor);
		--J->tracetop;

		BOT = savebot;
		return;
	}

	/* extract the function object's prototype property */
	mjs_getproperty(J, -n - 1, "prototype");
	if (mjs_isobject(J, -1))
		prototype = mjs_toobject(J, -1);
	else
		prototype = J->Object_prototype;
	mjs_pop(J, 1);

	/* create a new object with above prototype, and shift it into the 'this' slot */
	newobj = jsV_newobject(J, JS_COBJECT, prototype);
	mjs_pushobject(J, newobj);
	if (n > 0)
		mjs_rot(J, n + 1);

	/* call the function */
	mjs_call(J, n);

	/* if result is not an object, return the original object we created */
	if (!mjs_isobject(J, -1)) {
		mjs_pop(J, 1);
		mjs_pushobject(J, newobj);
	}
}

void mjs_eval(mjs_State *J)
{
	if (!mjs_isstring(J, -1))
		return;
	mjs_loadeval(J, "(eval)", mjs_tostring(J, -1));
	mjs_rot2pop1(J);
	mjs_copy(J, 0); /* copy 'this' */
	mjs_call(J, 0);
}

int mjs_pconstruct(mjs_State *J, int n)
{
	int savetop = TOP - n - 2;
	if (mjs_try(J)) {
		/* clean up the stack to only hold the error object */
		STACK[savetop] = STACK[TOP-1];
		TOP = savetop + 1;
		return 1;
	}
	mjs_construct(J, n);
	mjs_endtry(J);
	return 0;
}

int mjs_pcall(mjs_State *J, int n)
{
	int savetop = TOP - n - 2;
	if (mjs_try(J)) {
		/* clean up the stack to only hold the error object */
		STACK[savetop] = STACK[TOP-1];
		TOP = savetop + 1;
		return 1;
	}
	mjs_call(J, n);
	mjs_endtry(J);
	return 0;
}

/* Exceptions */

void *mjs_savetrypc(mjs_State *J, mjs_Instruction *pc)
{
	if (J->trytop == JS_TRYLIMIT)
		mjs_error(J, "try: exception stack overflow");
	J->trybuf[J->trytop].E = J->E;
	J->trybuf[J->trytop].envtop = J->envtop;
	J->trybuf[J->trytop].tracetop = J->tracetop;
	J->trybuf[J->trytop].top = J->top;
	J->trybuf[J->trytop].bot = J->bot;
	J->trybuf[J->trytop].strict = J->strict;
	J->trybuf[J->trytop].pc = pc;
	return J->trybuf[J->trytop++].buf;
}

void *mjs_savetry(mjs_State *J)
{
	if (J->trytop == JS_TRYLIMIT)
		mjs_error(J, "try: exception stack overflow");
	J->trybuf[J->trytop].E = J->E;
	J->trybuf[J->trytop].envtop = J->envtop;
	J->trybuf[J->trytop].tracetop = J->tracetop;
	J->trybuf[J->trytop].top = J->top;
	J->trybuf[J->trytop].bot = J->bot;
	J->trybuf[J->trytop].strict = J->strict;
	J->trybuf[J->trytop].pc = NULL;
	return J->trybuf[J->trytop++].buf;
}

void mjs_endtry(mjs_State *J)
{
	if (J->trytop == 0)
		mjs_error(J, "endtry: exception stack underflow");
	--J->trytop;
}

void mjs_throw(mjs_State *J)
{
	if (J->trytop > 0) {
		mjs_Value v = *stackidx(J, -1);
		--J->trytop;
		J->E = J->trybuf[J->trytop].E;
		J->envtop = J->trybuf[J->trytop].envtop;
		J->tracetop = J->trybuf[J->trytop].tracetop;
		J->top = J->trybuf[J->trytop].top;
		J->bot = J->trybuf[J->trytop].bot;
		J->strict = J->trybuf[J->trytop].strict;
		mjs_pushvalue(J, v);
		longjmp(J->trybuf[J->trytop].buf, 1);
	}
	if (J->panic)
		J->panic(J);
	abort();
}

/* Main interpreter loop */

static void jsR_dumpstack(mjs_State *J)
{
	int i;
	printf("stack {\n");
	for (i = 0; i < TOP; ++i) {
		putchar(i == BOT ? '>' : ' ');
		printf("%4d: ", i);
		mjs_dumpvalue(J, STACK[i]);
		putchar('\n');
	}
	printf("}\n");
}

static void jsR_dumpenvironment(mjs_State *J, mjs_Environment *E, int d)
{
	printf("scope %d ", d);
	mjs_dumpobject(J, E->variables);
	if (E->outer)
		jsR_dumpenvironment(J, E->outer, d+1);
}

void mjs_stacktrace(mjs_State *J)
{
	int n;
	printf("stack trace:\n");
	for (n = J->tracetop; n >= 0; --n) {
		const char *name = J->trace[n].name;
		const char *file = J->trace[n].file;
		int line = J->trace[n].line;
		if (line > 0) {
			if (name[0])
				printf("\tat %s (%s:%d)\n", name, file, line);
			else
				printf("\tat %s:%d\n", file, line);
		} else
			printf("\tat %s (%s)\n", name, file);
	}
}

void mjs_trap(mjs_State *J, int pc)
{
	if (pc > 0) {
		mjs_Function *F = STACK[BOT-1].u.object->u.f.function;
		printf("trap at %d in function ", pc);
		jsC_dumpfunction(J, F);
	}
	jsR_dumpstack(J);
	jsR_dumpenvironment(J, J->E, 0);
	mjs_stacktrace(J);
}

static void jsR_run(mjs_State *J, mjs_Function *F)
{
	mjs_Function **FT = F->funtab;
	double *NT = F->numtab;
	const char **ST = F->strtab;
	const char **VT = F->vartab-1;
	int lightweight = F->lightweight;
	mjs_Instruction *pcstart = F->code;
	mjs_Instruction *pc = F->code;
	enum mjs_OpCode opcode;
	int offset;
	int savestrict;

	const char *str;
	mjs_Object *obj;
	double x, y;
	unsigned int ux, uy;
	int ix, iy, okay;
	int b;

	savestrict = J->strict;
	J->strict = F->strict;

	while (1) {
		if (J->gccounter > JS_GCLIMIT)
			mjs_gc(J, 0);

		J->trace[J->tracetop].line = *pc++;

		opcode = *pc++;

		switch (opcode) {
		case OP_POP: mjs_pop(J, 1); break;
		case OP_DUP: mjs_dup(J); break;
		case OP_DUP2: mjs_dup2(J); break;
		case OP_ROT2: mjs_rot2(J); break;
		case OP_ROT3: mjs_rot3(J); break;
		case OP_ROT4: mjs_rot4(J); break;

		case OP_INTEGER: mjs_pushnumber(J, *pc++ - 32768); break;
		case OP_NUMBER: mjs_pushnumber(J, NT[*pc++]); break;
		case OP_STRING: mjs_pushliteral(J, ST[*pc++]); break;

		case OP_CLOSURE: mjs_newfunction(J, FT[*pc++], J->E); break;
		case OP_NEWOBJECT: mjs_newobject(J); break;
		case OP_NEWARRAY: mjs_newarray(J); break;
		case OP_NEWREGEXP: mjs_newregexp(J, ST[pc[0]], pc[1]); pc += 2; break;

		case OP_UNDEF: mjs_pushundefined(J); break;
		case OP_NULL: mjs_pushnull(J); break;
		case OP_TRUE: mjs_pushboolean(J, 1); break;
		case OP_FALSE: mjs_pushboolean(J, 0); break;

		case OP_THIS:
			if (J->strict) {
				mjs_copy(J, 0);
			} else {
				if (mjs_iscoercible(J, 0))
					mjs_copy(J, 0);
				else
					mjs_pushglobal(J);
			}
			break;

		case OP_CURRENT:
			mjs_currentfunction(J);
			break;

		case OP_GETLOCAL:
			if (lightweight) {
				CHECKSTACK(1);
				STACK[TOP++] = STACK[BOT + *pc++];
			} else {
				str = VT[*pc++];
				if (!mjs_hasvar(J, str))
					mjs_referenceerror(J, "'%s' is not defined", str);
			}
			break;

		case OP_SETLOCAL:
			if (lightweight) {
				STACK[BOT + *pc++] = STACK[TOP-1];
			} else {
				mjs_setvar(J, VT[*pc++]);
			}
			break;

		case OP_DELLOCAL:
			if (lightweight) {
				++pc;
				mjs_pushboolean(J, 0);
			} else {
				b = mjs_delvar(J, VT[*pc++]);
				mjs_pushboolean(J, b);
			}
			break;

		case OP_GETVAR:
			str = ST[*pc++];
			if (!mjs_hasvar(J, str))
				mjs_referenceerror(J, "'%s' is not defined", str);
			break;

		case OP_HASVAR:
			if (!mjs_hasvar(J, ST[*pc++]))
				mjs_pushundefined(J);
			break;

		case OP_SETVAR:
			mjs_setvar(J, ST[*pc++]);
			break;

		case OP_DELVAR:
			b = mjs_delvar(J, ST[*pc++]);
			mjs_pushboolean(J, b);
			break;

		case OP_IN:
			str = mjs_tostring(J, -2);
			if (!mjs_isobject(J, -1))
				mjs_typeerror(J, "operand to 'in' is not an object");
			b = mjs_hasproperty(J, -1, str);
			mjs_pop(J, 2 + b);
			mjs_pushboolean(J, b);
			break;

		case OP_INITPROP:
			obj = mjs_toobject(J, -3);
			str = mjs_tostring(J, -2);
			jsR_setproperty(J, obj, str);
			mjs_pop(J, 2);
			break;

		case OP_INITGETTER:
			obj = mjs_toobject(J, -3);
			str = mjs_tostring(J, -2);
			jsR_defproperty(J, obj, str, 0, NULL, jsR_tofunction(J, -1), NULL);
			mjs_pop(J, 2);
			break;

		case OP_INITSETTER:
			obj = mjs_toobject(J, -3);
			str = mjs_tostring(J, -2);
			jsR_defproperty(J, obj, str, 0, NULL, NULL, jsR_tofunction(J, -1));
			mjs_pop(J, 2);
			break;

		case OP_GETPROP:
			str = mjs_tostring(J, -1);
			obj = mjs_toobject(J, -2);
			jsR_getproperty(J, obj, str);
			mjs_rot3pop2(J);
			break;

		case OP_GETPROP_S:
			str = ST[*pc++];
			obj = mjs_toobject(J, -1);
			jsR_getproperty(J, obj, str);
			mjs_rot2pop1(J);
			break;

		case OP_SETPROP:
			str = mjs_tostring(J, -2);
			obj = mjs_toobject(J, -3);
			jsR_setproperty(J, obj, str);
			mjs_rot3pop2(J);
			break;

		case OP_SETPROP_S:
			str = ST[*pc++];
			obj = mjs_toobject(J, -2);
			jsR_setproperty(J, obj, str);
			mjs_rot2pop1(J);
			break;

		case OP_DELPROP:
			str = mjs_tostring(J, -1);
			obj = mjs_toobject(J, -2);
			b = jsR_delproperty(J, obj, str);
			mjs_pop(J, 2);
			mjs_pushboolean(J, b);
			break;

		case OP_DELPROP_S:
			str = ST[*pc++];
			obj = mjs_toobject(J, -1);
			b = jsR_delproperty(J, obj, str);
			mjs_pop(J, 1);
			mjs_pushboolean(J, b);
			break;

		case OP_ITERATOR:
			if (mjs_iscoercible(J, -1)) {
				obj = jsV_newiterator(J, mjs_toobject(J, -1), 0);
				mjs_pop(J, 1);
				mjs_pushobject(J, obj);
			}
			break;

		case OP_NEXTITER:
			if (mjs_isobject(J, -1)) {
				obj = mjs_toobject(J, -1);
				str = jsV_nextiterator(J, obj);
				if (str) {
					mjs_pushliteral(J, str);
					mjs_pushboolean(J, 1);
				} else {
					mjs_pop(J, 1);
					mjs_pushboolean(J, 0);
				}
			} else {
				mjs_pop(J, 1);
				mjs_pushboolean(J, 0);
			}
			break;

		/* Function calls */

		case OP_EVAL:
			mjs_eval(J);
			break;

		case OP_CALL:
			mjs_call(J, *pc++);
			break;

		case OP_NEW:
			mjs_construct(J, *pc++);
			break;

		/* Unary operators */

		case OP_TYPEOF:
			str = mjs_typeof(J, -1);
			mjs_pop(J, 1);
			mjs_pushliteral(J, str);
			break;

		case OP_POS:
			x = mjs_tonumber(J, -1);
			mjs_pop(J, 1);
			mjs_pushnumber(J, x);
			break;

		case OP_NEG:
			x = mjs_tonumber(J, -1);
			mjs_pop(J, 1);
			mjs_pushnumber(J, -x);
			break;

		case OP_BITNOT:
			ix = mjs_toint32(J, -1);
			mjs_pop(J, 1);
			mjs_pushnumber(J, ~ix);
			break;

		case OP_LOGNOT:
			b = mjs_toboolean(J, -1);
			mjs_pop(J, 1);
			mjs_pushboolean(J, !b);
			break;

		case OP_INC:
			x = mjs_tonumber(J, -1);
			mjs_pop(J, 1);
			mjs_pushnumber(J, x + 1);
			break;

		case OP_DEC:
			x = mjs_tonumber(J, -1);
			mjs_pop(J, 1);
			mjs_pushnumber(J, x - 1);
			break;

		case OP_POSTINC:
			x = mjs_tonumber(J, -1);
			mjs_pop(J, 1);
			mjs_pushnumber(J, x + 1);
			mjs_pushnumber(J, x);
			break;

		case OP_POSTDEC:
			x = mjs_tonumber(J, -1);
			mjs_pop(J, 1);
			mjs_pushnumber(J, x - 1);
			mjs_pushnumber(J, x);
			break;

		/* Multiplicative operators */

		case OP_MUL:
			x = mjs_tonumber(J, -2);
			y = mjs_tonumber(J, -1);
			mjs_pop(J, 2);
			mjs_pushnumber(J, x * y);
			break;

		case OP_DIV:
			x = mjs_tonumber(J, -2);
			y = mjs_tonumber(J, -1);
			mjs_pop(J, 2);
			mjs_pushnumber(J, x / y);
			break;

		case OP_MOD:
			x = mjs_tonumber(J, -2);
			y = mjs_tonumber(J, -1);
			mjs_pop(J, 2);
			mjs_pushnumber(J, fmod(x, y));
			break;

		/* Additive operators */

		case OP_ADD:
			mjs_concat(J);
			break;

		case OP_SUB:
			x = mjs_tonumber(J, -2);
			y = mjs_tonumber(J, -1);
			mjs_pop(J, 2);
			mjs_pushnumber(J, x - y);
			break;

		/* Shift operators */

		case OP_SHL:
			ix = mjs_toint32(J, -2);
			uy = mjs_touint32(J, -1);
			mjs_pop(J, 2);
			mjs_pushnumber(J, ix << (uy & 0x1F));
			break;

		case OP_SHR:
			ix = mjs_toint32(J, -2);
			uy = mjs_touint32(J, -1);
			mjs_pop(J, 2);
			mjs_pushnumber(J, ix >> (uy & 0x1F));
			break;

		case OP_USHR:
			ux = mjs_touint32(J, -2);
			uy = mjs_touint32(J, -1);
			mjs_pop(J, 2);
			mjs_pushnumber(J, ux >> (uy & 0x1F));
			break;

		/* Relational operators */

		case OP_LT: b = mjs_compare(J, &okay); mjs_pop(J, 2); mjs_pushboolean(J, okay && b < 0); break;
		case OP_GT: b = mjs_compare(J, &okay); mjs_pop(J, 2); mjs_pushboolean(J, okay && b > 0); break;
		case OP_LE: b = mjs_compare(J, &okay); mjs_pop(J, 2); mjs_pushboolean(J, okay && b <= 0); break;
		case OP_GE: b = mjs_compare(J, &okay); mjs_pop(J, 2); mjs_pushboolean(J, okay && b >= 0); break;

		case OP_INSTANCEOF:
			b = mjs_instanceof(J);
			mjs_pop(J, 2);
			mjs_pushboolean(J, b);
			break;

		/* Equality */

		case OP_EQ: b = mjs_equal(J); mjs_pop(J, 2); mjs_pushboolean(J, b); break;
		case OP_NE: b = mjs_equal(J); mjs_pop(J, 2); mjs_pushboolean(J, !b); break;
		case OP_STRICTEQ: b = mjs_strictequal(J); mjs_pop(J, 2); mjs_pushboolean(J, b); break;
		case OP_STRICTNE: b = mjs_strictequal(J); mjs_pop(J, 2); mjs_pushboolean(J, !b); break;

		case OP_JCASE:
			offset = *pc++;
			b = mjs_strictequal(J);
			if (b) {
				mjs_pop(J, 2);
				pc = pcstart + offset;
			} else {
				mjs_pop(J, 1);
			}
			break;

		/* Binary bitwise operators */

		case OP_BITAND:
			ix = mjs_toint32(J, -2);
			iy = mjs_toint32(J, -1);
			mjs_pop(J, 2);
			mjs_pushnumber(J, ix & iy);
			break;

		case OP_BITXOR:
			ix = mjs_toint32(J, -2);
			iy = mjs_toint32(J, -1);
			mjs_pop(J, 2);
			mjs_pushnumber(J, ix ^ iy);
			break;

		case OP_BITOR:
			ix = mjs_toint32(J, -2);
			iy = mjs_toint32(J, -1);
			mjs_pop(J, 2);
			mjs_pushnumber(J, ix | iy);
			break;

		/* Try and Catch */

		case OP_THROW:
			mjs_throw(J);

		case OP_TRY:
			offset = *pc++;
			if (mjs_trypc(J, pc)) {
				pc = J->trybuf[J->trytop].pc;
			} else {
				pc = pcstart + offset;
			}
			break;

		case OP_ENDTRY:
			mjs_endtry(J);
			break;

		case OP_CATCH:
			str = ST[*pc++];
			obj = jsV_newobject(J, JS_COBJECT, NULL);
			mjs_pushobject(J, obj);
			mjs_rot2(J);
			mjs_setproperty(J, -2, str);
			J->E = jsR_newenvironment(J, obj, J->E);
			mjs_pop(J, 1);
			break;

		case OP_ENDCATCH:
			J->E = J->E->outer;
			break;

		/* With */

		case OP_WITH:
			obj = mjs_toobject(J, -1);
			J->E = jsR_newenvironment(J, obj, J->E);
			mjs_pop(J, 1);
			break;

		case OP_ENDWITH:
			J->E = J->E->outer;
			break;

		/* Branching */

		case OP_DEBUGGER:
			mjs_trap(J, (int)(pc - pcstart) - 1);
			break;

		case OP_JUMP:
			pc = pcstart + *pc;
			break;

		case OP_JTRUE:
			offset = *pc++;
			b = mjs_toboolean(J, -1);
			mjs_pop(J, 1);
			if (b)
				pc = pcstart + offset;
			break;

		case OP_JFALSE:
			offset = *pc++;
			b = mjs_toboolean(J, -1);
			mjs_pop(J, 1);
			if (!b)
				pc = pcstart + offset;
			break;

		case OP_RETURN:
			J->strict = savestrict;
			return;
		}
	}
}
