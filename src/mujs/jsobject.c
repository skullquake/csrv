#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static void jsB_new_Object(mjs_State *J)
{
	if (mjs_isundefined(J, 1) || mjs_isnull(J, 1))
		mjs_newobject(J);
	else
		mjs_pushobject(J, mjs_toobject(J, 1));
}

static void jsB_Object(mjs_State *J)
{
	if (mjs_isundefined(J, 1) || mjs_isnull(J, 1))
		mjs_newobject(J);
	else
		mjs_pushobject(J, mjs_toobject(J, 1));
}

static void Op_toString(mjs_State *J)
{
	if (mjs_isundefined(J, 0))
		mjs_pushliteral(J, "[object Undefined]");
	else if (mjs_isnull(J, 0))
		mjs_pushliteral(J, "[object Null]");
	else {
		mjs_Object *self = mjs_toobject(J, 0);
		switch (self->type) {
		case JS_COBJECT: mjs_pushliteral(J, "[object Object]"); break;
		case JS_CARRAY: mjs_pushliteral(J, "[object Array]"); break;
		case JS_CFUNCTION: mjs_pushliteral(J, "[object Function]"); break;
		case JS_CSCRIPT: mjs_pushliteral(J, "[object Function]"); break;
		case JS_CEVAL: mjs_pushliteral(J, "[object Function]"); break;
		case JS_CCFUNCTION: mjs_pushliteral(J, "[object Function]"); break;
		case JS_CERROR: mjs_pushliteral(J, "[object Error]"); break;
		case JS_CBOOLEAN: mjs_pushliteral(J, "[object Boolean]"); break;
		case JS_CNUMBER: mjs_pushliteral(J, "[object Number]"); break;
		case JS_CSTRING: mjs_pushliteral(J, "[object String]"); break;
		case JS_CREGEXP: mjs_pushliteral(J, "[object RegExp]"); break;
		case JS_CDATE: mjs_pushliteral(J, "[object Date]"); break;
		case JS_CMATH: mjs_pushliteral(J, "[object Math]"); break;
		case JS_CJSON: mjs_pushliteral(J, "[object JSON]"); break;
		case JS_CARGUMENTS: mjs_pushliteral(J, "[object Arguments]"); break;
		case JS_CITERATOR: mjs_pushliteral(J, "[object Iterator]"); break;
		case JS_CUSERDATA:
			mjs_pushliteral(J, "[object ");
			mjs_pushliteral(J, self->u.user.tag);
			mjs_concat(J);
			mjs_pushliteral(J, "]");
			mjs_concat(J);
			break;
		}
	}
}

static void Op_valueOf(mjs_State *J)
{
	mjs_copy(J, 0);
}

static void Op_hasOwnProperty(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	const char *name = mjs_tostring(J, 1);
	mjs_Property *ref = jsV_getownproperty(J, self, name);
	mjs_pushboolean(J, ref != NULL);
}

static void Op_isPrototypeOf(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	if (mjs_isobject(J, 1)) {
		mjs_Object *V = mjs_toobject(J, 1);
		do {
			V = V->prototype;
			if (V == self) {
				mjs_pushboolean(J, 1);
				return;
			}
		} while (V);
	}
	mjs_pushboolean(J, 0);
}

static void Op_propertyIsEnumerable(mjs_State *J)
{
	mjs_Object *self = mjs_toobject(J, 0);
	const char *name = mjs_tostring(J, 1);
	mjs_Property *ref = jsV_getownproperty(J, self, name);
	mjs_pushboolean(J, ref && !(ref->atts & JS_DONTENUM));
}

static void O_getPrototypeOf(mjs_State *J)
{
	mjs_Object *obj;
	if (!mjs_isobject(J, 1))
		mjs_typeerror(J, "not an object");
	obj = mjs_toobject(J, 1);
	if (obj->prototype)
		mjs_pushobject(J, obj->prototype);
	else
		mjs_pushnull(J);
}

