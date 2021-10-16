#ifndef jsi_h
#define jsi_h

#include "mujs.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include <math.h>
#include <float.h>
#include <limits.h>

/* Microsoft Visual C */
#ifdef _MSC_VER
#pragma warning(disable:4996) /* _CRT_SECURE_NO_WARNINGS */
#pragma warning(disable:4244) /* implicit conversion from double to int */
#pragma warning(disable:4267) /* implicit conversion of int to smaller int */
#define inline __inline
#if _MSC_VER < 1900 /* MSVC 2015 */
#define snprintf jsW_snprintf
#define vsnprintf jsW_vsnprintf
static int jsW_vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	int n;
	n = _vsnprintf(str, size, fmt, ap);
	str[size-1] = 0;
	return n;
}
static int jsW_snprintf(char *str, size_t size, const char *fmt, ...)
{
	int n;
	va_list ap;
	va_start(ap, fmt);
	n = jsW_vsnprintf(str, size, fmt, ap);
	va_end(ap);
	return n;
}
#endif
#if _MSC_VER <= 1700 /* <= MSVC 2012 */
#define isnan(x) _isnan(x)
#define isinf(x) (!_finite(x))
#define isfinite(x) _finite(x)
static __inline int signbit(double x) { __int64 i; memcpy(&i, &x, 8); return i>>63; }
#define INFINITY (DBL_MAX+DBL_MAX)
#define NAN (INFINITY-INFINITY)
#endif
#endif

#define soffsetof(x,y) ((int)offsetof(x,y))
#define nelem(a) (int)(sizeof (a) / sizeof (a)[0])

void *mjs_malloc(mjs_State *J, int size);
void *mjs_realloc(mjs_State *J, void *ptr, int size);
void mjs_free(mjs_State *J, void *ptr);

typedef struct mjs_Regexp mjs_Regexp;
typedef struct mjs_Value mjs_Value;
typedef struct mjs_Object mjs_Object;
typedef struct mjs_String mjs_String;
typedef struct mjs_Ast mjs_Ast;
typedef struct mjs_Function mjs_Function;
typedef struct mjs_Environment mjs_Environment;
typedef struct mjs_StringNode mjs_StringNode;
typedef struct mjs_Jumpbuf mjs_Jumpbuf;
typedef struct mjs_StackTrace mjs_StackTrace;

/* Limits */

#ifndef JS_STACKSIZE
#define JS_STACKSIZE 256	/* value stack size */
#endif
#ifndef JS_ENVLIMIT
#define JS_ENVLIMIT 128		/* environment stack size */
#endif
#ifndef JS_TRYLIMIT
#define JS_TRYLIMIT 64		/* exception stack size */
#endif
#ifndef JS_GCLIMIT
#define JS_GCLIMIT 10000	/* run gc cycle every N allocations */
#endif
#ifndef JS_ASTLIMIT
#define JS_ASTLIMIT 100		/* max nested expressions */
#endif

/* instruction size -- change to int if you get integer overflow syntax errors */

#ifdef JS_INSTRUCTION
typedef JS_INSTRUCTION mjs_Instruction;
#else
typedef unsigned short mjs_Instruction;
#endif

/* String interning */

char *mjs_strdup(mjs_State *J, const char *s);
const char *mjs_intern(mjs_State *J, const char *s);
void jsS_dumpstrings(mjs_State *J);
void jsS_freestrings(mjs_State *J);

/* Portable strtod and printf float formatting */

void mjs_fmtexp(char *p, int e);
int mjs_grisu2(double v, char *buffer, int *K);
double mjs_strtod(const char *as, char **aas);

/* Private stack functions */

void mjs_newarguments(mjs_State *J);
void mjs_newfunction(mjs_State *J, mjs_Function *function, mjs_Environment *scope);
void mjs_newscript(mjs_State *J, mjs_Function *fun, mjs_Environment *scope, int type);
void mjs_loadeval(mjs_State *J, const char *filename, const char *source);

