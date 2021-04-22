#ifndef mumjs_h
#define mumjs_h

#include <setjmp.h> /* required for setjmp in fz_try macro */

#ifdef __cplusplus
extern "C" {
#endif

/* noreturn is a GCC extension */
#ifdef __GNUC__
#define JS_NORETURN __attribute__((noreturn))
#else
#ifdef _MSC_VER
#define JS_NORETURN __declspec(noreturn)
#else
#define JS_NORETURN
#endif
#endif

/* GCC can do type checking of printf strings */
#ifdef __printflike
#define JS_PRINTFLIKE __printflike
#else
#if __GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ >= 7
#define JS_PRINTFLIKE(fmtarg, firstvararg) \
	__attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#else
#define JS_PRINTFLIKE(fmtarg, firstvararg)
#endif
#endif

typedef struct mjs_State mjs_State;

typedef void *(*mjs_Alloc)(void *memctx, void *ptr, int size);
typedef void (*mjs_Panic)(mjs_State *J);
typedef void (*mjs_CFunction)(mjs_State *J);
typedef void (*mjs_Finalize)(mjs_State *J, void *p);
typedef int (*mjs_HasProperty)(mjs_State *J, void *p, const char *name);
typedef int (*mjs_Put)(mjs_State *J, void *p, const char *name);
typedef int (*mjs_Delete)(mjs_State *J, void *p, const char *name);
typedef void (*mjs_Report)(mjs_State *J, const char *message);

/* Basic functions */
mjs_State *mjs_newstate(mjs_Alloc alloc, void *actx, int flags);
void mjs_setcontext(mjs_State *J, void *uctx);
void *mjs_getcontext(mjs_State *J);
void mjs_setreport(mjs_State *J, mjs_Report report);
mjs_Panic mjs_atpanic(mjs_State *J, mjs_Panic panic);
void mjs_freestate(mjs_State *J);
void mjs_gc(mjs_State *J, int report);

int mjs_dostring(mjs_State *J, const char *source);
int mjs_dofile(mjs_State *J, const char *filename);
int mjs_ploadstring(mjs_State *J, const char *filename, const char *source);
int mjs_ploadfile(mjs_State *J, const char *filename);
int mjs_pcall(mjs_State *J, int n);
int mjs_pconstruct(mjs_State *J, int n);

/* Exception handling */

void *mjs_savetry(mjs_State *J); /* returns a jmp_buf */

#define mjs_try(J) \
	setjmp(mjs_savetry(J))

void mjs_endtry(mjs_State *J);

/* State constructor flags */
enum {
	JS_STRICT = 1,
};

/* RegExp flags */
enum {
	JS_REGEXP_G = 1,
	JS_REGEXP_I = 2,
	JS_REGEXP_M = 4,
};

/* Property attribute flags */
enum {
	JS_READONLY = 1,
	JS_DONTENUM = 2,
	JS_DONTCONF = 4,
};

void mjs_report(mjs_State *J, const char *message);

void mjs_newerror(mjs_State *J, const char *message);
void mjs_newevalerror(mjs_State *J, const char *message);
void mjs_newrangeerror(mjs_State *J, const char *message);
void mjs_newreferenceerror(mjs_State *J, const char *message);
void mjs_newsyntaxerror(mjs_State *J, const char *message);
void mjs_newtypeerror(mjs_State *J, const char *message);
void mjs_newurierror(mjs_State *J, const char *message);

JS_NORETURN void mjs_error(mjs_State *J, const char *fmt, ...) JS_PRINTFLIKE(2,3);
JS_NORETURN void mjs_evalerror(mjs_State *J, const char *fmt, ...) JS_PRINTFLIKE(2,3);
JS_NORETURN void mjs_rangeerror(mjs_State *J, const char *fmt, ...) JS_PRINTFLIKE(2,3);
JS_NORETURN void mjs_referenceerror(mjs_State *J, const char *fmt, ...) JS_PRINTFLIKE(2,3);
JS_NORETURN void mjs_syntaxerror(mjs_State *J, const char *fmt, ...) JS_PRINTFLIKE(2,3);
JS_NORETURN void mjs_typeerror(mjs_State *J, const char *fmt, ...) JS_PRINTFLIKE(2,3);
JS_NORETURN void mjs_urierror(mjs_State *J, const char *fmt, ...) JS_PRINTFLIKE(2,3);
JS_NORETURN void mjs_throw(mjs_State *J);

void mjs_loadstring(mjs_State *J, const char *filename, const char *source);
void mjs_loadfile(mjs_State *J, const char *filename);

void mjs_eval(mjs_State *J);
void mjs_call(mjs_State *J, int n);
void mjs_construct(mjs_State *J, int n);

const char *mjs_ref(mjs_State *J);
void mjs_unref(mjs_State *J, const char *ref);

void mjs_getregistry(mjs_State *J, const char *name);
void mjs_setregistry(mjs_State *J, const char *name);
void mjs_delregistry(mjs_State *J, const char *name);

void mjs_getglobal(mjs_State *J, const char *name);
void mjs_setglobal(mjs_State *J, const char *name);
void mjs_defglobal(mjs_State *J, const char *name, int atts);
void mjs_delglobal(mjs_State *J, const char *name);

int mjs_hasproperty(mjs_State *J, int idx, const char *name);
void mjs_getproperty(mjs_State *J, int idx, const char *name);
void mjs_setproperty(mjs_State *J, int idx, const char *name);
void mjs_defproperty(mjs_State *J, int idx, const char *name, int atts);
void mjs_delproperty(mjs_State *J, int idx, const char *name);
void mjs_defaccessor(mjs_State *J, int idx, const char *name, int atts);

int mjs_getlength(mjs_State *J, int idx);
void mjs_setlength(mjs_State *J, int idx, int len);
int mjs_hasindex(mjs_State *J, int idx, int i);
void mjs_getindex(mjs_State *J, int idx, int i);
void mjs_setindex(mjs_State *J, int idx, int i);
void mjs_delindex(mjs_State *J, int idx, int i);

void mjs_currentfunction(mjs_State *J);
void mjs_pushglobal(mjs_State *J);
void mjs_pushundefined(mjs_State *J);
void mjs_pushnull(mjs_State *J);
void mjs_pushboolean(mjs_State *J, int v);
void mjs_pushnumber(mjs_State *J, double v);
void mjs_pushstring(mjs_State *J, const char *v);
void mjs_pushlstring(mjs_State *J, const char *v, int n);
void mjs_pushliteral(mjs_State *J, const char *v);

void mjs_newobjectx(mjs_State *J);
void mjs_newobject(mjs_State *J);
void mjs_newarray(mjs_State *J);
void mjs_newboolean(mjs_State *J, int v);
void mjs_newnumber(mjs_State *J, double v);
void mjs_newstring(mjs_State *J, const char *v);
void mjs_newcfunction(mjs_State *J, mjs_CFunction fun, const char *name, int length);
void mjs_newcconstructor(mjs_State *J, mjs_CFunction fun, mjs_CFunction con, const char *name, int length);
void mjs_newuserdata(mjs_State *J, const char *tag, void *data, mjs_Finalize finalize);
void mjs_newuserdatax(mjs_State *J, const char *tag, void *data, mjs_HasProperty has, mjs_Put put, mjs_Delete del, mjs_Finalize finalize);
void mjs_newregexp(mjs_State *J, const char *pattern, int flags);

void mjs_pushiterator(mjs_State *J, int idx, int own);
const char *mjs_nextiterator(mjs_State *J, int idx);

int mjs_isdefined(mjs_State *J, int idx);
int mjs_isundefined(mjs_State *J, int idx);
int mjs_isnull(mjs_State *J, int idx);
int mjs_isboolean(mjs_State *J, int idx);
int mjs_isnumber(mjs_State *J, int idx);
int mjs_isstring(mjs_State *J, int idx);
int mjs_isprimitive(mjs_State *J, int idx);
int mjs_isobject(mjs_State *J, int idx);
int mjs_isarray(mjs_State *J, int idx);
int mjs_isregexp(mjs_State *J, int idx);
int mjs_iscoercible(mjs_State *J, int idx);
int mjs_iscallable(mjs_State *J, int idx);
int mjs_isuserdata(mjs_State *J, int idx, const char *tag);
int mjs_iserror(mjs_State *J, int idx);
int mjs_isnumberobject(mjs_State *J, int idx);
int mjs_isstringobject(mjs_State *J, int idx);

int mjs_toboolean(mjs_State *J, int idx);
double mjs_tonumber(mjs_State *J, int idx);
const char *mjs_tostring(mjs_State *J, int idx);
void *mjs_touserdata(mjs_State *J, int idx, const char *tag);

const char *mjs_trystring(mjs_State *J, int idx, const char *error);
double mjs_trynumber(mjs_State *J, int idx, double error);
int mjs_tryinteger(mjs_State *J, int idx, int error);
int mjs_tryboolean(mjs_State *J, int idx, int error);

int mjs_tointeger(mjs_State *J, int idx);
int mjs_toint32(mjs_State *J, int idx);
unsigned int mjs_touint32(mjs_State *J, int idx);
short mjs_toint16(mjs_State *J, int idx);
unsigned short mjs_touint16(mjs_State *J, int idx);

int mjs_gettop(mjs_State *J);
void mjs_pop(mjs_State *J, int n);
void mjs_rot(mjs_State *J, int n);
void mjs_copy(mjs_State *J, int idx);
void mjs_remove(mjs_State *J, int idx);
void mjs_insert(mjs_State *J, int idx);
void mjs_replace(mjs_State* J, int idx);

void mjs_dup(mjs_State *J);
void mjs_dup2(mjs_State *J);
void mjs_rot2(mjs_State *J);
void mjs_rot3(mjs_State *J);
void mjs_rot4(mjs_State *J);
void mjs_rot2pop1(mjs_State *J);
void mjs_rot3pop2(mjs_State *J);

void mjs_concat(mjs_State *J);
int mjs_compare(mjs_State *J, int *okay);
int mjs_equal(mjs_State *J);
int mjs_strictequal(mjs_State *J);
int mjs_instanceof(mjs_State *J);
const char *mjs_typeof(mjs_State *J, int idx);

void mjs_repr(mjs_State *J, int idx);
const char *mjs_torepr(mjs_State *J, int idx);
const char *mjs_tryrepr(mjs_State *J, int idx, const char *error);

#ifdef __cplusplus
}
#endif

#endif