static void O_getOwnPropertyDescriptor(mjs_State *J)
{
	mjs_Object *obj;
	mjs_Property *ref;
	if (!mjs_isobject(J, 1))
		mjs_typeerror(J, "not an object");
	obj = mjs_toobject(J, 1);
	ref = jsV_getproperty(J, obj, mjs_tostring(J, 2));
	if (!ref)
		mjs_pushundefined(J);
	else {
		mjs_newobject(J);
		if (!ref->getter && !ref->setter) {
			mjs_pushvalue(J, ref->value);
			mjs_setproperty(J, -2, "value");
			mjs_pushboolean(J, !(ref->atts & JS_READONLY));
			mjs_setproperty(J, -2, "writable");
		} else {
			if (ref->getter)
				mjs_pushobject(J, ref->getter);
			else
				mjs_pushundefined(J);
			mjs_setproperty(J, -2, "get");
			if (ref->setter)
				mjs_pushobject(J, ref->setter);
			else
				mjs_pushundefined(J);
			mjs_setproperty(J, -2, "set");
		}
		mjs_pushboolean(J, !(ref->atts & JS_DONTENUM));
		mjs_setproperty(J, -2, "enumerable");
		mjs_pushboolean(J, !(ref->atts & JS_DONTCONF));
		mjs_setproperty(J, -2, "configurable");
	}
}

static int O_getOwnPropertyNames_walk(mjs_State *J, mjs_Property *ref, int i)
{
	if (ref->left->level)
		i = O_getOwnPropertyNames_walk(J, ref->left, i);
	mjs_pushliteral(J, ref->name);
	mjs_setindex(J, -2, i++);
	if (ref->right->level)
		i = O_getOwnPropertyNames_walk(J, ref->right, i);
	return i;
}

static void O_getOwnPropertyNames(mjs_State *J)
{
	mjs_Object *obj;
	int k;
	int i;

	if (!mjs_isobject(J, 1))
		mjs_typeerror(J, "not an object");
	obj = mjs_toobject(J, 1);

	mjs_newarray(J);

	if (obj->properties->level)
		i = O_getOwnPropertyNames_walk(J, obj->properties, 0);
	else
		i = 0;

	if (obj->type == JS_CARRAY) {
		mjs_pushliteral(J, "length");
		mjs_setindex(J, -2, i++);
	}

	if (obj->type == JS_CSTRING) {
		mjs_pushliteral(J, "length");
		mjs_setindex(J, -2, i++);
		for (k = 0; k < obj->u.s.length; ++k) {
			mjs_pushnumber(J, k);
			mjs_setindex(J, -2, i++);
		}
	}

	if (obj->type == JS_CREGEXP) {
		mjs_pushliteral(J, "source");
		mjs_setindex(J, -2, i++);
		mjs_pushliteral(J, "global");
		mjs_setindex(J, -2, i++);
		mjs_pushliteral(J, "ignoreCase");
		mjs_setindex(J, -2, i++);
		mjs_pushliteral(J, "multiline");
		mjs_setindex(J, -2, i++);
		mjs_pushliteral(J, "lastIndex");
		mjs_setindex(J, -2, i++);
	}
}

static void ToPropertyDescriptor(mjs_State *J, mjs_Object *obj, const char *name, mjs_Object *desc)
{
	int haswritable = 0;
	int hasvalue = 0;
	int enumerable = 0;
	int configurable = 0;
	int writable = 0;
	int atts = 0;

	mjs_pushobject(J, obj);
	mjs_pushobject(J, desc);

	if (mjs_hasproperty(J, -1, "writable")) {
		haswritable = 1;
		writable = mjs_toboolean(J, -1);
		mjs_pop(J, 1);
	}
	if (mjs_hasproperty(J, -1, "enumerable")) {
		enumerable = mjs_toboolean(J, -1);
		mjs_pop(J, 1);
	}
	if (mjs_hasproperty(J, -1, "configurable")) {
		configurable = mjs_toboolean(J, -1);
		mjs_pop(J, 1);
	}
	if (mjs_hasproperty(J, -1, "value")) {
		hasvalue = 1;
		mjs_setproperty(J, -3, name);
	}

	if (!writable) atts |= JS_READONLY;
	if (!enumerable) atts |= JS_DONTENUM;
	if (!configurable) atts |= JS_DONTCONF;

	if (mjs_hasproperty(J, -1, "get")) {
		if (haswritable || hasvalue)
			mjs_typeerror(J, "value/writable and get/set attributes are exclusive");
	} else {
		mjs_pushundefined(J);
	}

	if (mjs_hasproperty(J, -2, "set")) {
		if (haswritable || hasvalue)
			mjs_typeerror(J, "value/writable and get/set attributes are exclusive");
	} else {
		mjs_pushundefined(J);
	}

	mjs_defaccessor(J, -4, name, atts);

	mjs_pop(J, 2);
}

