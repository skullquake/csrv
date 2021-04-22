/*!@file duk/native.c
 * @brief Various native functions to expose to Duktape
 * references : creating/returning object:
 *              https://stackoverflow.com/questions/36281265/using-json-objects-in-duktape
 *              differentiating between windows and linux:
 *              https://stackoverflow.com/questions/6649936/c-compiling-on-windows-and-linux-ifdef-switch
 *              [f]stat:
 *              https://en.wikipedia.org/wiki/Stat_(system_call)
 */
#include"duktape/log.h"
/*! Native print function, prints value to `stderr`. Appends newline character
 *  \param ctx Duktape context to run function in
 *  \return `duk_ret_t` return type
 */
static duk_ret_t native_log(duk_context*ctx){
	fprintf(stderr,"%s\n",duk_to_string(ctx,0));
	return 0;/*no return value(=undefined)*/
}
/*! Native print function, prints value to `stderr`.
 *  \param ctx Duktape context to run function in
 *  \return `duk_ret_t` return type
 */
static duk_ret_t native_log_(duk_context*ctx){
	fprintf(stderr,"%s",duk_to_string(ctx,0));
	return 0;/*no return value(=undefined)*/
}
//---------------------------
/*! Register native function on context
 *  \return `duk_ret_t` return type
 */
void duk_register_log(duk_context*ctx){
	duk_push_c_function(ctx,native_log,1/*nargs*/);
	duk_put_global_string(ctx,"log");
	duk_push_c_function(ctx,native_log_,1/*nargs*/);
	duk_put_global_string(ctx,"log_");
}