#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

#if defined(_MSC_VER) && (_MSC_VER < 1700) /* VS2012 has stdint.h */
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

static void jsB_new_Number(mjs_State *J)
{
	mjs_newnumber(J, mjs_gettop(J) > 1 ? mjs_tonumber(J, 1) : 0);
}

static void jsB_Number(mjs_State *J)
{
	mjs_pushnumber(J, mjs_gettop(J) > 1 ? mjs_tonumber(J, 1) : 0);
}

static void Np_valueOf(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	if (self->type != JS_CNUMBER) mjs_typeerror(J, "not a number");
	mjs_pushnumber(J, self->u.number);
}

static void Np_toString(mjs_State *J)
{
	char buf[100];
	mjs_Object *self = mjs_toobject(J, 0);
	int radix = mjs_isundefined(J, 1) ? 10 : mjs_tointeger(J, 1);
	if (self->type != JS_CNUMBER)
		mjs_typeerror(J, "not a number");
	if (radix == 10) {
		mjs_pushstring(J, jsV_numbertostring(J, buf, self->u.number));
		return;
	}
	if (radix < 2 || radix > 36)
		mjs_rangeerror(J, "invalid radix");

	/* lame number to string conversion for any radix from 2 to 36 */
	{
		static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
		double number = self->u.number;
		int sign = self->u.number < 0;
		mjs_Buffer *sb = NULL;
		uint64_t u, limit = ((uint64_t)1<<52);

		int ndigits, exp, point;

		if (number == 0) { mjs_pushstring(J, "0"); return; }
		if (isnan(number)) { mjs_pushstring(J, "NaN"); return; }
		if (isinf(number)) { mjs_pushstring(J, sign ? "-Infinity" : "Infinity"); return; }

		if (sign)
			number = -number;

		/* fit as many digits as we want in an int */
		exp = 0;
		while (number * pow(radix, exp) > limit)
			--exp;
		while (number * pow(radix, exp+1) < limit)
			++exp;
		u = number * pow(radix, exp) + 0.5;

		/* trim trailing zeros */
		while (u > 0 && (u % radix) == 0) {
			u /= radix;
			--exp;
		}

		/* serialize digits */
		ndigits = 0;
		while (u > 0) {
			buf[ndigits++] = digits[u % radix];
			u /= radix;
		}
		point = ndigits - exp;

		if (mjs_try(J)) {
			mjs_free(J, sb);
			mjs_throw(J);
		}

		if (sign)
			mjs_putc(J, &sb, '-');

		if (point <= 0) {
			mjs_putc(J, &sb, '0');
			mjs_putc(J, &sb, '.');
			while (point++ < 0)
				mjs_putc(J, &sb, '0');
			while (ndigits-- > 0)
				mjs_putc(J, &sb, buf[ndigits]);
		} else {
			while (ndigits-- > 0) {
				mjs_putc(J, &sb, buf[ndigits]);
				if (--point == 0 && ndigits > 0)
					mjs_putc(J, &sb, '.');
			}
			while (point-- > 0)
				mjs_putc(J, &sb, '0');
		}

		mjs_putc(J, &sb, 0);
		mjs_pushstring(J, sb->s);

		mjs_endtry(J);
		mjs_free(J, sb);
	}
}

/* Customized ToString() on a number */
static void numtostr(mjs_State *J, const char *fmt, int w, double n)
{
	/* buf needs to fit printf("%.20f", 1e20) */
	char buf[50], *e;
	sprintf(buf, fmt, w, n);
	e = strchr(buf, 'e');
	if (e) {
		int exp = atoi(e+1);
		sprintf(e, "e%+d", exp);
	}
	mjs_pushstring(J, buf);
}

static void Np_toFixed(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	int width = mjs_tointeger(J, 1);
	char buf[32];
	double x;
	if (self->type != JS_CNUMBER) mjs_typeerror(J, "not a number");
	if (width < 0) mjs_rangeerror(J, "precision %d out of range", width);
	if (width > 20) mjs_rangeerror(J, "precision %d out of range", width);
	x = self->u.number;
	if (isnan(x) || isinf(x) || x <= -1e21 || x >= 1e21)
		mjs_pushstring(J, jsV_numbertostring(J, buf, x));
	else
		numtostr(J, "%.*f", width, x);
}

static void Np_toExponential(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	int width = mjs_tointeger(J, 1);
	char buf[32];
	double x;
	if (self->type != JS_CNUMBER) mjs_typeerror(J, "not a number");
	if (width < 0) mjs_rangeerror(J, "precision %d out of range", width);
	if (width > 20) mjs_rangeerror(J, "precision %d out of range", width);
	x = self->u.number;
	if (isnan(x) || isinf(x))
		mjs_pushstring(J, jsV_numbertostring(J, buf, x));
	else
		numtostr(J, "%.*e", width, self->u.number);
}

static void Np_toPrecision(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	int width = mjs_tointeger(J, 1);
	char buf[32];
	double x;
	if (self->type != JS_CNUMBER) mjs_typeerror(J, "not a number");
	if (width < 1) mjs_rangeerror(J, "precision %d out of range", width);
	if (width > 21) mjs_rangeerror(J, "precision %d out of range", width);
	x = self->u.number;
	if (isnan(x) || isinf(x))
		mjs_pushstring(J, jsV_numbertostring(J, buf, x));
	else
		numtostr(J, "%.*g", width, self->u.number);
}

void jsB_initnumber(mjs_State *J)
{
	J->Number_prototype->u.number = 0;

	mjs_pushobject(J, J->Number_prototype);
	{
		jsB_propf(J, "Number.prototype.valueOf", Np_valueOf, 0);
		jsB_propf(J, "Number.prototype.toString", Np_toString, 1);
		jsB_propf(J, "Number.prototype.toLocaleString", Np_toString, 0);
		jsB_propf(J, "Number.prototype.toFixed", Np_toFixed, 1);
		jsB_propf(J, "Number.prototype.toExponential", Np_toExponential, 1);
		jsB_propf(J, "Number.prototype.toPrecision", Np_toPrecision, 1);
	}
	mjs_newcconstructor(J, jsB_Number, jsB_new_Number, "Number", 0); /* 1 */
	{
		jsB_propn(J, "MAX_VALUE", 1.7976931348623157e+308);
		jsB_propn(J, "MIN_VALUE", 5e-324);
		jsB_propn(J, "NaN", NAN);
		jsB_propn(J, "NEGATIVE_INFINITY", -INFINITY);
		jsB_propn(J, "POSITIVE_INFINITY", INFINITY);
	}
	mjs_defglobal(J, "Number", JS_DONTENUM);
}
