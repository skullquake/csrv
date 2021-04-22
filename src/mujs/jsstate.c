#include "jsi.h"
#include "jsparse.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsrun.h"
#include "jsbuiltin.h"

#include <assert.h>
#include <errno.h>

static void *mjs_defaultalloc(void *actx, void *ptr, int size)
{
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
	if (size == 0) {
		free(ptr);
		return NULL;
	}
#endif
	return realloc(ptr, (size_t)size);
}

static void mjs_defaultreport(mjs_State *J, const char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
}

static void mjs_defaultpanic(mjs_State *J)
{
	mjs_report(J, "uncaught exception");
	/* return to javascript to abort */
}

int mjs_ploadstring(mjs_State *J, const char *filename, const char *source)
{
	if (mjs_try(J))
		return 1;
	mjs_loadstring(J, filename, source);
	mjs_endtry(J);
	return 0;
}

int mjs_ploadfile(mjs_State *J, const char *filename)
{
	if (mjs_try(J))
		return 1;
	mjs_loadfile(J, filename);
	mjs_endtry(J);
	return 0;
}

const char *mjs_trystring(mjs_State *J, int idx, const char *error)
{
	const char *s;
	if (mjs_try(J)) {
		mjs_pop(J, 1);
		return error;
	}
	s = mjs_tostring(J, idx);
	mjs_endtry(J);
	return s;
}

double mjs_trynumber(mjs_State *J, int idx, double error)
{
	double v;
	if (mjs_try(J)) {
		mjs_pop(J, 1);
		return error;
	}
	v = mjs_tonumber(J, idx);
	mjs_endtry(J);
	return v;
}

int mjs_tryinteger(mjs_State *J, int idx, int error)
{
	int v;
	if (mjs_try(J)) {
		mjs_pop(J, 1);
		return error;
	}
	v = mjs_tointeger(J, idx);
	mjs_endtry(J);
	return v;
}

int mjs_tryboolean(mjs_State *J, int idx, int error)
{
	int v;
	if (mjs_try(J)) {
		mjs_pop(J, 1);
		return error;
	}
	v = mjs_toboolean(J, idx);
	mjs_endtry(J);
	return v;
}

static void mjs_loadstringx(mjs_State *J, const char *filename, const char *source, int iseval)
{
	mjs_Ast *P;
	mjs_Function *F;

	if (mjs_try(J)) {
		jsP_freeparse(J);
		mjs_throw(J);
	}

	P = jsP_parse(J, filename, source);
	F = jsC_compilescript(J, P, iseval ? J->strict : J->default_strict);
	jsP_freeparse(J);
	mjs_newscript(J, F, iseval ? (J->strict ? J->E : NULL) : J->GE, iseval ? JS_CEVAL : JS_CSCRIPT);

	mjs_endtry(J);
}

void mjs_loadeval(mjs_State *J, const char *filename, const char *source)
{
	mjs_loadstringx(J, filename, source, 1);
}

void mjs_loadstring(mjs_State *J, const char *filename, const char *source)
{
	mjs_loadstringx(J, filename, source, 0);
}

void mjs_loadfile(mjs_State *J, const char *filename)
{
	FILE *f;
	char *s, *p;
	int n, t;

	f = fopen(filename, "rb");
	if (!f) {
		mjs_error(J, "cannot open file '%s': %s", filename, strerror(errno));
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fclose(f);
		mjs_error(J, "cannot seek in file '%s': %s", filename, strerror(errno));
	}

	n = ftell(f);
	if (n < 0) {
		fclose(f);
		mjs_error(J, "cannot tell in file '%s': %s", filename, strerror(errno));
	}

	if (fseek(f, 0, SEEK_SET) < 0) {
		fclose(f);
		mjs_error(J, "cannot seek in file '%s': %s", filename, strerror(errno));
	}

	if (mjs_try(J)) {
		fclose(f);
		mjs_throw(J);
	}
	s = mjs_malloc(J, n + 1); /* add space for string terminator */
	mjs_endtry(J);

	t = fread(s, 1, (size_t)n, f);
	if (t != n) {
		mjs_free(J, s);
		fclose(f);
		mjs_error(J, "cannot read data from file '%s': %s", filename, strerror(errno));
	}

	s[n] = 0; /* zero-terminate string containing file data */

	if (mjs_try(J)) {
		mjs_free(J, s);
		fclose(f);
		mjs_throw(J);
	}

	/* skip first line if it starts with "#!" */
	p = s;
	if (p[0] == '#' && p[1] == '!') {
		p += 2;
		while (*p && *p != '\n')
			++p;
	}

	mjs_loadstring(J, filename, p);

	mjs_free(J, s);
	fclose(f);
	mjs_endtry(J);
}

int mjs_dostring(mjs_State *J, const char *source)
{
	if (mjs_try(J)) {
		mjs_report(J, mjs_trystring(J, -1, "Error"));
		mjs_pop(J, 1);
		return 1;
	}
	mjs_loadstring(J, "[string]", source);
	mjs_pushundefined(J);
	mjs_call(J, 0);
	mjs_pop(J, 1);
	mjs_endtry(J);
	return 0;
}

int mjs_dofile(mjs_State *J, const char *filename)
{
	if (mjs_try(J)) {
		mjs_report(J, mjs_trystring(J, -1, "Error"));
		mjs_pop(J, 1);
		return 1;
	}
	mjs_loadfile(J, filename);
	mjs_pushundefined(J);
	mjs_call(J, 0);
	mjs_pop(J, 1);
	mjs_endtry(J);
	return 0;
}

mjs_Panic mjs_atpanic(mjs_State *J, mjs_Panic panic)
{
	mjs_Panic old = J->panic;
	J->panic = panic;
	return old;
}

void mjs_report(mjs_State *J, const char *message)
{
	if (J->report)
		J->report(J, message);
}

void mjs_setreport(mjs_State *J, mjs_Report report)
{
	J->report = report;
}

void mjs_setcontext(mjs_State *J, void *uctx)
{
	J->uctx = uctx;
}

void *mjs_getcontext(mjs_State *J)
{
	return J->uctx;
}

mjs_State *mjs_newstate(mjs_Alloc alloc, void *actx, int flags)
{
	mjs_State *J;

	assert(sizeof(mjs_Value) == 16);
	assert(soffsetof(mjs_Value, type) == 15);

	if (!alloc)
		alloc = mjs_defaultalloc;

	J = alloc(actx, NULL, sizeof *J);
	if (!J)
		return NULL;
	memset(J, 0, sizeof(*J));
	J->actx = actx;
	J->alloc = alloc;

	if (flags & JS_STRICT)
		J->strict = J->default_strict = 1;

	J->trace[0].name = "-top-";
	J->trace[0].file = "native";
	J->trace[0].line = 0;

	J->report = mjs_defaultreport;
	J->panic = mjs_defaultpanic;

	J->stack = alloc(actx, NULL, JS_STACKSIZE * sizeof *J->stack);
	if (!J->stack) {
		alloc(actx, NULL, 0);
		return NULL;
	}

	J->gcmark = 1;
	J->nextref = 0;

	J->R = jsV_newobject(J, JS_COBJECT, NULL);
	J->G = jsV_newobject(J, JS_COBJECT, NULL);
	J->E = jsR_newenvironment(J, J->G, NULL);
	J->GE = J->E;

	jsB_init(J);

	return J;
}
