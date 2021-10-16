#include "jsi.h"
#include "jslex.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "utf.h"

#define JSV_ISSTRING(v) (v->type==JS_TSHRSTR || v->type==JS_TMEMSTR || v->type==JS_TLITSTR)
#define JSV_TOSTRING(v) (v->type==JS_TSHRSTR ? v->u.shrstr : v->type==JS_TLITSTR ? v->u.litstr : v->type==JS_TMEMSTR ? v->u.memstr->p : "")

int jsV_numbertointeger(double n)
{
	if (n == 0) return 0;
	if (isnan(n)) return 0;
	n = (n < 0) ? -floor(-n) : floor(n);
	if (n < INT_MIN) return INT_MIN;
	if (n > INT_MAX) return INT_MAX;
	return (int)n;
}

int jsV_numbertoint32(double n)
{
	double two32 = 4294967296.0;
	double two31 = 2147483648.0;

	if (!isfinite(n) || n == 0)
		return 0;

	n = fmod(n, two32);
	n = n >= 0 ? floor(n) : ceil(n) + two32;
	if (n >= two31)
		return n - two32;
	else
		return n;
}

unsigned int jsV_numbertouint32(double n)
{
	return (unsigned int)jsV_numbertoint32(n);
}

short jsV_numbertoint16(double n)
{
	return jsV_numbertoint32(n);
}

unsigned short jsV_numbertouint16(double n)
{
	return jsV_numbertoint32(n);
}

/* obj.toString() */
static int jsV_toString(mjs_State *J, mjs_Object *obj)
{
	mjs_pushobject(J, obj);
	mjs_getproperty(J, -1, "toString");
	if (mjs_iscallable(J, -1)) {
		mjs_rot2(J);
		mjs_call(J, 0);
		if (mjs_isprimitive(J, -1))
			return 1;
		mjs_pop(J, 1);
		return 0;
	}
	mjs_pop(J, 2);
	return 0;
}

/* obj.valueOf() */
static int jsV_valueOf(mjs_State *J, mjs_Object *obj)
{
	mjs_pushobject(J, obj);
	mjs_getproperty(J, -1, "valueOf");
	if (mjs_iscallable(J, -1)) {
		mjs_rot2(J);
		mjs_call(J, 0);
		if (mjs_isprimitive(J, -1))
			return 1;
		mjs_pop(J, 1);
		return 0;
	}
	mjs_pop(J, 2);
	return 0;
}

/* ToPrimitive() on a value */
void jsV_toprimitive(mjs_State *J, mjs_Value *v, int preferred)
{
	mjs_Object *obj;

	if (v->type != JS_TOBJECT)
		return;

	obj = v->u.object;

	if (preferred == JS_HNONE)
		preferred = obj->type == JS_CDATE ? JS_HSTRING : JS_HNUMBER;

	if (preferred == JS_HSTRING) {
		if (jsV_toString(J, obj) || jsV_valueOf(J, obj)) {
			*v = *mjs_tovalue(J, -1);
			mjs_pop(J, 1);
			return;
		}
	} else {
		if (jsV_valueOf(J, obj) || jsV_toString(J, obj)) {
			*v = *mjs_tovalue(J, -1);
			mjs_pop(J, 1);
			return;
		}
	}

	if (J->strict)
		mjs_typeerror(J, "cannot convert object to primitive");

	v->type = JS_TLITSTR;
	v->u.litstr = "[object]";
	return;
}

/* ToBoolean() on a value */
int jsV_toboolean(mjs_State *J, mjs_Value *v)
{
	switch (v->type) {
	default:
	case JS_TSHRSTR: return v->u.shrstr[0] != 0;
	case JS_TUNDEFINED: return 0;
	case JS_TNULL: return 0;
	case JS_TBOOLEAN: return v->u.boolean;
	case JS_TNUMBER: return v->u.number != 0 && !isnan(v->u.number);
	case JS_TLITSTR: return v->u.litstr[0] != 0;
	case JS_TMEMSTR: return v->u.memstr->p[0] != 0;
	case JS_TOBJECT: return 1;
	}
}

const char *mjs_itoa(char *out, int v)
{
	char buf[32], *s = out;
	unsigned int a;
	int i = 0;
	if (v < 0) {
		a = -v;
		*s++ = '-';
	} else {
		a = v;
	}
	while (a) {
		buf[i++] = (a % 10) + '0';
		a /= 10;
	}
	if (i == 0)
		buf[i++] = '0';
	while (i > 0)
		*s++ = buf[--i];
	*s = 0;
	return out;
}

