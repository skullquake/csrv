#include "jsi.h"
#include "jslex.h"
#include "jsvalue.h"
#include "jsbuiltin.h"
#include "jscompile.h"
#include "utf.h"

static void reprvalue(mjs_State *J, mjs_Buffer **sb);

static void reprnum(mjs_State *J, mjs_Buffer **sb, double n)
{
	char buf[40];
	if (n == 0 && signbit(n))
		mjs_puts(J, sb, "-0");
	else
		mjs_puts(J, sb, jsV_numbertostring(J, buf, n));
}

static void reprstr(mjs_State *J, mjs_Buffer **sb, const char *s)
{
	static const char *HEX = "0123456789ABCDEF";
	Rune c;
	mjs_putc(J, sb, '"');
	while (*s) {
		s += chartorune(&c, s);
		switch (c) {
		case '"': mjs_puts(J, sb, "\\\""); break;
		case '\\': mjs_puts(J, sb, "\\\\"); break;
		case '\b': mjs_puts(J, sb, "\\b"); break;
		case '\f': mjs_puts(J, sb, "\\f"); break;
		case '\n': mjs_puts(J, sb, "\\n"); break;
		case '\r': mjs_puts(J, sb, "\\r"); break;
		case '\t': mjs_puts(J, sb, "\\t"); break;
		default:
			if (c < ' ' || c > 127) {
				mjs_puts(J, sb, "\\u");
				mjs_putc(J, sb, HEX[(c>>12)&15]);
				mjs_putc(J, sb, HEX[(c>>8)&15]);
				mjs_putc(J, sb, HEX[(c>>4)&15]);
				mjs_putc(J, sb, HEX[c&15]);
			} else {
				mjs_putc(J, sb, c); break;
			}
		}
	}
	mjs_putc(J, sb, '"');
}

#ifndef isalpha
#define isalpha(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#endif
#ifndef isdigit
#define isdigit(c) (c >= '0' && c <= '9')
#endif

static void reprident(mjs_State *J, mjs_Buffer **sb, const char *name)
{
	const char *p = name;
	if (isdigit(*p))
		while (isdigit(*p))
			++p;
	else if (isalpha(*p) || *p == '_')
		while (isdigit(*p) || isalpha(*p) || *p == '_')
			++p;
	if (p > name && *p == 0)
		mjs_puts(J, sb, name);
	else
		reprstr(J, sb, name);
}

static void reprobject(mjs_State *J, mjs_Buffer **sb)
{
	const char *key;
	int i, n;

	n = mjs_gettop(J) - 1;
	for (i = 0; i < n; ++i) {
		if (mjs_isobject(J, i)) {
			if (mjs_toobject(J, i) == mjs_toobject(J, -1)) {
				mjs_puts(J, sb, "{}");
				return;
			}
		}
	}

	n = 0;
	mjs_putc(J, sb, '{');
	mjs_pushiterator(J, -1, 1);
	while ((key = mjs_nextiterator(J, -1))) {
		if (n++ > 0)
			mjs_puts(J, sb, ", ");
		reprident(J, sb, key);
		mjs_puts(J, sb, ": ");
		mjs_getproperty(J, -2, key);
		reprvalue(J, sb);
		mjs_pop(J, 1);
	}
	mjs_pop(J, 1);
	mjs_putc(J, sb, '}');
}

static void reprarray(mjs_State *J, mjs_Buffer **sb)
{
	int n, i;

	n = mjs_gettop(J) - 1;
	for (i = 0; i < n; ++i) {
		if (mjs_isobject(J, i)) {
			if (mjs_toobject(J, i) == mjs_toobject(J, -1)) {
				mjs_puts(J, sb, "[]");
				return;
			}
		}
	}

	mjs_putc(J, sb, '[');
	n = mjs_getlength(J, -1);
	for (i = 0; i < n; ++i) {
		if (i > 0)
			mjs_puts(J, sb, ", ");
		if (mjs_hasindex(J, -1, i)) {
			reprvalue(J, sb);
			mjs_pop(J, 1);
		}
	}
	mjs_putc(J, sb, ']');
}

static void reprfun(mjs_State *J, mjs_Buffer **sb, mjs_Function *fun)
{
	int i;
	mjs_puts(J, sb, "function ");
	mjs_puts(J, sb, fun->name);
	mjs_putc(J, sb, '(');
	for (i = 0; i < fun->numparams; ++i) {
		if (i > 0)
			mjs_puts(J, sb, ", ");
		mjs_puts(J, sb, fun->vartab[i]);
	}
	mjs_puts(J, sb, ") { [byte code] }");
}

