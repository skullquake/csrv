#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

int mjs_getlength(mjs_State *J, int idx)
{
	int len;
	mjs_getproperty(J, idx, "length");
	len = mjs_tointeger(J, -1);
	mjs_pop(J, 1);
	return len;
}

void mjs_setlength(mjs_State *J, int idx, int len)
{
	mjs_pushnumber(J, len);
	mjs_setproperty(J, idx < 0 ? idx - 1 : idx, "length");
}

int mjs_hasindex(mjs_State *J, int idx, int i)
{
	char buf[32];
	return mjs_hasproperty(J, idx, mjs_itoa(buf, i));
}

void mjs_getindex(mjs_State *J, int idx, int i)
{
	char buf[32];
	mjs_getproperty(J, idx, mjs_itoa(buf, i));
}

void mjs_setindex(mjs_State *J, int idx, int i)
{
	char buf[32];
	mjs_setproperty(J, idx, mjs_itoa(buf, i));
}

void mjs_delindex(mjs_State *J, int idx, int i)
{
	char buf[32];
	mjs_delproperty(J, idx, mjs_itoa(buf, i));
}

static void jsB_new_Array(mjs_State *J)
{
	int i, top = mjs_gettop(J);

	mjs_newarray(J);

	if (top == 2) {
		if (mjs_isnumber(J, 1)) {
			mjs_copy(J, 1);
			mjs_setproperty(J, -2, "length");
		} else {
			mjs_copy(J, 1);
			mjs_setindex(J, -2, 0);
		}
	} else {
		for (i = 1; i < top; ++i) {
			mjs_copy(J, i);
			mjs_setindex(J, -2, i - 1);
		}
	}
}

static void Ap_concat(mjs_State *J)
{
	int i, top = mjs_gettop(J);
	int n, k, len;

	mjs_newarray(J);
	n = 0;

	for (i = 0; i < top; ++i) {
		mjs_copy(J, i);
		if (mjs_isarray(J, -1)) {
			len = mjs_getlength(J, -1);
			for (k = 0; k < len; ++k)
				if (mjs_hasindex(J, -1, k))
					mjs_setindex(J, -3, n++);
			mjs_pop(J, 1);
		} else {
			mjs_setindex(J, -2, n++);
		}
	}
}

static void Ap_join(mjs_State *J)
{
	char * volatile out = NULL;
	const char *sep;
	const char *r;
	int seplen;
	int k, n, len;

	len = mjs_getlength(J, 0);

	if (mjs_isdefined(J, 1)) {
		sep = mjs_tostring(J, 1);
		seplen = strlen(sep);
	} else {
		sep = ",";
		seplen = 1;
	}

	if (len == 0) {
		mjs_pushliteral(J, "");
		return;
	}

	if (mjs_try(J)) {
		mjs_free(J, out);
		mjs_throw(J);
	}

	n = 1;
	for (k = 0; k < len; ++k) {
		mjs_getindex(J, 0, k);
		if (mjs_isundefined(J, -1) || mjs_isnull(J, -1))
			r = "";
		else
			r = mjs_tostring(J, -1);
		n += strlen(r);

		if (k == 0) {
			out = mjs_malloc(J, n);
			strcpy(out, r);
		} else {
			n += seplen;
			out = mjs_realloc(J, out, n);
			strcat(out, sep);
			strcat(out, r);
		}

		mjs_pop(J, 1);
	}

	mjs_pushstring(J, out);
	mjs_endtry(J);
	mjs_free(J, out);
}

static void Ap_pop(mjs_State *J)
{
	int n;

	n = mjs_getlength(J, 0);

	if (n > 0) {
		mjs_getindex(J, 0, n - 1);
		mjs_delindex(J, 0, n - 1);
		mjs_setlength(J, 0, n - 1);
	} else {
		mjs_setlength(J, 0, 0);
		mjs_pushundefined(J);
	}
}

static void Ap_push(mjs_State *J)
{
	int i, top = mjs_gettop(J);
	int n;

	n = mjs_getlength(J, 0);

	for (i = 1; i < top; ++i, ++n) {
		mjs_copy(J, i);
		mjs_setindex(J, 0, n);
	}

	mjs_setlength(J, 0, n);

	mjs_pushnumber(J, n);
}

