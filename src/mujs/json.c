#include "jsi.h"
#include "jslex.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

#include "utf.h"

int mjs_isnumberobject(mjs_State *J, int idx)
{
	return mjs_isobject(J, idx) && mjs_toobject(J, idx)->type == JS_CNUMBER;
}

int mjs_isstringobject(mjs_State *J, int idx)
{
	return mjs_isobject(J, idx) && mjs_toobject(J, idx)->type == JS_CSTRING;
}

static void jsonnext(mjs_State *J)
{
	J->lookahead = jsY_lexjson(J);
}

static int jsonaccept(mjs_State *J, int t)
{
	if (J->lookahead == t) {
		jsonnext(J);
		return 1;
	}
	return 0;
}

static void jsonexpect(mjs_State *J, int t)
{
	if (!jsonaccept(J, t))
		mjs_syntaxerror(J, "JSON: unexpected token: %s (expected %s)",
				jsY_tokenstring(J->lookahead), jsY_tokenstring(t));
}

static void jsonvalue(mjs_State *J)
{
	int i;
	const char *name;

	switch (J->lookahead) {
	case TK_STRING:
		mjs_pushstring(J, J->text);
		jsonnext(J);
		break;

	case TK_NUMBER:
		mjs_pushnumber(J, J->number);
		jsonnext(J);
		break;

	case '{':
		mjs_newobject(J);
		jsonnext(J);
		if (jsonaccept(J, '}'))
			return;
		do {
			if (J->lookahead != TK_STRING)
				mjs_syntaxerror(J, "JSON: unexpected token: %s (expected string)", jsY_tokenstring(J->lookahead));
			name = J->text;
			jsonnext(J);
			jsonexpect(J, ':');
			jsonvalue(J);
			mjs_setproperty(J, -2, name);
		} while (jsonaccept(J, ','));
		jsonexpect(J, '}');
		break;

	case '[':
		mjs_newarray(J);
		jsonnext(J);
		i = 0;
		if (jsonaccept(J, ']'))
			return;
		do {
			jsonvalue(J);
			mjs_setindex(J, -2, i++);
		} while (jsonaccept(J, ','));
		jsonexpect(J, ']');
		break;

	case TK_TRUE:
		mjs_pushboolean(J, 1);
		jsonnext(J);
		break;

	case TK_FALSE:
		mjs_pushboolean(J, 0);
		jsonnext(J);
		break;

	case TK_NULL:
		mjs_pushnull(J);
		jsonnext(J);
		break;

	default:
		mjs_syntaxerror(J, "JSON: unexpected token: %s", jsY_tokenstring(J->lookahead));
	}
}

static void jsonrevive(mjs_State *J, const char *name)
{
	const char *key;
	char buf[32];

	/* revive is in 2 */
	/* holder is in -1 */

	mjs_getproperty(J, -1, name); /* get value from holder */

	if (mjs_isobject(J, -1)) {
		if (mjs_isarray(J, -1)) {
			int i = 0;
			int n = mjs_getlength(J, -1);
			for (i = 0; i < n; ++i) {
				jsonrevive(J, mjs_itoa(buf, i));
				if (mjs_isundefined(J, -1)) {
					mjs_pop(J, 1);
					mjs_delproperty(J, -1, buf);
				} else {
					mjs_setproperty(J, -2, buf);
				}
			}
		} else {
			mjs_pushiterator(J, -1, 1);
			while ((key = mjs_nextiterator(J, -1))) {
				mjs_rot2(J);
				jsonrevive(J, key);
				if (mjs_isundefined(J, -1)) {
					mjs_pop(J, 1);
					mjs_delproperty(J, -1, key);
				} else {
					mjs_setproperty(J, -2, key);
				}
				mjs_rot2(J);
			}
			mjs_pop(J, 1);
		}
	}

	mjs_copy(J, 2); /* reviver function */
	mjs_copy(J, -3); /* holder as this */
	mjs_pushstring(J, name); /* name */
	mjs_copy(J, -4); /* value */
	mjs_call(J, 2);
	mjs_rot2pop1(J); /* pop old value, leave new value on stack */
}