double mjs_stringtofloat(const char *s, char **ep)
{
	char *end;
	double n;
	const char *e = s;
	int isflt = 0;
	if (*e == '+' || *e == '-') ++e;
	while (*e >= '0' && *e <= '9') ++e;
	if (*e == '.') { ++e; isflt = 1; }
	while (*e >= '0' && *e <= '9') ++e;
	if (*e == 'e' || *e == 'E') {
		++e;
		if (*e == '+' || *e == '-') ++e;
		while (*e >= '0' && *e <= '9') ++e;
		isflt = 1;
	}
	if (isflt || e - s > 9)
		n = mjs_strtod(s, &end);
	else
		n = strtol(s, &end, 10);
	if (end == e) {
		*ep = (char*)e;
		return n;
	}
	*ep = (char*)s;
	return 0;
}

/* ToNumber() on a string */
double jsV_stringtonumber(mjs_State *J, const char *s)
{
	char *e;
	double n;
	while (jsY_iswhite(*s) || jsY_isnewline(*s)) ++s;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X') && s[2] != 0)
		n = strtol(s + 2, &e, 16);
	else if (!strncmp(s, "Infinity", 8))
		n = INFINITY, e = (char*)s + 8;
	else if (!strncmp(s, "+Infinity", 9))
		n = INFINITY, e = (char*)s + 9;
	else if (!strncmp(s, "-Infinity", 9))
		n = -INFINITY, e = (char*)s + 9;
	else
		n = mjs_stringtofloat(s, &e);
	while (jsY_iswhite(*e) || jsY_isnewline(*e)) ++e;
	if (*e) return NAN;
	return n;
}

/* ToNumber() on a value */
double jsV_tonumber(mjs_State *J, mjs_Value *v)
{
	switch (v->type) {
	default:
	case JS_TSHRSTR: return jsV_stringtonumber(J, v->u.shrstr);
	case JS_TUNDEFINED: return NAN;
	case JS_TNULL: return 0;
	case JS_TBOOLEAN: return v->u.boolean;
	case JS_TNUMBER: return v->u.number;
	case JS_TLITSTR: return jsV_stringtonumber(J, v->u.litstr);
	case JS_TMEMSTR: return jsV_stringtonumber(J, v->u.memstr->p);
	case JS_TOBJECT:
		jsV_toprimitive(J, v, JS_HNUMBER);
		return jsV_tonumber(J, v);
	}
}

double jsV_tointeger(mjs_State *J, mjs_Value *v)
{
	return jsV_numbertointeger(jsV_tonumber(J, v));
}

/* ToString() on a number */
const char *jsV_numbertostring(mjs_State *J, char buf[32], double f)
{
	char digits[32], *p = buf, *s = digits;
	int exp, ndigits, point;

	if (f == 0) return "0";
	if (isnan(f)) return "NaN";
	if (isinf(f)) return f < 0 ? "-Infinity" : "Infinity";

	/* Fast case for integers. This only works assuming all integers can be
	 * exactly represented by a float. This is true for 32-bit integers and
	 * 64-bit floats. */
	if (f >= INT_MIN && f <= INT_MAX) {
		int i = (int)f;
		if ((double)i == f)
			return mjs_itoa(buf, i);
	}

	ndigits = mjs_grisu2(f, digits, &exp);
	point = ndigits + exp;

	if (signbit(f))
		*p++ = '-';

	if (point < -5 || point > 21) {
		*p++ = *s++;
		if (ndigits > 1) {
			int n = ndigits - 1;
			*p++ = '.';
			while (n--)
				*p++ = *s++;
		}
		mjs_fmtexp(p, point - 1);
	}

	else if (point <= 0) {
		*p++ = '0';
		*p++ = '.';
		while (point++ < 0)
			*p++ = '0';
		while (ndigits-- > 0)
			*p++ = *s++;
		*p = 0;
	}

	else {
		while (ndigits-- > 0) {
			*p++ = *s++;
			if (--point == 0 && ndigits > 0)
				*p++ = '.';
		}
		while (point-- > 0)
			*p++ = '0';
		*p = 0;
	}

	return buf;
}

