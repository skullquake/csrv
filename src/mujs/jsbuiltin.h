#ifndef mjs_builtin_h
#define mjs_builtin_h

void jsB_init(mjs_State *J);
void jsB_initobject(mjs_State *J);
void jsB_initarray(mjs_State *J);
void jsB_initfunction(mjs_State *J);
void jsB_initboolean(mjs_State *J);
void jsB_initnumber(mjs_State *J);
void jsB_initstring(mjs_State *J);
void jsB_initregexp(mjs_State *J);
void jsB_initerror(mjs_State *J);
void jsB_initmath(mjs_State *J);
void jsB_initjson(mjs_State *J);
void jsB_initdate(mjs_State *J);

void jsB_propf(mjs_State *J, const char *name, mjs_CFunction cfun, int n);
void jsB_propn(mjs_State *J, const char *name, double number);
void jsB_props(mjs_State *J, const char *name, const char *string);

#endif
