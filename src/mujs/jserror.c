#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

#define QQ(X) #X
#define Q(X) QQ(X)

static int jsB_stacktrace(mjs_State *J, int skip)
{
	char buf[256];
	int n = J->tracetop - skip;
	if (n <= 0)
		return 0;
	for (; n > 0; --n) {
		const char *name = J->trace[n].name;
		const char *file = J->trace[n].file;
		int line = J->trace[n].line;
		if (line > 0) {
			if (name[0])
				snprintf(buf, sizeof buf, "\n\tat %s (%s:%d)", name, file, line);
			else
				snprintf(buf, sizeof buf, "\n\tat %s:%d", file, line);
		} else
			snprintf(buf, sizeof buf, "\n\tat %s (%s)", name, file);
		mjs_pushstring(J, buf);
		if (n < J->tracetop - skip)
			mjs_concat(J);
	}
	return 1;
}

static void Ep_toString(mjs_State *J)
{
	const char *name = "Error";
	const char *message = "";

	if (!mjs_isobject(J, -1))
		mjs_typeerror(J, "not an object");

	if (mjs_hasproperty(J, 0, "name"))
		name = mjs_tostring(J, -1);
	if (mjs_hasproperty(J, 0, "message"))
		message = mjs_tostring(J, -1);

	if (name[0] == 0)
		mjs_pushstring(J, message);
	else if (message[0] == 0)
		mjs_pushstring(J, name);
	else {
		mjs_pushstring(J, name);
		mjs_pushstring(J, ": ");
		mjs_concat(J);
		mjs_pushstring(J, message);
		mjs_concat(J);
	}
}

static int jsB_ErrorX(mjs_State *J, mjs_Object *prototype)
{
	int top = mjs_gettop(J);
	mjs_pushobject(J, jsV_newobject(J, JS_CERROR, prototype));
	if (top > 1) {
		mjs_pushstring(J, mjs_tostring(J, 1));
		mjs_defproperty(J, -2, "message", JS_DONTENUM);
	}
	if (jsB_stacktrace(J, 1))
		mjs_defproperty(J, -2, "stackTrace", JS_DONTENUM);
	return 1;
}

static void mjs_newerrorx(mjs_State *J, const char *message, mjs_Object *prototype)
{
	mjs_pushobject(J, jsV_newobject(J, JS_CERROR, prototype));
	mjs_pushstring(J, message);
	mjs_setproperty(J, -2, "message");
	if (jsB_stacktrace(J, 0))
		mjs_setproperty(J, -2, "stackTrace");
}

#define DERROR(name, Name) \
	static void jsB_##Name(mjs_State *J) { \
		jsB_ErrorX(J, J->Name##_prototype); \
	} \
	void mjs_new##name(mjs_State *J, const char *s) { \
		mjs_newerrorx(J, s, J->Name##_prototype); \
	} \
	void mjs_##name(mjs_State *J, const char *fmt, ...) { \
		va_list ap; \
		char buf[256]; \
		va_start(ap, fmt); \
		vsnprintf(buf, sizeof buf, fmt, ap); \
		va_end(ap); \
		mjs_newerrorx(J, buf, J->Name##_prototype); \
		mjs_throw(J); \
	}

DERROR(error, Error)
DERROR(evalerror, EvalError)
DERROR(rangeerror, RangeError)
DERROR(referenceerror, ReferenceError)
DERROR(syntaxerror, SyntaxError)
DERROR(typeerror, TypeError)
DERROR(urierror, URIError)

#undef DERROR

void jsB_initerror(mjs_State *J)
{
	mjs_pushobject(J, J->Error_prototype);
	{
			jsB_props(J, "name", "Error");
			jsB_props(J, "message", "an error has occurred");
			jsB_propf(J, "Error.prototype.toString", Ep_toString, 0);
	}
	mjs_newcconstructor(J, jsB_Error, jsB_Error, "Error", 1);
	mjs_defglobal(J, "Error", JS_DONTENUM);

	#define IERROR(NAME) \
		mjs_pushobject(J, J->NAME##_prototype); \
		jsB_props(J, "name", Q(NAME)); \
		mjs_newcconstructor(J, jsB_##NAME, jsB_##NAME, Q(NAME), 1); \
		mjs_defglobal(J, Q(NAME), JS_DONTENUM);

	IERROR(EvalError);
	IERROR(RangeError);
	IERROR(ReferenceError);
	IERROR(SyntaxError);
	IERROR(TypeError);
	IERROR(URIError);

	#undef IERROR
}