static void JSON_parse(mjs_State *J)
{
	const char *source = mjs_tostring(J, 1);
	jsY_initlex(J, "JSON", source);
	jsonnext(J);

	if (mjs_iscallable(J, 2)) {
		mjs_newobject(J);
		jsonvalue(J);
		mjs_defproperty(J, -2, "", 0);
		jsonrevive(J, "");
	} else {
		jsonvalue(J);
	}
}

static void fmtnum(mjs_State *J, mjs_Buffer **sb, double n)
{
	if (isnan(n)) mjs_puts(J, sb, "null");
	else if (isinf(n)) mjs_puts(J, sb, "null");
	else if (n == 0) mjs_puts(J, sb, "0");
	else {
		char buf[40];
		mjs_puts(J, sb, jsV_numbertostring(J, buf, n));
	}
}

static void fmtstr(mjs_State *J, mjs_Buffer **sb, const char *s)
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

static void fmtindent(mjs_State *J, mjs_Buffer **sb, const char *gap, int level)
{
	mjs_putc(J, sb, '\n');
	while (level--)
		mjs_puts(J, sb, gap);
}

static int fmtvalue(mjs_State *J, mjs_Buffer **sb, const char *key, const char *gap, int level);

static int filterprop(mjs_State *J, const char *key)
{
	int i, n, found;
	/* replacer/property-list is in stack slot 2 */
	if (mjs_isarray(J, 2)) {
		found = 0;
		n = mjs_getlength(J, 2);
		for (i = 0; i < n && !found; ++i) {
			mjs_getindex(J, 2, i);
			if (mjs_isstring(J, -1) || mjs_isnumber(J, -1) ||
				mjs_isstringobject(J, -1) || mjs_isnumberobject(J, -1))
				found = !strcmp(key, mjs_tostring(J, -1));
			mjs_pop(J, 1);
		}
		return found;
	}
	return 1;
}

static void fmtobject(mjs_State *J, mjs_Buffer **sb, mjs_Object *obj, const char *gap, int level)
{
	const char *key;
	int save;
	int i, n;

	n = mjs_gettop(J) - 1;
	for (i = 4; i < n; ++i)
		if (mjs_isobject(J, i))
			if (mjs_toobject(J, i) == mjs_toobject(J, -1))
				mjs_typeerror(J, "cyclic object value");

	n = 0;
	mjs_putc(J, sb, '{');
	mjs_pushiterator(J, -1, 1);
	while ((key = mjs_nextiterator(J, -1))) {
		if (filterprop(J, key)) {
			save = (*sb)->n;
			if (n) mjs_putc(J, sb, ',');
			if (gap) fmtindent(J, sb, gap, level + 1);
			fmtstr(J, sb, key);
			mjs_putc(J, sb, ':');
			if (gap)
				mjs_putc(J, sb, ' ');
			mjs_rot2(J);
			if (!fmtvalue(J, sb, key, gap, level + 1))
				(*sb)->n = save;
			else
				++n;
			mjs_rot2(J);
		}
	}
	mjs_pop(J, 1);
	if (gap && n) fmtindent(J, sb, gap, level);
	mjs_putc(J, sb, '}');
}

static void fmtarray(mjs_State *J, mjs_Buffer **sb, const char *gap, int level)
{
	int n, i;
	char buf[32];

	n = mjs_gettop(J) - 1;
	for (i = 4; i < n; ++i)
		if (mjs_isobject(J, i))
			if (mjs_toobject(J, i) == mjs_toobject(J, -1))
				mjs_typeerror(J, "cyclic object value");

	mjs_putc(J, sb, '[');
	n = mjs_getlength(J, -1);
	for (i = 0; i < n; ++i) {
		if (i) mjs_putc(J, sb, ',');
		if (gap) fmtindent(J, sb, gap, level + 1);
		if (!fmtvalue(J, sb, mjs_itoa(buf, i), gap, level + 1))
			mjs_puts(J, sb, "null");
	}
	if (gap && n) fmtindent(J, sb, gap, level);
	mjs_putc(J, sb, ']');
}