/* ToString() on a value */
const char *jsV_tostring(mjs_State *J, mjs_Value *v)
{
	char buf[32];
	const char *p;
	switch (v->type) {
	default:
	case JS_TSHRSTR: return v->u.shrstr;
	case JS_TUNDEFINED: return "undefined";
	case JS_TNULL: return "null";
	case JS_TBOOLEAN: return v->u.boolean ? "true" : "false";
	case JS_TLITSTR: return v->u.litstr;
	case JS_TMEMSTR: return v->u.memstr->p;
	case JS_TNUMBER:
		p = jsV_numbertostring(J, buf, v->u.number);
		if (p == buf) {
			int n = strlen(p);
			if (n <= soffsetof(mjs_Value, type)) {
				char *s = v->u.shrstr;
				while (n--) *s++ = *p++;
				*s = 0;
				v->type = JS_TSHRSTR;
				return v->u.shrstr;
			} else {
				v->u.memstr = jsV_newmemstring(J, p, n);
				v->type = JS_TMEMSTR;
				return v->u.memstr->p;
			}
		}
		return p;
	case JS_TOBJECT:
		jsV_toprimitive(J, v, JS_HSTRING);
		return jsV_tostring(J, v);
	}
}

/* Objects */

static mjs_Object *jsV_newboolean(mjs_State *J, int v)
{
	mjs_Object *obj = jsV_newobject(J, JS_CBOOLEAN, J->Boolean_prototype);
	obj->u.boolean = v;
	return obj;
}

static mjs_Object *jsV_newnumber(mjs_State *J, double v)
{
	mjs_Object *obj = jsV_newobject(J, JS_CNUMBER, J->Number_prototype);
	obj->u.number = v;
	return obj;
}

static mjs_Object *jsV_newstring(mjs_State *J, const char *v)
{
	mjs_Object *obj = jsV_newobject(J, JS_CSTRING, J->String_prototype);
	obj->u.s.string = mjs_intern(J, v); /* TODO: mjs_String */
	obj->u.s.length = utflen(v);
	return obj;
}

/* ToObject() on a value */
mjs_Object *jsV_toobject(mjs_State *J, mjs_Value *v)
{
	switch (v->type) {
	default:
	case JS_TSHRSTR: return jsV_newstring(J, v->u.shrstr);
	case JS_TUNDEFINED: mjs_typeerror(J, "cannot convert undefined to object");
	case JS_TNULL: mjs_typeerror(J, "cannot convert null to object");
	case JS_TBOOLEAN: return jsV_newboolean(J, v->u.boolean);
	case JS_TNUMBER: return jsV_newnumber(J, v->u.number);
	case JS_TLITSTR: return jsV_newstring(J, v->u.litstr);
	case JS_TMEMSTR: return jsV_newstring(J, v->u.memstr->p);
	case JS_TOBJECT: return v->u.object;
	}
}

void mjs_newobjectx(mjs_State *J)
{
	mjs_Object *prototype = NULL;
	if (mjs_isobject(J, -1))
		prototype = mjs_toobject(J, -1);
	mjs_pop(J, 1);
	mjs_pushobject(J, jsV_newobject(J, JS_COBJECT, prototype));
}

void mjs_newobject(mjs_State *J)
{
	mjs_pushobject(J, jsV_newobject(J, JS_COBJECT, J->Object_prototype));
}

void mjs_newarguments(mjs_State *J)
{
	mjs_pushobject(J, jsV_newobject(J, JS_CARGUMENTS, J->Object_prototype));
}

void mjs_newarray(mjs_State *J)
{
	mjs_pushobject(J, jsV_newobject(J, JS_CARRAY, J->Array_prototype));
}

void mjs_newboolean(mjs_State *J, int v)
{
	mjs_pushobject(J, jsV_newboolean(J, v));
}

void mjs_newnumber(mjs_State *J, double v)
{
	mjs_pushobject(J, jsV_newnumber(J, v));
}

void mjs_newstring(mjs_State *J, const char *v)
{
	mjs_pushobject(J, jsV_newstring(J, v));
}

