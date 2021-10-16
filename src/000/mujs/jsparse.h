#ifndef mjs_parse_h
#define mjs_parse_h

enum mjs_AstType
{
	AST_LIST,
	AST_FUNDEC,
	AST_IDENTIFIER,

	EXP_IDENTIFIER,
	EXP_NUMBER,
	EXP_STRING,
	EXP_REGEXP,

	/* literals */
	EXP_UNDEF, /* for array elisions */
	EXP_NULL,
	EXP_TRUE,
	EXP_FALSE,
	EXP_THIS,

	EXP_ARRAY,
	EXP_OBJECT,
	EXP_PROP_VAL,
	EXP_PROP_GET,
	EXP_PROP_SET,

	EXP_FUN,

	/* expressions */
	EXP_INDEX,
	EXP_MEMBER,
	EXP_CALL,
	EXP_NEW,

	EXP_POSTINC,
	EXP_POSTDEC,

	EXP_DELETE,
	EXP_VOID,
	EXP_TYPEOF,
	EXP_PREINC,
	EXP_PREDEC,
	EXP_POS,
	EXP_NEG,
	EXP_BITNOT,
	EXP_LOGNOT,

	EXP_MOD,
	EXP_DIV,
	EXP_MUL,
	EXP_SUB,
	EXP_ADD,
	EXP_USHR,
	EXP_SHR,
	EXP_SHL,
	EXP_IN,
	EXP_INSTANCEOF,
	EXP_GE,
	EXP_LE,
	EXP_GT,
	EXP_LT,
	EXP_STRICTNE,
	EXP_STRICTEQ,
	EXP_NE,
	EXP_EQ,
	EXP_BITAND,
	EXP_BITXOR,
	EXP_BITOR,
	EXP_LOGAND,
	EXP_LOGOR,

	EXP_COND,

	EXP_ASS,
	EXP_ASS_MUL,
	EXP_ASS_DIV,
	EXP_ASS_MOD,
	EXP_ASS_ADD,
	EXP_ASS_SUB,
	EXP_ASS_SHL,
	EXP_ASS_SHR,
	EXP_ASS_USHR,
	EXP_ASS_BITAND,
	EXP_ASS_BITXOR,
	EXP_ASS_BITOR,

	EXP_COMMA,

	EXP_VAR, /* var initializer */

	/* statements */
	STM_BLOCK,
	STM_EMPTY,
	STM_VAR,
	STM_IF,
	STM_DO,
	STM_WHILE,
	STM_FOR,
	STM_FOR_VAR,
	STM_FOR_IN,
	STM_FOR_IN_VAR,
	STM_CONTINUE,
	STM_BREAK,
	STM_RETURN,
	STM_WITH,
	STM_SWITCH,
	STM_THROW,
	STM_TRY,
	STM_DEBUGGER,

	STM_LABEL,
	STM_CASE,
	STM_DEFAULT,
};

typedef struct mjs_JumpList mjs_JumpList;

struct mjs_JumpList
{
	enum mjs_AstType type;
	int inst;
	mjs_JumpList *next;
};

struct mjs_Ast
{
	enum mjs_AstType type;
	int line;
	mjs_Ast *parent, *a, *b, *c, *d;
	double number;
	const char *string;
	mjs_JumpList *jumps; /* list of break/continue jumps to patch */
	int casejump; /* for switch case clauses */
	mjs_Ast *gcnext; /* next in alloc list */
};

mjs_Ast *jsP_parsefunction(mjs_State *J, const char *filename, const char *params, const char *body);
mjs_Ast *jsP_parse(mjs_State *J, const char *filename, const char *source);
void jsP_freeparse(mjs_State *J);

const char *jsP_aststring(enum mjs_AstType type);
void jsP_dumpsyntax(mjs_State *J, mjs_Ast *prog, int minify);
void jsP_dumplist(mjs_State *J, mjs_Ast *prog);

#endif