static void Ap_reverse(mjs_State *J)
{
	int len, middle, lower;

	len = mjs_getlength(J, 0);
	middle = len / 2;
	lower = 0;

	while (lower != middle) {
		int upper = len - lower - 1;
		int haslower = mjs_hasindex(J, 0, lower);
		int hasupper = mjs_hasindex(J, 0, upper);
		if (haslower && hasupper) {
			mjs_setindex(J, 0, lower);
			mjs_setindex(J, 0, upper);
		} else if (hasupper) {
			mjs_setindex(J, 0, lower);
			mjs_delindex(J, 0, upper);
		} else if (haslower) {
			mjs_setindex(J, 0, upper);
			mjs_delindex(J, 0, lower);
		}
		++lower;
	}

	mjs_copy(J, 0);
}

static void Ap_shift(mjs_State *J)
{
	int k, len;

	len = mjs_getlength(J, 0);

	if (len == 0) {
		mjs_setlength(J, 0, 0);
		mjs_pushundefined(J);
		return;
	}

	mjs_getindex(J, 0, 0);

	for (k = 1; k < len; ++k) {
		if (mjs_hasindex(J, 0, k))
			mjs_setindex(J, 0, k - 1);
		else
			mjs_delindex(J, 0, k - 1);
	}

	mjs_delindex(J, 0, len - 1);
	mjs_setlength(J, 0, len - 1);
}

static void Ap_slice(mjs_State *J)
{
	int len, s, e, n;
	double sv, ev;

	mjs_newarray(J);

	len = mjs_getlength(J, 0);
	sv = mjs_tointeger(J, 1);
	ev = mjs_isdefined(J, 2) ? mjs_tointeger(J, 2) : len;

	if (sv < 0) sv = sv + len;
	if (ev < 0) ev = ev + len;

	s = sv < 0 ? 0 : sv > len ? len : sv;
	e = ev < 0 ? 0 : ev > len ? len : ev;

	for (n = 0; s < e; ++s, ++n)
		if (mjs_hasindex(J, 0, s))
			mjs_setindex(J, -2, n);
}

struct sortslot {
	mjs_Value v;
	mjs_State *J;
};

static int sortcmp(const void *avoid, const void *bvoid)
{
	const struct sortslot *aslot = avoid, *bslot = bvoid;
	const mjs_Value *a = &aslot->v, *b = &bslot->v;
	mjs_State *J = aslot->J;
	const char *sx, *sy;
	double v;
	int c;

	int unx = (a->type == JS_TUNDEFINED);
	int uny = (b->type == JS_TUNDEFINED);
	if (unx) return !uny;
	if (uny) return -1;

	if (mjs_iscallable(J, 1)) {
		mjs_copy(J, 1); /* copy function */
		mjs_pushundefined(J);
		mjs_pushvalue(J, *a);
		mjs_pushvalue(J, *b);
		mjs_call(J, 2);
		v = mjs_tonumber(J, -1);
		c = (v == 0) ? 0 : (v < 0) ? -1 : 1;
		mjs_pop(J, 1);
	} else {
		mjs_pushvalue(J, *a);
		mjs_pushvalue(J, *b);
		sx = mjs_tostring(J, -2);
		sy = mjs_tostring(J, -1);
		c = strcmp(sx, sy);
		mjs_pop(J, 2);
	}
	return c;
}

static void Ap_sort(mjs_State *J)
{
	struct sortslot *array = NULL;
	int i, n, len;

	len = mjs_getlength(J, 0);
	if (len <= 0) {
		mjs_copy(J, 0);
		return;
	}

	if (len >= INT_MAX / (int)sizeof(*array))
		mjs_rangeerror(J, "array is too large to sort");

	array = mjs_malloc(J, len * sizeof *array);

	/* Holding objects where the GC cannot see them is illegal, but if we
	 * don't allow the GC to run we can use qsort() on a temporary array of
	 * mjs_Values for fast sorting.
	 */
	++J->gcpause;

	if (mjs_try(J)) {
		--J->gcpause;
		mjs_free(J, array);
		mjs_throw(J);
	}

	n = 0;
	for (i = 0; i < len; ++i) {
		if (mjs_hasindex(J, 0, i)) {
			array[n].v = *mjs_tovalue(J, -1);
			array[n].J = J;
			mjs_pop(J, 1);
			++n;
		}
	}

	qsort(array, n, sizeof *array, sortcmp);

	for (i = 0; i < n; ++i) {
		mjs_pushvalue(J, array[i].v);
		mjs_setindex(J, 0, i);
	}
	for (i = n; i < len; ++i) {
		mjs_delindex(J, 0, i);
	}

	--J->gcpause;

	mjs_endtry(J);
	mjs_free(J, array);

	mjs_copy(J, 0);
}

