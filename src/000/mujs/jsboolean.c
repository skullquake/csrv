#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static void jsB_new_Boolean(mjs_State *J)
{
	mjs_newboolean(J, mjs_toboolean(J, 1));
}

static void jsB_Boolean(mjs_State *J)
{
	mjs_pushboolean(J, mjs_toboolean(J, 1));
}

static void Bp_toString(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	if (self->type != JS_CBOOLEAN) mjs_typeerror(J, "not a boolean");
	mjs_pushliteral(J, self->u.boolean ? "true" : "false");
}

static void Bp_valueOf(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	if (self->type != JS_CBOOLEAN) mjs_typeerror(J, "not a boolean");
	mjs_pushboolean(J, self->u.boolean);
}

void jsB_initboolean(mjs_State *J)
{
	J->Boolean_prototype->u.boolean = 0;

	mjs_pushobject(J, J->Boolean_prototype);
	{
		jsB_propf(J, "Boolean.prototype.toString", Bp_toString, 0);
		jsB_propf(J, "Boolean.prototype.valueOf", Bp_valueOf, 0);
	}
	mjs_newcconstructor(J, jsB_Boolean, jsB_new_Boolean, "Boolean", 1);
	mjs_defglobal(J, "Boolean", JS_DONTENUM);
}
