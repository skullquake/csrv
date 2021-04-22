#ifndef mjs_run_h
#define mjs_run_h

mjs_Environment *jsR_newenvironment(mjs_State *J, mjs_Object *variables, mjs_Environment *outer);

struct mjs_Environment
{
	mjs_Environment *outer;
	mjs_Object *variables;

	mjs_Environment *gcnext;
	int gcmark;
};

#endif