static void Ap_splice(mjs_State *J)
{
	int top = mjs_gettop(J);
	int len, start, del, add, k;
	double f;

	mjs_newarray(J);

	len = mjs_getlength(J, 0);

	f = mjs_tointeger(J, 1);
	if (f < 0) f = f + len;
	start = f < 0 ? 0 : f > len ? len : f;

	f = mjs_tointeger(J, 2);
	del = f < 0 ? 0 : f > len - start ? len - start : f;

	/* copy deleted items to return array */
	for (k = 0; k < del; ++k)
		if (mjs_hasindex(J, 0, start + k))
			mjs_setindex(J, -2, k);
	mjs_setlength(J, -1, del);

	/* shift the tail to resize the hole left by deleted items */
	add = top - 3;
	if (add < del) {
		for (k = start; k < len - del; ++k) {
			if (mjs_hasindex(J, 0, k + del))
				mjs_setindex(J, 0, k + add);
			else
				mjs_delindex(J, 0, k + add);
		}
		for (k = len; k > len - del + add; --k)
			mjs_delindex(J, 0, k - 1);
	} else if (add > del) {
		for (k = len - del; k > start; --k) {
			if (mjs_hasindex(J, 0, k + del - 1))
				mjs_setindex(J, 0, k + add - 1);
			else
				mjs_delindex(J, 0, k + add - 1);
		}
	}

	/* copy new items into the hole */
	for (k = 0; k < add; ++k) {
		mjs_copy(J, 3 + k);
		mjs_setindex(J, 0, start + k);
	}

	mjs_setlength(J, 0, len - del + add);
}

static void Ap_unshift(mjs_State *J)
{
	int i, top = mjs_gettop(J);
	int k, len;

	len = mjs_getlength(J, 0);

	for (k = len; k > 0; --k) {
		int from = k - 1;
		int to = k + top - 2;
		if (mjs_hasindex(J, 0, from))
			mjs_setindex(J, 0, to);
		else
			mjs_delindex(J, 0, to);
	}

	for (i = 1; i < top; ++i) {
		mjs_copy(J, i);
		mjs_setindex(J, 0, i - 1);
	}

	mjs_setlength(J, 0, len + top - 1);

	mjs_pushnumber(J, len + top - 1);
}

static void Ap_toString(mjs_State *J)
{
	int top = mjs_gettop(J);
	mjs_pop(J, top - 1);
	Ap_join(J);
}

static void Ap_indexOf(mjs_State *J)
{
	int k, len, from;

	len = mjs_getlength(J, 0);
	from = mjs_isdefined(J, 2) ? mjs_tointeger(J, 2) : 0;
	if (from < 0) from = len + from;
	if (from < 0) from = 0;

	mjs_copy(J, 1);
	for (k = from; k < len; ++k) {
		if (mjs_hasindex(J, 0, k)) {
			if (mjs_strictequal(J)) {
				mjs_pushnumber(J, k);
				return;
			}
			mjs_pop(J, 1);
		}
	}

	mjs_pushnumber(J, -1);
}

static void Ap_lastIndexOf(mjs_State *J)
{
	int k, len, from;

	len = mjs_getlength(J, 0);
	from = mjs_isdefined(J, 2) ? mjs_tointeger(J, 2) : len - 1;
	if (from > len - 1) from = len - 1;
	if (from < 0) from = len + from;

	mjs_copy(J, 1);
	for (k = from; k >= 0; --k) {
		if (mjs_hasindex(J, 0, k)) {
			if (mjs_strictequal(J)) {
				mjs_pushnumber(J, k);
				return;
			}
			mjs_pop(J, 1);
		}
	}

	mjs_pushnumber(J, -1);
}

