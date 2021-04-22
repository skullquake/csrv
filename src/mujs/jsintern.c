#include "jsi.h"

/* Dynamically grown string buffer */

void mjs_putc(mjs_State *J, mjs_Buffer **sbp, int c)
{
	mjs_Buffer *sb = *sbp;
	if (!sb) {
		sb = mjs_malloc(J, sizeof *sb);
		sb->n = 0;
		sb->m = sizeof sb->s;
		*sbp = sb;
	} else if (sb->n == sb->m) {
		sb = mjs_realloc(J, sb, (sb->m *= 2) + soffsetof(mjs_Buffer, s));
		*sbp = sb;
	}
	sb->s[sb->n++] = c;
}

void mjs_puts(mjs_State *J, mjs_Buffer **sb, const char *s)
{
	while (*s)
		mjs_putc(J, sb, *s++);
}

void mjs_putm(mjs_State *J, mjs_Buffer **sb, const char *s, const char *e)
{
	while (s < e)
		mjs_putc(J, sb, *s++);
}

/* Use an AA-tree to quickly look up interned strings. */

struct mjs_StringNode
{
	mjs_StringNode *left, *right;
	int level;
	char string[1];
};

static mjs_StringNode jsS_sentinel = { &jsS_sentinel, &jsS_sentinel, 0, ""};

static mjs_StringNode *jsS_newstringnode(mjs_State *J, const char *string, const char **result)
{
	int n = strlen(string);
	mjs_StringNode *node = mjs_malloc(J, soffsetof(mjs_StringNode, string) + n + 1);
	node->left = node->right = &jsS_sentinel;
	node->level = 1;
	memcpy(node->string, string, n + 1);
	return *result = node->string, node;
}

static mjs_StringNode *jsS_skew(mjs_StringNode *node)
{
	if (node->left->level == node->level) {
		mjs_StringNode *temp = node;
		node = node->left;
		temp->left = node->right;
		node->right = temp;
	}
	return node;
}

static mjs_StringNode *jsS_split(mjs_StringNode *node)
{
	if (node->right->right->level == node->level) {
		mjs_StringNode *temp = node;
		node = node->right;
		temp->right = node->left;
		node->left = temp;
		++node->level;
	}
	return node;
}

static mjs_StringNode *jsS_insert(mjs_State *J, mjs_StringNode *node, const char *string, const char **result)
{
	if (node != &jsS_sentinel) {
		int c = strcmp(string, node->string);
		if (c < 0)
			node->left = jsS_insert(J, node->left, string, result);
		else if (c > 0)
			node->right = jsS_insert(J, node->right, string, result);
		else
			return *result = node->string, node;
		node = jsS_skew(node);
		node = jsS_split(node);
		return node;
	}
	return jsS_newstringnode(J, string, result);
}

static void dumpstringnode(mjs_StringNode *node, int level)
{
	int i;
	if (node->left != &jsS_sentinel)
		dumpstringnode(node->left, level + 1);
	printf("%d: ", node->level);
	for (i = 0; i < level; ++i)
		putchar('\t');
	printf("'%s'\n", node->string);
	if (node->right != &jsS_sentinel)
		dumpstringnode(node->right, level + 1);
}

void jsS_dumpstrings(mjs_State *J)
{
	mjs_StringNode *root = J->strings;
	printf("interned strings {\n");
	if (root && root != &jsS_sentinel)
		dumpstringnode(root, 1);
	printf("}\n");
}

static void jsS_freestringnode(mjs_State *J, mjs_StringNode *node)
{
	if (node->left != &jsS_sentinel) jsS_freestringnode(J, node->left);
	if (node->right != &jsS_sentinel) jsS_freestringnode(J, node->right);
	mjs_free(J, node);
}

void jsS_freestrings(mjs_State *J)
{
	if (J->strings && J->strings != &jsS_sentinel)
		jsS_freestringnode(J, J->strings);
}

const char *mjs_intern(mjs_State *J, const char *s)
{
	const char *result;
	if (!J->strings)
		J->strings = &jsS_sentinel;
	J->strings = jsS_insert(J, J->strings, s, &result);
	return result;
}