static int fmtvalue(mjs_State *J, mjs_Buffer **sb, const char *key, const char *gap, int level)
{
	/* replacer/property-list is in 2 */
	/* holder is in -1 */

	mjs_getproperty(J, -1, key);

	if (mjs_isobject(J, -1)) {
		if (mjs_hasproperty(J, -1, "toJSON")) {
			if (mjs_iscallable(J, -1)) {
				mjs_copy(J, -2);
				mjs_pushstring(J, key);
				mjs_call(J, 1);
				mjs_rot2pop1(J);
			} else {
				mjs_pop(J, 1);
			}
		}
	}

	if (mjs_iscallable(J, 2)) {
		mjs_copy(J, 2); /* replacer function */
		mjs_copy(J, -3); /* holder as this */
		mjs_pushstring(J, key); /* name */
		mjs_copy(J, -4); /* old value */
		mjs_call(J, 2);
		mjs_rot2pop1(J); /* pop old value, leave new value on stack */
	}

	if (mjs_isobject(J, -1) && !mjs_iscallable(J, -1)) {
		mjs_Object *obj = mjs_toobject(J, -1);
		switch (obj->type) {
		case JS_CNUMBER: fmtnum(J, sb, obj->u.number); break;
		case JS_CSTRING: fmtstr(J, sb, obj->u.s.string); break;
		case JS_CBOOLEAN: mjs_puts(J, sb, obj->u.boolean ? "true" : "false"); break;
		case JS_CARRAY: fmtarray(J, sb, gap, level); break;
		default: fmtobject(J, sb, obj, gap, level); break;
		}
	}
	else if (mjs_isboolean(J, -1))
		mjs_puts(J, sb, mjs_toboolean(J, -1) ? "true" : "false");
	else if (mjs_isnumber(J, -1))
		fmtnum(J, sb, mjs_tonumber(J, -1));
	else if (mjs_isstring(J, -1))
		fmtstr(J, sb, mjs_tostring(J, -1));
	else if (mjs_isnull(J, -1))
		mjs_puts(J, sb, "null");
	else {
		mjs_pop(J, 1);
		return 0;
	}

	mjs_pop(J, 1);
	return 1;
}

static void JSON_stringify(mjs_State *J)
{
	mjs_Buffer *sb = NULL;
	char buf[12];
	const char *s, *gap;
	int n;

	gap = NULL;

	if (mjs_isnumber(J, 3) || mjs_isnumberobject(J, 3)) {
		n = mjs_tointeger(J, 3);
		if (n < 0) n = 0;
		if (n > 10) n = 10;
		memset(buf, ' ', n);
		buf[n] = 0;
		if (n > 0) gap = buf;
	} else if (mjs_isstring(J, 3) || mjs_isstringobject(J, 3)) {
		s = mjs_tostring(J, 3);
		n = strlen(s);
		if (n > 10) n = 10;
		memcpy(buf, s, n);
		buf[n] = 0;
		if (n > 0) gap = buf;
	}

	if (mjs_try(J)) {
		mjs_free(J, sb);
		mjs_throw(J);
	}

	mjs_newobject(J); /* wrapper */
	mjs_copy(J, 1);
	mjs_defproperty(J, -2, "", 0);
	if (!fmtvalue(J, &sb, "", gap, 0)) {
		mjs_pushundefined(J);
	} else {
		mjs_putc(J, &sb, 0);
		mjs_pushstring(J, sb ? sb->s : "");
		mjs_rot2pop1(J);
	}

	mjs_endtry(J);
	mjs_free(J, sb);
}

void jsB_initjson(mjs_State *J)
{
	mjs_pushobject(J, jsV_newobject(J, JS_CJSON, J->Object_prototype));
	{
		jsB_propf(J, "JSON.parse", JSON_parse, 2);
		jsB_propf(J, "JSON.stringify", JSON_stringify, 3);
	}
	mjs_defglobal(J, "JSON", JS_DONTENUM);
}