static void Ap_every(mjs_State *J)
{
	int hasthis = mjs_gettop(J) >= 3;
	int k, len;

	if (!mjs_iscallable(J, 1))
		mjs_typeerror(J, "callback is not a function");

	len = mjs_getlength(J, 0);
	for (k = 0; k < len; ++k) {
		if (mjs_hasindex(J, 0, k)) {
			mjs_copy(J, 1);
			if (hasthis)
				mjs_copy(J, 2);
			else
				mjs_pushundefined(J);
			mjs_copy(J, -3);
			mjs_pushnumber(J, k);
			mjs_copy(J, 0);
			mjs_call(J, 3);
			if (!mjs_toboolean(J, -1))
				return;
			mjs_pop(J, 2);
		}
	}

	mjs_pushboolean(J, 1);
}

static void Ap_some(mjs_State *J)
{
	int hasthis = mjs_gettop(J) >= 3;
	int k, len;

	if (!mjs_iscallable(J, 1))
		mjs_typeerror(J, "callback is not a function");

	len = mjs_getlength(J, 0);
	for (k = 0; k < len; ++k) {
		if (mjs_hasindex(J, 0, k)) {
			mjs_copy(J, 1);
			if (hasthis)
				mjs_copy(J, 2);
			else
				mjs_pushundefined(J);
			mjs_copy(J, -3);
			mjs_pushnumber(J, k);
			mjs_copy(J, 0);
			mjs_call(J, 3);
			if (mjs_toboolean(J, -1))
				return;
			mjs_pop(J, 2);
		}
	}

	mjs_pushboolean(J, 0);
}

static void Ap_forEach(mjs_State *J)
{
	int hasthis = mjs_gettop(J) >= 3;
	int k, len;

	if (!mjs_iscallable(J, 1))
		mjs_typeerror(J, "callback is not a function");

	len = mjs_getlength(J, 0);
	for (k = 0; k < len; ++k) {
		if (mjs_hasindex(J, 0, k)) {
			mjs_copy(J, 1);
			if (hasthis)
				mjs_copy(J, 2);
			else
				mjs_pushundefined(J);
			mjs_copy(J, -3);
			mjs_pushnumber(J, k);
			mjs_copy(J, 0);
			mjs_call(J, 3);
			mjs_pop(J, 2);
		}
	}

	mjs_pushundefined(J);
}

static void Ap_map(mjs_State *J)
{
	int hasthis = mjs_gettop(J) >= 3;
	int k, len;

	if (!mjs_iscallable(J, 1))
		mjs_typeerror(J, "callback is not a function");

	mjs_newarray(J);

	len = mjs_getlength(J, 0);
	for (k = 0; k < len; ++k) {
		if (mjs_hasindex(J, 0, k)) {
			mjs_copy(J, 1);
			if (hasthis)
				mjs_copy(J, 2);
			else
				mjs_pushundefined(J);
			mjs_copy(J, -3);
			mjs_pushnumber(J, k);
			mjs_copy(J, 0);
			mjs_call(J, 3);
			mjs_setindex(J, -3, k);
			mjs_pop(J, 1);
		}
	}
}

static void Ap_filter(mjs_State *J)
{
	int hasthis = mjs_gettop(J) >= 3;
	int k, to, len;

	if (!mjs_iscallable(J, 1))
		mjs_typeerror(J, "callback is not a function");

	mjs_newarray(J);
	to = 0;

	len = mjs_getlength(J, 0);
	for (k = 0; k < len; ++k) {
		if (mjs_hasindex(J, 0, k)) {
			mjs_copy(J, 1);
			if (hasthis)
				mjs_copy(J, 2);
			else
				mjs_pushundefined(J);
			mjs_copy(J, -3);
			mjs_pushnumber(J, k);
			mjs_copy(J, 0);
			mjs_call(J, 3);
			if (mjs_toboolean(J, -1)) {
				mjs_pop(J, 1);
				mjs_setindex(J, -2, to++);
			} else {
				mjs_pop(J, 2);
			}
		}
	}
}