static void O_defineProperty(mjs_State *J)
{
	if (!mjs_isobject(J, 1)) mjs_typeerror(J, "not an object");
	if (!mjs_isobject(J, 3)) mjs_typeerror(J, "not an object");
	ToPropertyDescriptor(J, mjs_toobject(J, 1), mjs_tostring(J, 2), mjs_toobject(J, 3));
	mjs_copy(J, 1);
}

static void O_defineProperties_walk(mjs_State *J, mjs_Property *ref)
{
	if (ref->left->level)
		O_defineProperties_walk(J, ref->left);
	if (!(ref->atts & JS_DONTENUM)) {
		mjs_pushvalue(J, ref->value);
		ToPropertyDescriptor(J, mjs_toobject(J, 1), ref->name, mjs_toobject(J, -1));
		mjs_pop(J, 1);
	}
	if (ref->right->level)
		O_defineProperties_walk(J, ref->right);
}

static void O_defineProperties(mjs_State *J)
{
	mjs_Object *props;

	if (!mjs_isobject(J, 1)) mjs_typeerror(J, "not an object");
	if (!mjs_isobject(J, 2)) mjs_typeerror(J, "not an object");

	props = mjs_toobject(J, 2);
	if (props->properties->level)
		O_defineProperties_walk(J, props->properties);

	mjs_copy(J, 1);
}

static void O_create_walk(mjs_State *J, mjs_Object *obj, mjs_Property *ref)
{
	if (ref->left->level)
		O_create_walk(J, obj, ref->left);
	if (!(ref->atts & JS_DONTENUM)) {
		if (ref->value.type != JS_TOBJECT)
			mjs_typeerror(J, "not an object");
		ToPropertyDescriptor(J, obj, ref->name, ref->value.u.object);
	}
	if (ref->right->level)
		O_create_walk(J, obj, ref->right);
}

static void O_create(mjs_State *J)
{
	mjs_Object *obj;
	mjs_Object *proto;
	mjs_Object *props;

	if (mjs_isobject(J, 1))
		proto = mjs_toobject(J, 1);
	else if (mjs_isnull(J, 1))
		proto = NULL;
	else
		mjs_typeerror(J, "not an object or null");

	obj = jsV_newobject(J, JS_COBJECT, proto);
	mjs_pushobject(J, obj);

	if (mjs_isdefined(J, 2)) {
		if (!mjs_isobject(J, 2))
			mjs_typeerror(J, "not an object");
		props = mjs_toobject(J, 2);
		if (props->properties->level)
			O_create_walk(J, obj, props->properties);
	}
}

static int O_keys_walk(mjs_State *J, mjs_Property *ref, int i)
{
	if (ref->left->level)
		i = O_keys_walk(J, ref->left, i);
	if (!(ref->atts & JS_DONTENUM)) {
		mjs_pushliteral(J, ref->name);
		mjs_setindex(J, -2, i++);
	}
	if (ref->right->level)
		i = O_keys_walk(J, ref->right, i);
	return i;
}

static void O_keys(mjs_State *J)
{
	mjs_Object *obj;
	int i, k;

	if (!mjs_isobject(J, 1))
		mjs_typeerror(J, "not an object");
	obj = mjs_toobject(J, 1);

	mjs_newarray(J);

	if (obj->properties->level)
		i = O_keys_walk(J, obj->properties, 0);
	else
		i = 0;

	if (obj->type == JS_CSTRING) {
		for (k = 0; k < obj->u.s.length; ++k) {
			mjs_pushnumber(J, k);
			mjs_setindex(J, -2, i++);
		}
	}
}

static void O_preventExtensions(mjs_State *J)
{
	if (!mjs_isobject(J, 1))
		mjs_typeerror(J, "not an object");
	mjs_toobject(J, 1)->extensible = 0;
	mjs_copy(J, 1);
}

static void O_isExtensible(mjs_State *J)
{
	if (!mjs_isobject(J, 1))
		mjs_typeerror(J, "not an object");
	mjs_pushboolean(J, mjs_toobject(J, 1)->extensible);
}

static void O_seal_walk(mjs_State *J, mjs_Property *ref)
{
	if (ref->left->level)
		O_seal_walk(J, ref->left);
	ref->atts |= JS_DONTCONF;
	if (ref->right->level)
		O_seal_walk(J, ref->right);
}

static void O_seal(mjs_State *J)
{
	mjs_Object *obj;

	if (!mjs_isobject(J, 1))
		mjs_typeerror(J, "not an object");

	obj = mjs_toobject(J, 1);
	obj->extensible = 0;

	if (obj->properties->level)
		O_seal_walk(J, obj->properties);

	mjs_copy(J, 1);
}

