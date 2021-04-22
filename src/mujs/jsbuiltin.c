#include "jsi.h"
#include "jslex.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static void jsB_globalf(mjs_State *J, const char *name, mjs_CFunction cfun, int n)
{
	mjs_newcfunction(J, cfun, name, n);
	mjs_defglobal(J, name, JS_DONTENUM);
}

void jsB_propf(mjs_State *J, const char *name, mjs_CFunction cfun, int n)
{
	const char *pname = strrchr(name, '.');
	pname = pname ? pname + 1 : name;
	mjs_newcfunction(J, cfun, name, n);
	mjs_defproperty(J, -2, pname, JS_DONTENUM);
}

void jsB_propn(mjs_State *J, const char *name, double number)
{
	mjs_pushnumber(J, number);
	mjs_defproperty(J, -2, name, JS_READONLY | JS_DONTENUM | JS_DONTCONF);
}

void jsB_props(mjs_State *J, const char *name, const char *string)
{
	mjs_pushliteral(J, string);
	mjs_defproperty(J, -2, name, JS_DONTENUM);
}

static void jsB_parseInt(mjs_State *J)
{
	const char *s = mjs_tostring(J, 1);
	int radix = mjs_isdefined(J, 2) ? mjs_tointeger(J, 2) : 10;
	double sign = 1;
	double n;
	char *e;

	while (jsY_iswhite(*s) || jsY_isnewline(*s))
		++s;
	if (*s == '-') {
		++s;
		sign = -1;
	} else if (*s == '+') {
		++s;
	}
	if (radix == 0) {
		radix = 10;
		if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
			s += 2;
			radix = 16;
		}
	} else if (radix < 2 || radix > 36) {
		mjs_pushnumber(J, NAN);
		return;
	}
	n = strtol(s, &e, radix);
	if (s == e)
		mjs_pushnumber(J, NAN);
	else
		mjs_pushnumber(J, n * sign);
}

static void jsB_parseFloat(mjs_State *J)
{
	const char *s = mjs_tostring(J, 1);
	char *e;
	double n;

	while (jsY_iswhite(*s) || jsY_isnewline(*s)) ++s;
	if (!strncmp(s, "Infinity", 8))
		mjs_pushnumber(J, INFINITY);
	else if (!strncmp(s, "+Infinity", 9))
		mjs_pushnumber(J, INFINITY);
	else if (!strncmp(s, "-Infinity", 9))
		mjs_pushnumber(J, -INFINITY);
	else {
		n = mjs_stringtofloat(s, &e);
		if (e == s)
			mjs_pushnumber(J, NAN);
		else
			mjs_pushnumber(J, n);
	}
}

static void jsB_isNaN(mjs_State *J)
{
	double n = mjs_tonumber(J, 1);
	mjs_pushboolean(J, isnan(n));
}

static void jsB_isFinite(mjs_State *J)
{
	double n = mjs_tonumber(J, 1);
	mjs_pushboolean(J, isfinite(n));
}

static void Encode(mjs_State *J, const char *str, const char *unescaped)
{
	mjs_Buffer *sb = NULL;

	static const char *HEX = "0123456789ABCDEF";

	if (mjs_try(J)) {
		mjs_free(J, sb);
		mjs_throw(J);
	}

	while (*str) {
		int c = (unsigned char) *str++;
		if (strchr(unescaped, c))
			mjs_putc(J, &sb, c);
		else {
			mjs_putc(J, &sb, '%');
			mjs_putc(J, &sb, HEX[(c >> 4) & 0xf]);
			mjs_putc(J, &sb, HEX[c & 0xf]);
		}
	}
	mjs_putc(J, &sb, 0);

	mjs_pushstring(J, sb ? sb->s : "");
	mjs_endtry(J);
	mjs_free(J, sb);
}

