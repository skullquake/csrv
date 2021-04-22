#ifdef DUK_DSO
#include"duktape/duk_dso.hpp"
#include"readlib/readlib.hpp"
#include"dynlod/dynlod.h"
#include<fstream>
//------------------------------------------------------------------------------
//native functions - begin
//------------------------------------------------------------------------------
/*! Native dlopen
 *  \return `duk_ret_t` return type
 */
static int native_dlopen(duk_context *ctx) {
        const char*path= duk_to_string(ctx,0);
        int mod=duk_to_int(ctx,1);
	void*lib=(void*)dlopen(path,mod);
	if(lib==nullptr){
		duk_push_null(ctx);
	}else{
		duk_push_pointer(ctx,lib);
	}
        return 1;
}
/*! Native dlsym
 *  \return `duk_ret_t` return type
 */
static int native_dlsym(duk_context *ctx) {
        void*lib=(void*)duk_to_pointer(ctx,0);
        const char*symnam= duk_to_string(ctx,1);
	void*sym=(void*)dlsym(lib,symnam);
	if(sym==nullptr){
		duk_push_null(ctx);
	}else{
		duk_push_pointer(ctx,sym);
	}
        return 1;
}
/*! Native dlclose
 *  \return `duk_ret_t` return type
 */
static int native_dlclose(duk_context *ctx) {
        void*lib=(void*)duk_to_pointer(ctx,0);
	duk_push_int(ctx,dlclose(lib));
        return 1;
}
/*! Duk dlclose
 *  \return std::vector<std::string> symbols
 */
std::vector<std::string> wrapped_dlclose(std::string path) {
	std::vector<std::string> ret;
	bool file_exists=false;
	{
		std::ifstream ifs(path);
		file_exists=ifs.good();
		ifs.close();
	}
	if(file_exists){
		void*hdl=dlopen(path.c_str(),RTLD_LAZY);//has to be open for readlib???
		if(!hdl){
			//oss<<nlohmann::json({{"err","failed to open object"}});
		}else{
			std::vector<std::string>vsym=readlib(path);
			ret=vsym;
			dlclose(hdl);
		}
	}else{
		//oss<<nlohmann::json({{"error","file "+path+" not found"}});
	}
	return ret;
}
/*! Native dlinvoke
 *  executes function, for now void(*)(void*)
 *  \return `duk_ret_t` return type
 */
static int native_dlinvoke(duk_context *ctx) {
        void*sym=(void*)duk_to_pointer(ctx,0);
	int ret=0;
	if(sym==nullptr){
	}else{
		if(duk_is_null(ctx,1)) {
			((void(*)(void*))sym)(nullptr);
		}else if(duk_is_undefined(ctx,1)) {
			((void(*)(void*))sym)(nullptr);
		}else if(duk_is_number(ctx,1)) {
			auto val=duk_to_number(ctx,1);
			((void(*)(void*))sym)((void*)(&val));
		}else if(duk_is_string(ctx,1)) {
			((void(*)(void*))sym)((void*)duk_to_string(ctx,1));
		}else if(duk_is_pointer(ctx,1)) {
			//((void(*)(void*))sym)((void*)duk_to_pointer(ctx,1));
			ret=((int(*)(void*))sym)((void*)duk_to_pointer(ctx,1));
		}else if(duk_is_object(ctx,1)) {
		}else if(duk_is_array(ctx,1)) {
		}else if(duk_is_boolean(ctx,1)) {
		}else if(duk_is_function(ctx,1)) {
		}else if(duk_is_thread(ctx,1)) {
		}else if(duk_is_buffer(ctx,1)) {
			void *ptr;
			duk_size_t sz;
			ptr=duk_get_buffer(ctx,1,&sz);
			((void(*)(void*))sym)(ptr);
		}else if(duk_is_fixed_buffer(ctx,1)) {
		}else if(duk_is_dynamic_buffer(ctx,1)) {
		}else{
		}
	}
        return ret;
}

//------------------------------------------------------------------------------
//native functions - end
//------------------------------------------------------------------------------
void duk_dso_init(duk_context *ctx){
		duk_push_int(ctx,RTLD_LAZY);
		duk_put_global_string(ctx,"RTLD_LAZY");
		duk_push_int(ctx,RTLD_NOW);
		duk_put_global_string(ctx,"RTLD_NOW");
		duk_push_int(ctx,RTLD_BINDING_MASK);
		duk_put_global_string(ctx,"RTLD_BINDING_MASK");
		duk_push_int(ctx,RTLD_NOLOAD);
		duk_put_global_string(ctx,"RTLD_NOLOAD");
		duk_push_int(ctx,RTLD_DEEPBIND);
		duk_put_global_string(ctx,"RTLD_DEEPBIND");
		duk_push_c_function(ctx,native_dlopen,2);
		duk_put_global_string(ctx,"dlopen");
		duk_push_c_function(ctx,native_dlsym,2);
		duk_put_global_string(ctx,"dlsym");
		duk_push_pointer(ctx,(void*)ctx);
		duk_put_global_string(ctx,"ctx");
		duk_push_c_function(ctx,native_dlclose,1);
		duk_put_global_string(ctx,"dlclose");
		duk_push_c_function(ctx,native_dlinvoke,2);
		duk_put_global_string(ctx,"dlinvoke");
		//dukglue_register_function(ctx,getModulePath,"getModulePath");
		//dukglue_register_function(ctx,getModulePath,"getModulePath");
		//dukglue_register_function(ctx,readlib,"readlib");
}
#endif