void mjs_newfunction(mjs_State *J, mjs_Function *fun, mjs_Environment *scope)
{
	mjs_Object *obj = jsV_newobject(J, JS_CFUNCTION, J->Function_prototype);
	obj->u.f.function = fun;
	obj->u.f.scope = scope;
	mjs_pushobject(J, obj);
	{
		mjs_pushnumber(J, fun->numparams);
		mjs_defproperty(J, -2, "length", JS_READONLY | JS_DONTENUM | JS_DONTCONF);
		mjs_newobject(J);
		{
			mjs_copy(J, -2);
			mjs_defproperty(J, -2, "constructor", JS_DONTENUM);
		}
		mjs_defproperty(J, -2, "prototype", JS_DONTENUM | JS_DONTCONF);
	}
}

void mjs_newscript(mjs_State *J, mjs_Function *fun, mjs_Environment *scope, int type)
{
	mjs_Object *obj = jsV_newobject(J, type, NULL);
	obj->u.f.function = fun;
	obj->u.f.scope = scope;
	mjs_pushobject(J, obj);
}

void mjs_newcfunction(mjs_State *J, mjs_CFunction cfun, const char *name, int length)
{
	mjs_Object *obj = jsV_newobject(J, JS_CCFUNCTION, J->Function_prototype);
	obj->u.c.name = name;
	obj->u.c.function = cfun;
	obj->u.c.constructor = NULL;
	obj->u.c.length = length;
	mjs_pushobject(J, obj);
	{
		mjs_pushnumber(J, length);
		mjs_defproperty(J, -2, "length", JS_READONLY | JS_DONTENUM | JS_DONTCONF);
		mjs_newobject(J);
		{
			mjs_copy(J, -2);
			mjs_defproperty(J, -2, "constructor", JS_DONTENUM);
		}
		mjs_defproperty(J, -2, "prototype", JS_DONTENUM | JS_DONTCONF);
	}
}

/* prototype -- constructor */
void mjs_newcconstructor(mjs_State *J, mjs_CFunction cfun, mjs_CFunction ccon, const char *name, int length)
{
	mjs_Object *obj = jsV_newobject(J, JS_CCFUNCTION, J->Function_prototype);
	obj->u.c.name = name;
	obj->u.c.function = cfun;
	obj->u.c.constructor = ccon;
	obj->u.c.length = length;
	mjs_pushobject(J, obj); /* proto obj */
	{
		mjs_pushnumber(J, length);
		mjs_defproperty(J, -2, "length", JS_READONLY | JS_DONTENUM | JS_DONTCONF);
		mjs_rot2(J); /* obj proto */
		mjs_copy(J, -2); /* obj proto obj */
		mjs_defproperty(J, -2, "constructor", JS_DONTENUM);
		mjs_defproperty(J, -2, "prototype", JS_DONTENUM | JS_DONTCONF);
	}
}

void mjs_newuserdatax(mjs_State *J, const char *tag, void *data, mjs_HasProperty has, mjs_Put put, mjs_Delete delete, mjs_Finalize finalize)
{
	mjs_Object *prototype = NULL;
	mjs_Object *obj;

	if (mjs_isobject(J, -1))
		prototype = mjs_toobject(J, -1);
	mjs_pop(J, 1);

	obj = jsV_newobject(J, JS_CUSERDATA, prototype);
	obj->u.user.tag = tag;
	obj->u.user.data = data;
	obj->u.user.has = has;
	obj->u.user.put = put;
	obj->u.user.delete = delete;
	obj->u.user.finalize = finalize;
	mjs_pushobject(J, obj);
}

void mjs_newuserdata(mjs_State *J, const char *tag, void *data, mjs_Finalize finalize)
{
	mjs_newuserdatax(J, tag, data, NULL, NULL, NULL, finalize);
}

/* Non-trivial operations on values. These are implemented using the stack. */

int mjs_instanceof(mjs_State *J)
{
	mjs_Object *O, *V;

	if (!mjs_iscallable(J, -1))
		mjs_typeerror(J, "instanceof: invalid operand");

	if (!mjs_isobject(J, -2))
		return 0;

	mjs_getproperty(J, -1, "prototype");
	if (!mjs_isobject(J, -1))
		mjs_typeerror(J, "instanceof: 'prototype' property is not an object");
	O = mjs_toobject(J, -1);
	mjs_pop(J, 1);

	V = mjs_toobject(J, -2);
	while (V) {
		V = V->prototype;
		if (O == V)
			return 1;
	}

	return 0;
}

