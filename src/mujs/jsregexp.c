#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"
#include "regexp.h"

void mjs_newregexp(mjs_State *J, const char *pattern, int flags)
{
	const char *error;
	mjs_Object *obj;
	Reprog *prog;
	int opts;

	obj = jsV_newobject(J, JS_CREGEXP, J->RegExp_prototype);

	opts = 0;
	if (flags & JS_REGEXP_I) opts |= REG_ICASE;
	if (flags & JS_REGEXP_M) opts |= REG_NEWLINE;

	prog = mjs_regcompx(J->alloc, J->actx, pattern, opts, &error);
	if (!prog)
		mjs_syntaxerror(J, "regular expression: %s", error);

	obj->u.r.prog = prog;
	obj->u.r.source = mjs_strdup(J, pattern);
	obj->u.r.flags = flags;
	obj->u.r.last = 0;
	mjs_pushobject(J, obj);
}

void mjs_RegExp_prototype_exec(mjs_State *J, mjs_Regexp *re, const char *text)
{
	int result;
	int i;
	int opts;
	Resub m;

	opts = 0;
	if (re->flags & JS_REGEXP_G) {
		if (re->last > strlen(text)) {
			re->last = 0;
			mjs_pushnull(J);
			return;
		}
		if (re->last > 0) {
			text += re->last;
			opts |= REG_NOTBOL;
		}
	}

	result = mjs_regexec(re->prog, text, &m, opts);
	if (result < 0)
		mjs_error(J, "regexec failed");
	if (result == 0) {
		mjs_newarray(J);
		mjs_pushstring(J, text);
		mjs_setproperty(J, -2, "input");
		mjs_pushnumber(J, mjs_utfptrtoidx(text, m.sub[0].sp));
		mjs_setproperty(J, -2, "index");
		for (i = 0; i < m.nsub; ++i) {
			mjs_pushlstring(J, m.sub[i].sp, m.sub[i].ep - m.sub[i].sp);
			mjs_setindex(J, -2, i);
		}
		if (re->flags & JS_REGEXP_G)
			re->last = re->last + (m.sub[0].ep - text);
		return;
	}

	if (re->flags & JS_REGEXP_G)
		re->last = 0;

	mjs_pushnull(J);
}

static void Rp_test(mjs_State *J)
{
	mjs_Regexp *re;
	const char *text;
	int result;
	int opts;
	Resub m;

	re = mjs_toregexp(J, 0);
	text = mjs_tostring(J, 1);

	opts = 0;
	if (re->flags & JS_REGEXP_G) {
		if (re->last > strlen(text)) {
			re->last = 0;
			mjs_pushboolean(J, 0);
			return;
		}
		if (re->last > 0) {
			text += re->last;
			opts |= REG_NOTBOL;
		}
	}

	result = mjs_regexec(re->prog, text, &m, opts);
	if (result < 0)
		mjs_error(J, "regexec failed");
	if (result == 0) {
		if (re->flags & JS_REGEXP_G)
			re->last = re->last + (m.sub[0].ep - text);
		mjs_pushboolean(J, 1);
		return;
	}

	if (re->flags & JS_REGEXP_G)
		re->last = 0;

	mjs_pushboolean(J, 0);
}

static void jsB_new_RegExp(mjs_State *J)
{
	mjs_Regexp *old;
	const char *pattern;
	int flags;

	if (mjs_isregexp(J, 1)) {
		if (mjs_isdefined(J, 2))
			mjs_typeerror(J, "cannot supply flags when creating one RegExp from another");
		old = mjs_toregexp(J, 1);
		pattern = old->source;
		flags = old->flags;
	} else if (mjs_isundefined(J, 1)) {
		pattern = "(?:)";
		flags = 0;
	} else {
		pattern = mjs_tostring(J, 1);
		flags = 0;
	}

	if (strlen(pattern) == 0)
		pattern = "(?:)";

	if (mjs_isdefined(J, 2)) {
		const char *s = mjs_tostring(J, 2);
		int g = 0, i = 0, m = 0;
		while (*s) {
			if (*s == 'g') ++g;
			else if (*s == 'i') ++i;
			else if (*s == 'm') ++m;
			else mjs_syntaxerror(J, "invalid regular expression flag: '%c'", *s);
			++s;
		}
		if (g > 1) mjs_syntaxerror(J, "invalid regular expression flag: 'g'");
		if (i > 1) mjs_syntaxerror(J, "invalid regular expression flag: 'i'");
		if (m > 1) mjs_syntaxerror(J, "invalid regular expression flag: 'm'");
		if (g) flags |= JS_REGEXP_G;
		if (i) flags |= JS_REGEXP_I;
		if (m) flags |= JS_REGEXP_M;
	}

	mjs_newregexp(J, pattern, flags);
}

static void jsB_RegExp(mjs_State *J)
{
	if (mjs_isregexp(J, 1))
		return;
	jsB_new_RegExp(J);
}

static void Rp_toString(mjs_State *J)
{
	mjs_Regexp *re;
	char *out;

	re = mjs_toregexp(J, 0);

	out = mjs_malloc(J, strlen(re->source) + 6); /* extra space for //gim */
	strcpy(out, "/");
	strcat(out, re->source);
	strcat(out, "/");
	if (re->flags & JS_REGEXP_G) strcat(out, "g");
	if (re->flags & JS_REGEXP_I) strcat(out, "i");
	if (re->flags & JS_REGEXP_M) strcat(out, "m");

	if (mjs_try(J)) {
		mjs_free(J, out);
		mjs_throw(J);
	}
	mjs_pop(J, 0);
	mjs_pushstring(J, out);
	mjs_endtry(J);
	mjs_free(J, out);
}

static void Rp_exec(mjs_State *J)
{
	mjs_RegExp_prototype_exec(J, mjs_toregexp(J, 0), mjs_tostring(J, 1));
}

void jsB_initregexp(mjs_State *J)
{
	mjs_pushobject(J, J->RegExp_prototype);
	{
		jsB_propf(J, "RegExp.prototype.toString", Rp_toString, 0);
		jsB_propf(J, "RegExp.prototype.test", Rp_test, 0);
		jsB_propf(J, "RegExp.prototype.exec", Rp_exec, 0);
	}
	mjs_newcconstructor(J, jsB_RegExp, jsB_new_RegExp, "RegExp", 1);
	mjs_defglobal(J, "RegExp", JS_DONTENUM);
}