mjs_Regexp *mjs_toregexp(mjs_State *J, int idx);
int mjs_isarrayindex(mjs_State *J, const char *str, int *idx);
int mjs_runeat(mjs_State *J, const char *s, int i);
int mjs_utfptrtoidx(const char *s, const char *p);
const char *mjs_utfidxtoptr(const char *s, int i);

void mjs_dup(mjs_State *J);
void mjs_dup2(mjs_State *J);
void mjs_rot2(mjs_State *J);
void mjs_rot3(mjs_State *J);
void mjs_rot4(mjs_State *J);
void mjs_rot2pop1(mjs_State *J);
void mjs_rot3pop2(mjs_State *J);
void mjs_dup1rot3(mjs_State *J);
void mjs_dup1rot4(mjs_State *J);

void mjs_RegExp_prototype_exec(mjs_State *J, mjs_Regexp *re, const char *text);

void mjs_trap(mjs_State *J, int pc); /* dump stack and environment to stdout */

struct mjs_StackTrace
{
	const char *name;
	const char *file;
	int line;
};

/* Exception handling */

struct mjs_Jumpbuf
{
	jmp_buf buf;
	mjs_Environment *E;
	int envtop;
	int tracetop;
	int top, bot;
	int strict;
	mjs_Instruction *pc;
};

void *mjs_savetrypc(mjs_State *J, mjs_Instruction *pc);

#define mjs_trypc(J, PC) \
	setjmp(mjs_savetrypc(J, PC))

/* String buffer */

typedef struct mjs_Buffer { int n, m; char s[64]; } mjs_Buffer;

void mjs_putc(mjs_State *J, mjs_Buffer **sbp, int c);
void mjs_puts(mjs_State *J, mjs_Buffer **sb, const char *s);
void mjs_putm(mjs_State *J, mjs_Buffer **sb, const char *s, const char *e);

/* State struct */

struct mjs_State
{
	void *actx;
	void *uctx;
	mjs_Alloc alloc;
	mjs_Report report;
	mjs_Panic panic;

	mjs_StringNode *strings;

	int default_strict;
	int strict;

	/* parser input source */
	const char *filename;
	const char *source;
	int line;

	/* lexer state */
	struct { char *text; int len, cap; } lexbuf;
	int lexline;
	int lexchar;
	int lasttoken;
	int newline;

	/* parser state */
	int astdepth;
	int lookahead;
	const char *text;
	double number;
	mjs_Ast *gcast; /* list of allocated nodes to free after parsing */

	/* runtime environment */
	mjs_Object *Object_prototype;
	mjs_Object *Array_prototype;
	mjs_Object *Function_prototype;
	mjs_Object *Boolean_prototype;
	mjs_Object *Number_prototype;
	mjs_Object *String_prototype;
	mjs_Object *RegExp_prototype;
	mjs_Object *Date_prototype;

	mjs_Object *Error_prototype;
	mjs_Object *EvalError_prototype;
	mjs_Object *RangeError_prototype;
	mjs_Object *ReferenceError_prototype;
	mjs_Object *SyntaxError_prototype;
	mjs_Object *TypeError_prototype;
	mjs_Object *URIError_prototype;

	unsigned int seed; /* Math.random seed */

	int nextref; /* for mjs_ref use */
	mjs_Object *R; /* registry of hidden values */
	mjs_Object *G; /* the global object */
	mjs_Environment *E; /* current environment scope */
	mjs_Environment *GE; /* global environment scope (at the root) */

	/* execution stack */
	int top, bot;
	mjs_Value *stack;

	/* garbage collector list */
	int gcpause;
	int gcmark;
	int gccounter;
	mjs_Environment *gcenv;
	mjs_Function *gcfun;
	mjs_Object *gcobj;
	mjs_String *gcstr;

	/* environments on the call stack but currently not in scope */
	int envtop;
	mjs_Environment *envstack[JS_ENVLIMIT];

	/* debug info stack trace */
	int tracetop;
	mjs_StackTrace trace[JS_ENVLIMIT];

	/* exception stack */
	int trytop;
	mjs_Jumpbuf trybuf[JS_TRYLIMIT];
};

#endif