static void reprvalue(mjs_State *J, mjs_Buffer **sb)
{
	if (mjs_isundefined(J, -1))
		mjs_puts(J, sb, "undefined");
	else if (mjs_isnull(J, -1))
		mjs_puts(J, sb, "null");
	else if (mjs_isboolean(J, -1))
		mjs_puts(J, sb, mjs_toboolean(J, -1) ? "true" : "false");
	else if (mjs_isnumber(J, -1))
		reprnum(J, sb, mjs_tonumber(J, -1));
	else if (mjs_isstring(J, -1))
		reprstr(J, sb, mjs_tostring(J, -1));
	else if (mjs_isobject(J, -1)) {
		mjs_Object *obj = mjs_toobject(J, -1);
		switch (obj->type) {
		default:
			reprobject(J, sb);
			break;
		case JS_CARRAY:
			reprarray(J, sb);
			break;
		case JS_CFUNCTION:
		case JS_CSCRIPT:
		case JS_CEVAL:
			reprfun(J, sb, obj->u.f.function);
			break;
		case JS_CCFUNCTION:
			mjs_puts(J, sb, "function ");
			mjs_puts(J, sb, obj->u.c.name);
			mjs_puts(J, sb, "() { [native code] }");
			break;
		case JS_CBOOLEAN:
			mjs_puts(J, sb, "(new Boolean(");
			mjs_puts(J, sb, obj->u.boolean ? "true" : "false");
			mjs_puts(J, sb, "))");
			break;
		case JS_CNUMBER:
			mjs_puts(J, sb, "(new Number(");
			reprnum(J, sb, obj->u.number);
			mjs_puts(J, sb, "))");
			break;
		case JS_CSTRING:
			mjs_puts(J, sb, "(new String(");
			reprstr(J, sb, obj->u.s.string);
			mjs_puts(J, sb, "))");
			break;
		case JS_CREGEXP:
			mjs_putc(J, sb, '/');
			mjs_puts(J, sb, obj->u.r.source);
			mjs_putc(J, sb, '/');
			if (obj->u.r.flags & JS_REGEXP_G) mjs_putc(J, sb, 'g');
			if (obj->u.r.flags & JS_REGEXP_I) mjs_putc(J, sb, 'i');
			if (obj->u.r.flags & JS_REGEXP_M) mjs_putc(J, sb, 'm');
			break;
		case JS_CDATE:
			{
				char buf[40];
				mjs_puts(J, sb, "(new Date(");
				mjs_puts(J, sb, jsV_numbertostring(J, buf, obj->u.number));
				mjs_puts(J, sb, "))");
			}
			break;
		case JS_CERROR:
			mjs_puts(J, sb, "(new ");
			mjs_getproperty(J, -1, "name");
			mjs_puts(J, sb, mjs_tostring(J, -1));
			mjs_pop(J, 1);
			mjs_putc(J, sb, '(');
			mjs_getproperty(J, -1, "message");
			reprstr(J, sb, mjs_tostring(J, -1));
			mjs_pop(J, 1);
			mjs_puts(J, sb, "))");
			break;
		case JS_CMATH:
			mjs_puts(J, sb, "Math");
			break;
		case JS_CJSON:
			mjs_puts(J, sb, "JSON");
			break;
		case JS_CITERATOR:
			mjs_puts(J, sb, "[iterator ");
			break;
		case JS_CUSERDATA:
			mjs_puts(J, sb, "[userdata ");
			mjs_puts(J, sb, obj->u.user.tag);
			mjs_putc(J, sb, ']');
			break;
		}
	}
}

void mjs_repr(mjs_State *J, int idx)
{
	mjs_Buffer *sb = NULL;
	int savebot;

	if (mjs_try(J)) {
		mjs_free(J, sb);
		mjs_throw(J);
	}

	mjs_copy(J, idx);

	savebot = J->bot;
	J->bot = J->top - 1;
	reprvalue(J, &sb);
	J->bot = savebot;

	mjs_pop(J, 1);

	mjs_putc(J, &sb, 0);
	mjs_pushstring(J, sb ? sb->s : "undefined");

	mjs_endtry(J);
	mjs_free(J, sb);
}

const char *mjs_torepr(mjs_State *J, int idx)
{
	mjs_repr(J, idx);
	mjs_replace(J, idx < 0 ? idx-1 : idx);
	return mjs_tostring(J, idx);
}

const char *mjs_tryrepr(mjs_State *J, int idx, const char *error)
{
	const char *s;
	if (mjs_try(J)) {
		mjs_pop(J, 1);
		return error;
	}
	s = mjs_torepr(J, idx);
	mjs_endtry(J);
	return s;
}
