#include "jsi.h"
#include "jsvalue.h"

/*
	Use an AA-tree to quickly look up properties in objects:

	The level of every leaf node is one.
	The level of every left child is one less than its parent.
	The level of every right child is equal or one less than its parent.
	The level of every right grandchild is less than its grandparent.
	Every node of level greater than one has two children.

	A link where the child's level is equal to that of its parent is called a horizontal link.
	Individual right horizontal links are allowed, but consecutive ones are forbidden.
	Left horizontal links are forbidden.

	skew() fixes left horizontal links.
	split() fixes consecutive right horizontal links.
*/

static mjs_Property sentinel = {
	"",
	&sentinel, &sentinel,
	0, 0,
	{ {0}, {0}, JS_TUNDEFINED },
	NULL, NULL
};

static mjs_Property *newproperty(mjs_State *J, mjs_Object *obj, const char *name)
{
	mjs_Property *node = mjs_malloc(J, sizeof *node);
	node->name = mjs_intern(J, name);
	node->left = node->right = &sentinel;
	node->level = 1;
	node->atts = 0;
	node->value.type = JS_TUNDEFINED;
	node->value.u.number = 0;
	node->getter = NULL;
	node->setter = NULL;
	++obj->count;
	return node;
}

static mjs_Property *lookup(mjs_Property *node, const char *name)
{
	while (node != &sentinel) {
		int c = strcmp(name, node->name);
		if (c == 0)
			return node;
		else if (c < 0)
			node = node->left;
		else
			node = node->right;
	}
	return NULL;
}

static mjs_Property *skew(mjs_Property *node)
{
	if (node->left->level == node->level) {
		mjs_Property *temp = node;
		node = node->left;
		temp->left = node->right;
		node->right = temp;
	}
	return node;
}

static mjs_Property *split(mjs_Property *node)
{
	if (node->right->right->level == node->level) {
		mjs_Property *temp = node;
		node = node->right;
		temp->right = node->left;
		node->left = temp;
		++node->level;
	}
	return node;
}

static mjs_Property *insert(mjs_State *J, mjs_Object *obj, mjs_Property *node, const char *name, mjs_Property **result)
{
	if (node != &sentinel) {
		int c = strcmp(name, node->name);
		if (c < 0)
			node->left = insert(J, obj, node->left, name, result);
		else if (c > 0)
			node->right = insert(J, obj, node->right, name, result);
		else
			return *result = node;
		node = skew(node);
		node = split(node);
		return node;
	}
	return *result = newproperty(J, obj, name);
}

static void freeproperty(mjs_State *J, mjs_Object *obj, mjs_Property *node)
{
	mjs_free(J, node);
	--obj->count;
}

static mjs_Property *delete(mjs_State *J, mjs_Object *obj, mjs_Property *node, const char *name)
{
	mjs_Property *temp, *succ;

	if (node != &sentinel) {
		int c = strcmp(name, node->name);
		if (c < 0) {
			node->left = delete(J, obj, node->left, name);
		} else if (c > 0) {
			node->right = delete(J, obj, node->right, name);
		} else {
			if (node->left == &sentinel) {
				temp = node;
				node = node->right;
				freeproperty(J, obj, temp);
			} else if (node->right == &sentinel) {
				temp = node;
				node = node->left;
				freeproperty(J, obj, temp);
			} else {
				succ = node->right;
				while (succ->left != &sentinel)
					succ = succ->left;
				node->name = succ->name;
				node->atts = succ->atts;
				node->value = succ->value;
				node->right = delete(J, obj, node->right, succ->name);
			}
		}

		if (node->left->level < node->level - 1 ||
			node->right->level < node->level - 1)
		{
			if (node->right->level > --node->level)
				node->right->level = node->level;
			node = skew(node);
			node->right = skew(node->right);
			node->right->right = skew(node->right->right);
			node = split(node);
			node->right = split(node->right);
		}
	}
	return node;
}

mjs_Object *jsV_newobject(mjs_State *J, enum mjs_Class type, mjs_Object *prototype)
{
	mjs_Object *obj = mjs_malloc(J, sizeof *obj);
	memset(obj, 0, sizeof *obj);
	obj->gcmark = 0;
	obj->gcnext = J->gcobj;
	J->gcobj = obj;
	++J->gccounter;

	obj->type = type;
	obj->properties = &sentinel;
	obj->prototype = prototype;
	obj->extensible = 1;
	return obj;
}

mjs_Property *jsV_getownproperty(mjs_State *J, mjs_Object *obj, const char *name)
{
	return lookup(obj->properties, name);
}

mjs_Property *jsV_getpropertyx(mjs_State *J, mjs_Object *obj, const char *name, int *own)
{
	*own = 1;
	do {
		mjs_Property *ref = lookup(obj->properties, name);
		if (ref)
			return ref;
		obj = obj->prototype;
		*own = 0;
	} while (obj);
	return NULL;
}

