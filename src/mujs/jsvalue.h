#ifndef mjs_value_h
#define mjs_value_h

typedef struct mjs_Property mjs_Property;
typedef struct mjs_Iterator mjs_Iterator;

/* Hint to ToPrimitive() */
enum {
	JS_HNONE,
	JS_HNUMBER,
	JS_HSTRING
};

enum mjs_Type {
	JS_TSHRSTR, /* type tag doubles as string zero-terminator */
	JS_TUNDEFINED,
	JS_TNULL,
	JS_TBOOLEAN,
	JS_TNUMBER,
	JS_TLITSTR,
	JS_TMEMSTR,
	JS_TOBJECT,
};

enum mjs_Class {
	JS_COBJECT,
	JS_CARRAY,
	JS_CFUNCTION,
	JS_CSCRIPT, /* function created from global code */
	JS_CEVAL, /* function created from eval code */
	JS_CCFUNCTION, /* built-in function */
	JS_CERROR,
	JS_CBOOLEAN,
	JS_CNUMBER,
	JS_CSTRING,
	JS_CREGEXP,
	JS_CDATE,
	JS_CMATH,
	JS_CJSON,
	JS_CARGUMENTS,
	JS_CITERATOR,
	JS_CUSERDATA,
};

/*
	Short strings abuse the mjs_Value struct. By putting the type tag in the
	last byte, and using 0 as the tag for short strings, we can use the
	entire mjs_Value as string storage by letting the type tag serve double
	purpose as the string zero terminator.
*/

struct mjs_Value
{
	union {
		int boolean;
		double number;
		char shrstr[8];
		const char *litstr;
		mjs_String *memstr;
		mjs_Object *object;
	} u;
	char pad[7]; /* extra storage for shrstr */
	char type; /* type tag and zero terminator for shrstr */
};

struct mjs_String
{
	mjs_String *gcnext;
	char gcmark;
	char p[1];
};

struct mjs_Regexp
{
	void *prog;
	char *source;
	unsigned short flags;
	unsigned short last;
};

struct mjs_Object
{
	enum mjs_Class type;
	int extensible;
	mjs_Property *properties;
	int count; /* number of properties, for array sparseness check */
	mjs_Object *prototype;
	union {
		int boolean;
		double number;
		struct {
			const char *string;
			int length;
		} s;
		struct {
			int length;
		} a;
		struct {
			mjs_Function *function;
			mjs_Environment *scope;
		} f;
		struct {
			const char *name;
			mjs_CFunction function;
			mjs_CFunction constructor;
			int length;
		} c;
		mjs_Regexp r;
		struct {
			mjs_Object *target;
			mjs_Iterator *head;
		} iter;
		struct {
			const char *tag;
			void *data;
			mjs_HasProperty has;
			mjs_Put put;
			mjs_Delete delete;
			mjs_Finalize finalize;
		} user;
	} u;
	mjs_Object *gcnext;
	int gcmark;
};

struct mjs_Property
{
	const char *name;
	mjs_Property *left, *right;
	int level;
	int atts;
	mjs_Value value;
	mjs_Object *getter;
	mjs_Object *setter;
};

struct mjs_Iterator
{
	const char *name;
	mjs_Iterator *next;
};

/* jsrun.c */
mjs_String *jsV_newmemstring(mjs_State *J, const char *s, int n);
mjs_Value *mjs_tovalue(mjs_State *J, int idx);
void mjs_toprimitive(mjs_State *J, int idx, int hint);
mjs_Object *mjs_toobject(mjs_State *J, int idx);
void mjs_pushvalue(mjs_State *J, mjs_Value v);
void mjs_pushobject(mjs_State *J, mjs_Object *v);

/* jsvalue.c */
int jsV_toboolean(mjs_State *J, mjs_Value *v);
double jsV_tonumber(mjs_State *J, mjs_Value *v);
double jsV_tointeger(mjs_State *J, mjs_Value *v);
const char *jsV_tostring(mjs_State *J, mjs_Value *v);
mjs_Object *jsV_toobject(mjs_State *J, mjs_Value *v);
void jsV_toprimitive(mjs_State *J, mjs_Value *v, int preferred);

const char *mjs_itoa(char buf[32], int a);
double mjs_stringtofloat(const char *s, char **ep);
int jsV_numbertointeger(double n);
int jsV_numbertoint32(double n);
unsigned int jsV_numbertouint32(double n);
short jsV_numbertoint16(double n);
unsigned short jsV_numbertouint16(double n);
const char *jsV_numbertostring(mjs_State *J, char buf[32], double number);
double jsV_stringtonumber(mjs_State *J, const char *string);

/* jsproperty.c */
mjs_Object *jsV_newobject(mjs_State *J, enum mjs_Class type, mjs_Object *prototype);
mjs_Property *jsV_getownproperty(mjs_State *J, mjs_Object *obj, const char *name);
mjs_Property *jsV_getpropertyx(mjs_State *J, mjs_Object *obj, const char *name, int *own);
mjs_Property *jsV_getproperty(mjs_State *J, mjs_Object *obj, const char *name);
mjs_Property *jsV_setproperty(mjs_State *J, mjs_Object *obj, const char *name);
mjs_Property *jsV_nextproperty(mjs_State *J, mjs_Object *obj, const char *name);
void jsV_delproperty(mjs_State *J, mjs_Object *obj, const char *name);

mjs_Object *jsV_newiterator(mjs_State *J, mjs_Object *obj, int own);
const char *jsV_nextiterator(mjs_State *J, mjs_Object *iter);

void jsV_resizearray(mjs_State *J, mjs_Object *obj, int newlen);

/* jsdump.c */
void mjs_dumpobject(mjs_State *J, mjs_Object *obj);
void mjs_dumpvalue(mjs_State *J, mjs_Value v);

#endif