static void Decode(mjs_State *J, const char *str, const char *reserved)
{
	mjs_Buffer *sb = NULL;
	int a, b;

	if (mjs_try(J)) {
		mjs_free(J, sb);
		mjs_throw(J);
	}

	while (*str) {
		int c = (unsigned char) *str++;
		if (c != '%')
			mjs_putc(J, &sb, c);
		else {
			if (!str[0] || !str[1])
				mjs_urierror(J, "truncated escape sequence");
			a = *str++;
			b = *str++;
			if (!jsY_ishex(a) || !jsY_ishex(b))
				mjs_urierror(J, "invalid escape sequence");
			c = jsY_tohex(a) << 4 | jsY_tohex(b);
			if (!strchr(reserved, c))
				mjs_putc(J, &sb, c);
			else {
				mjs_putc(J, &sb, '%');
				mjs_putc(J, &sb, a);
				mjs_putc(J, &sb, b);
			}
		}
	}
	mjs_putc(J, &sb, 0);

	mjs_pushstring(J, sb ? sb->s : "");
	mjs_endtry(J);
	mjs_free(J, sb);
}

#define URIRESERVED ";/?:@&=+$,"
#define URIALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define URIDIGIT "0123456789"
#define URIMARK "-_.!~*`()"
#define URIUNESCAPED URIALPHA URIDIGIT URIMARK

static void jsB_decodeURI(mjs_State *J)
{
	Decode(J, mjs_tostring(J, 1), URIRESERVED "#");
}

static void jsB_decodeURIComponent(mjs_State *J)
{
	Decode(J, mjs_tostring(J, 1), "");
}

static void jsB_encodeURI(mjs_State *J)
{
	Encode(J, mjs_tostring(J, 1), URIUNESCAPED URIRESERVED "#");
}

static void jsB_encodeURIComponent(mjs_State *J)
{
	Encode(J, mjs_tostring(J, 1), URIUNESCAPED);
}

void jsB_init(mjs_State *J)
{
	/* Create the prototype objects here, before the constructors */
	J->Object_prototype = jsV_newobject(J, JS_COBJECT, NULL);
	J->Array_prototype = jsV_newobject(J, JS_CARRAY, J->Object_prototype);
	J->Function_prototype = jsV_newobject(J, JS_CCFUNCTION, J->Object_prototype);
	J->Boolean_prototype = jsV_newobject(J, JS_CBOOLEAN, J->Object_prototype);
	J->Number_prototype = jsV_newobject(J, JS_CNUMBER, J->Object_prototype);
	J->String_prototype = jsV_newobject(J, JS_CSTRING, J->Object_prototype);
	J->RegExp_prototype = jsV_newobject(J, JS_COBJECT, J->Object_prototype);
	J->Date_prototype = jsV_newobject(J, JS_CDATE, J->Object_prototype);

	/* All the native error types */
	J->Error_prototype = jsV_newobject(J, JS_CERROR, J->Object_prototype);
	J->EvalError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);
	J->RangeError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);
	J->ReferenceError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);
	J->SyntaxError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);
	J->TypeError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);
	J->URIError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);

	/* Create the constructors and fill out the prototype objects */
	jsB_initobject(J);
	jsB_initarray(J);
	jsB_initfunction(J);
	jsB_initboolean(J);
	jsB_initnumber(J);
	jsB_initstring(J);
	jsB_initregexp(J);
	jsB_initdate(J);
	jsB_initerror(J);
	jsB_initmath(J);
	jsB_initjson(J);

	/* Initialize the global object */
	mjs_pushnumber(J, NAN);
	mjs_defglobal(J, "NaN", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	mjs_pushnumber(J, INFINITY);
	mjs_defglobal(J, "Infinity", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	mjs_pushundefined(J);
	mjs_defglobal(J, "undefined", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	jsB_globalf(J, "parseInt", jsB_parseInt, 1);
	jsB_globalf(J, "parseFloat", jsB_parseFloat, 1);
	jsB_globalf(J, "isNaN", jsB_isNaN, 1);
	jsB_globalf(J, "isFinite", jsB_isFinite, 1);

	jsB_globalf(J, "decodeURI", jsB_decodeURI, 1);
	jsB_globalf(J, "decodeURIComponent", jsB_decodeURIComponent, 1);
	jsB_globalf(J, "encodeURI", jsB_encodeURI, 1);
	jsB_globalf(J, "encodeURIComponent", jsB_encodeURIComponent, 1);
}