static void Ap_reduce(mjs_State *J)
{
	int hasinitial = mjs_gettop(J) >= 3;
	int k, len;

	if (!mjs_iscallable(J, 1))
		mjs_typeerror(J, "callback is not a function");

	len = mjs_getlength(J, 0);
	k = 0;

	if (len == 0 && !hasinitial)
		mjs_typeerror(J, "no initial value");

	/* initial value of accumulator */
	if (hasinitial)
		mjs_copy(J, 2);
	else {
		while (k < len)
			if (mjs_hasindex(J, 0, k++))
				break;
		if (k == len)
			mjs_typeerror(J, "no initial value");
	}

	while (k < len) {
		if (mjs_hasindex(J, 0, k)) {
			mjs_copy(J, 1);
			mjs_pushundefined(J);
			mjs_rot(J, 4); /* accumulator on top */
			mjs_rot(J, 4); /* property on top */
			mjs_pushnumber(J, k);
			mjs_copy(J, 0);
			mjs_call(J, 4); /* calculate new accumulator */
		}
		++k;
	}

	/* return accumulator */
}

static void Ap_reduceRight(mjs_State *J)
{
	int hasinitial = mjs_gettop(J) >= 3;
	int k, len;

	if (!mjs_iscallable(J, 1))
		mjs_typeerror(J, "callback is not a function");

	len = mjs_getlength(J, 0);
	k = len - 1;

	if (len == 0 && !hasinitial)
		mjs_typeerror(J, "no initial value");

	/* initial value of accumulator */
	if (hasinitial)
		mjs_copy(J, 2);
	else {
		while (k >= 0)
			if (mjs_hasindex(J, 0, k--))
				break;
		if (k < 0)
			mjs_typeerror(J, "no initial value");
	}

	while (k >= 0) {
		if (mjs_hasindex(J, 0, k)) {
			mjs_copy(J, 1);
			mjs_pushundefined(J);
			mjs_rot(J, 4); /* accumulator on top */
			mjs_rot(J, 4); /* property on top */
			mjs_pushnumber(J, k);
			mjs_copy(J, 0);
			mjs_call(J, 4); /* calculate new accumulator */
		}
		--k;
	}

	/* return accumulator */
}

static void A_isArray(mjs_State *J)
{
	if (mjs_isobject(J, 1)) {
		mjs_Object *T = mjs_toobject(J, 1);
		mjs_pushboolean(J, T->type == JS_CARRAY);
	} else {
		mjs_pushboolean(J, 0);
	}
}

void jsB_initarray(mjs_State *J)
{
	mjs_pushobject(J, J->Array_prototype);
	{
		jsB_propf(J, "Array.prototype.toString", Ap_toString, 0);
		jsB_propf(J, "Array.prototype.concat", Ap_concat, 0); /* 1 */
		jsB_propf(J, "Array.prototype.join", Ap_join, 1);
		jsB_propf(J, "Array.prototype.pop", Ap_pop, 0);
		jsB_propf(J, "Array.prototype.push", Ap_push, 0); /* 1 */
		jsB_propf(J, "Array.prototype.reverse", Ap_reverse, 0);
		jsB_propf(J, "Array.prototype.shift", Ap_shift, 0);
		jsB_propf(J, "Array.prototype.slice", Ap_slice, 2);
		jsB_propf(J, "Array.prototype.sort", Ap_sort, 1);
		jsB_propf(J, "Array.prototype.splice", Ap_splice, 0); /* 2 */
		jsB_propf(J, "Array.prototype.unshift", Ap_unshift, 0); /* 1 */

		/* ES5 */
		jsB_propf(J, "Array.prototype.indexOf", Ap_indexOf, 1);
		jsB_propf(J, "Array.prototype.lastIndexOf", Ap_lastIndexOf, 1);
		jsB_propf(J, "Array.prototype.every", Ap_every, 1);
		jsB_propf(J, "Array.prototype.some", Ap_some, 1);
		jsB_propf(J, "Array.prototype.forEach", Ap_forEach, 1);
		jsB_propf(J, "Array.prototype.map", Ap_map, 1);
		jsB_propf(J, "Array.prototype.filter", Ap_filter, 1);
		jsB_propf(J, "Array.prototype.reduce", Ap_reduce, 1);
		jsB_propf(J, "Array.prototype.reduceRight", Ap_reduceRight, 1);
	}
	mjs_newcconstructor(J, jsB_new_Array, jsB_new_Array, "Array", 0); /* 1 */
	{
		/* ES5 */
		jsB_propf(J, "Array.isArray", A_isArray, 1);
	}
	mjs_defglobal(J, "Array", JS_DONTENUM);
}