mjs_Property *jsV_getproperty(mjs_State *J, mjs_Object *obj, const char *name)
{
	do {
		mjs_Property *ref = lookup(obj->properties, name);
		if (ref)
			return ref;
		obj = obj->prototype;
	} while (obj);
	return NULL;
}

static mjs_Property *jsV_getenumproperty(mjs_State *J, mjs_Object *obj, const char *name)
{
	do {
		mjs_Property *ref = lookup(obj->properties, name);
		if (ref && !(ref->atts & JS_DONTENUM))
			return ref;
		obj = obj->prototype;
	} while (obj);
	return NULL;
}

mjs_Property *jsV_setproperty(mjs_State *J, mjs_Object *obj, const char *name)
{
	mjs_Property *result;

	if (!obj->extensible) {
		result = lookup(obj->properties, name);
		if (J->strict && !result)
			mjs_typeerror(J, "object is non-extensible");
		return result;
	}

	obj->properties = insert(J, obj, obj->properties, name, &result);

	return result;
}

void jsV_delproperty(mjs_State *J, mjs_Object *obj, const char *name)
{
	obj->properties = delete(J, obj, obj->properties, name);
}

/* Flatten hierarchy of enumerable properties into an iterator object */

static mjs_Iterator *itwalk(mjs_State *J, mjs_Iterator *iter, mjs_Property *prop, mjs_Object *seen)
{
	if (prop->right != &sentinel)
		iter = itwalk(J, iter, prop->right, seen);
	if (!(prop->atts & JS_DONTENUM)) {
		if (!seen || !jsV_getenumproperty(J, seen, prop->name)) {
			mjs_Iterator *head = mjs_malloc(J, sizeof *head);
			head->name = prop->name;
			head->next = iter;
			iter = head;
		}
	}
	if (prop->left != &sentinel)
		iter = itwalk(J, iter, prop->left, seen);
	return iter;
}

static mjs_Iterator *itflatten(mjs_State *J, mjs_Object *obj)
{
	mjs_Iterator *iter = NULL;
	if (obj->prototype)
		iter = itflatten(J, obj->prototype);
	if (obj->properties != &sentinel)
		iter = itwalk(J, iter, obj->properties, obj->prototype);
	return iter;
}

mjs_Object *jsV_newiterator(mjs_State *J, mjs_Object *obj, int own)
{
	char buf[32];
	int k;
	mjs_Object *io = jsV_newobject(J, JS_CITERATOR, NULL);
	io->u.iter.target = obj;
	if (own) {
		io->u.iter.head = NULL;
		if (obj->properties != &sentinel)
			io->u.iter.head = itwalk(J, io->u.iter.head, obj->properties, NULL);
	} else {
		io->u.iter.head = itflatten(J, obj);
	}
	if (obj->type == JS_CSTRING) {
		mjs_Iterator *tail = io->u.iter.head;
		if (tail)
			while (tail->next)
				tail = tail->next;
		for (k = 0; k < obj->u.s.length; ++k) {
			mjs_itoa(buf, k);
			if (!jsV_getenumproperty(J, obj, buf)) {
				mjs_Iterator *node = mjs_malloc(J, sizeof *node);
				node->name = mjs_intern(J, mjs_itoa(buf, k));
				node->next = NULL;
				if (!tail)
					io->u.iter.head = tail = node;
				else {
					tail->next = node;
					tail = node;
				}
			}
		}
	}
	return io;
}

const char *jsV_nextiterator(mjs_State *J, mjs_Object *io)
{
	int k;
	if (io->type != JS_CITERATOR)
		mjs_typeerror(J, "not an iterator");
	while (io->u.iter.head) {
		mjs_Iterator *next = io->u.iter.head->next;
		const char *name = io->u.iter.head->name;
		mjs_free(J, io->u.iter.head);
		io->u.iter.head = next;
		if (jsV_getproperty(J, io->u.iter.target, name))
			return name;
		if (io->u.iter.target->type == JS_CSTRING)
			if (mjs_isarrayindex(J, name, &k) && k < io->u.iter.target->u.s.length)
				return name;
	}
	return NULL;
}

/* Walk all the properties and delete them one by one for arrays */

void jsV_resizearray(mjs_State *J, mjs_Object *obj, int newlen)
{
	char buf[32];
	const char *s;
	int k;
	if (newlen < obj->u.a.length) {
		if (obj->u.a.length > obj->count * 2) {
			mjs_Object *it = jsV_newiterator(J, obj, 1);
			while ((s = jsV_nextiterator(J, it))) {
				k = jsV_numbertointeger(jsV_stringtonumber(J, s));
				if (k >= newlen && !strcmp(s, jsV_numbertostring(J, buf, k)))
					jsV_delproperty(J, obj, s);
			}
		} else {
			for (k = newlen; k < obj->u.a.length; ++k) {
				jsV_delproperty(J, obj, mjs_itoa(buf, k));
			}
		}
	}
	obj->u.a.length = newlen;
}