void mjs_concat(mjs_State *J)
{
	mjs_toprimitive(J, -2, JS_HNONE);
	mjs_toprimitive(J, -1, JS_HNONE);

	if (mjs_isstring(J, -2) || mjs_isstring(J, -1)) {
		const char *sa = mjs_tostring(J, -2);
		const char *sb = mjs_tostring(J, -1);
		/* TODO: create mjs_String directly */
		char *sab = mjs_malloc(J, strlen(sa) + strlen(sb) + 1);
		strcpy(sab, sa);
		strcat(sab, sb);
		if (mjs_try(J)) {
			mjs_free(J, sab);
			mjs_throw(J);
		}
		mjs_pop(J, 2);
		mjs_pushstring(J, sab);
		mjs_endtry(J);
		mjs_free(J, sab);
	} else {
		double x = mjs_tonumber(J, -2);
		double y = mjs_tonumber(J, -1);
		mjs_pop(J, 2);
		mjs_pushnumber(J, x + y);
	}
}

int mjs_compare(mjs_State *J, int *okay)
{
	mjs_toprimitive(J, -2, JS_HNUMBER);
	mjs_toprimitive(J, -1, JS_HNUMBER);

	*okay = 1;
	if (mjs_isstring(J, -2) && mjs_isstring(J, -1)) {
		return strcmp(mjs_tostring(J, -2), mjs_tostring(J, -1));
	} else {
		double x = mjs_tonumber(J, -2);
		double y = mjs_tonumber(J, -1);
		if (isnan(x) || isnan(y))
			*okay = 0;
		return x < y ? -1 : x > y ? 1 : 0;
	}
}

int mjs_equal(mjs_State *J)
{
	mjs_Value *x = mjs_tovalue(J, -2);
	mjs_Value *y = mjs_tovalue(J, -1);

retry:
	if (JSV_ISSTRING(x) && JSV_ISSTRING(y))
		return !strcmp(JSV_TOSTRING(x), JSV_TOSTRING(y));
	if (x->type == y->type) {
		if (x->type == JS_TUNDEFINED) return 1;
		if (x->type == JS_TNULL) return 1;
		if (x->type == JS_TNUMBER) return x->u.number == y->u.number;
		if (x->type == JS_TBOOLEAN) return x->u.boolean == y->u.boolean;
		if (x->type == JS_TOBJECT) return x->u.object == y->u.object;
		return 0;
	}

	if (x->type == JS_TNULL && y->type == JS_TUNDEFINED) return 1;
	if (x->type == JS_TUNDEFINED && y->type == JS_TNULL) return 1;

	if (x->type == JS_TNUMBER && JSV_ISSTRING(y))
		return x->u.number == jsV_tonumber(J, y);
	if (JSV_ISSTRING(x) && y->type == JS_TNUMBER)
		return jsV_tonumber(J, x) == y->u.number;

	if (x->type == JS_TBOOLEAN) {
		x->type = JS_TNUMBER;
		x->u.number = x->u.boolean ? 1 : 0;
		goto retry;
	}
	if (y->type == JS_TBOOLEAN) {
		y->type = JS_TNUMBER;
		y->u.number = y->u.boolean ? 1 : 0;
		goto retry;
	}
	if ((JSV_ISSTRING(x) || x->type == JS_TNUMBER) && y->type == JS_TOBJECT) {
		jsV_toprimitive(J, y, JS_HNONE);
		goto retry;
	}
	if (x->type == JS_TOBJECT && (JSV_ISSTRING(y) || y->type == JS_TNUMBER)) {
		jsV_toprimitive(J, x, JS_HNONE);
		goto retry;
	}

	return 0;
}

int mjs_strictequal(mjs_State *J)
{
	mjs_Value *x = mjs_tovalue(J, -2);
	mjs_Value *y = mjs_tovalue(J, -1);

	if (JSV_ISSTRING(x) && JSV_ISSTRING(y))
		return !strcmp(JSV_TOSTRING(x), JSV_TOSTRING(y));

	if (x->type != y->type) return 0;
	if (x->type == JS_TUNDEFINED) return 1;
	if (x->type == JS_TNULL) return 1;
	if (x->type == JS_TNUMBER) return x->u.number == y->u.number;
	if (x->type == JS_TBOOLEAN) return x->u.boolean == y->u.boolean;
	if (x->type == JS_TOBJECT) return x->u.object == y->u.object;
	return 0;
}