static int O_isSealed_walk(mjs_State *J, mjs_Property *ref)
{
	if (ref->left->level)
		if (!O_isSealed_walk(J, ref->left))
			return 0;
	if (!(ref->atts & JS_DONTCONF))
		return 0;
	if (ref->right->level)
		if (!O_isSealed_walk(J, ref->right))
			return 0;
	return 1;
}

static void O_isSealed(mjs_State *J)
{
	mjs_Object *obj;

	if (!mjs_isobject(J, 1))
		mjs_typeerror(J, "not an object");

	obj = mjs_toobject(J, 1);
	if (obj->extensible) {
		mjs_pushboolean(J, 0);
		return;
	}

	if (obj->properties->level)
		mjs_pushboolean(J, O_isSealed_walk(J, obj->properties));
	else
		mjs_pushboolean(J, 1);
}

static void O_freeze_walk(mjs_State *J, mjs_Property *ref)
{
	if (ref->left->level)
		O_freeze_walk(J, ref->left);
	ref->atts |= JS_READONLY | JS_DONTCONF;
	if (ref->right->level)
		O_freeze_walk(J, ref->right);
}

static void O_freeze(mjs_State *J)
{
	mjs_Object *obj;

	if (!mjs_isobject(J, 1))
		mjs_typeerror(J, "not an object");

	obj = mjs_toobject(J, 1);
	obj->extensible = 0;

	if (obj->properties->level)
		O_freeze_walk(J, obj->properties);

	mjs_copy(J, 1);
}

static int O_isFrozen_walk(mjs_State *J, mjs_Property *ref)
{
	if (ref->left->level)
		if (!O_isFrozen_walk(J, ref->left))
			return 0;
	if (!(ref->atts & JS_READONLY))
		return 0;
	if (!(ref->atts & JS_DONTCONF))
		return 0;
	if (ref->right->level)
		if (!O_isFrozen_walk(J, ref->right))
			return 0;
	return 1;
}

static void O_isFrozen(mjs_State *J)
{
	mjs_Object *obj;

	if (!mjs_isobject(J, 1))
		mjs_typeerror(J, "not an object");

	obj = mjs_toobject(J, 1);

	if (obj->properties->level) {
		if (!O_isFrozen_walk(J, obj->properties)) {
			mjs_pushboolean(J, 0);
			return;
		}
	}

	mjs_pushboolean(J, !obj->extensible);
}

void jsB_initobject(mjs_State *J)
{
	mjs_pushobject(J, J->Object_prototype);
	{
		jsB_propf(J, "Object.prototype.toString", Op_toString, 0);
		jsB_propf(J, "Object.prototype.toLocaleString", Op_toString, 0);
		jsB_propf(J, "Object.prototype.valueOf", Op_valueOf, 0);
		jsB_propf(J, "Object.prototype.hasOwnProperty", Op_hasOwnProperty, 1);
		jsB_propf(J, "Object.prototype.isPrototypeOf", Op_isPrototypeOf, 1);
		jsB_propf(J, "Object.prototype.propertyIsEnumerable", Op_propertyIsEnumerable, 1);
	}
	mjs_newcconstructor(J, jsB_Object, jsB_new_Object, "Object", 1);
	{
		/* ES5 */
		jsB_propf(J, "Object.getPrototypeOf", O_getPrototypeOf, 1);
		jsB_propf(J, "Object.getOwnPropertyDescriptor", O_getOwnPropertyDescriptor, 2);
		jsB_propf(J, "Object.getOwnPropertyNames", O_getOwnPropertyNames, 1);
		jsB_propf(J, "Object.create", O_create, 2);
		jsB_propf(J, "Object.defineProperty", O_defineProperty, 3);
		jsB_propf(J, "Object.defineProperties", O_defineProperties, 2);
		jsB_propf(J, "Object.seal", O_seal, 1);
		jsB_propf(J, "Object.freeze", O_freeze, 1);
		jsB_propf(J, "Object.preventExtensions", O_preventExtensions, 1);
		jsB_propf(J, "Object.isSealed", O_isSealed, 1);
		jsB_propf(J, "Object.isFrozen", O_isFrozen, 1);
		jsB_propf(J, "Object.isExtensible", O_isExtensible, 1);
		jsB_propf(J, "Object.keys", O_keys, 1);
	}
	mjs_defglobal(J, "Object", JS_DONTENUM);
}
