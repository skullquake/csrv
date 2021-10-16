#include"util/util.h"
#include<stdio.h>
#include"mongoose.h"

#include<sstream>
#include<iostream>
#include<cstdio>
#include<cstring>
#include"quickjspp.hpp"

/*
DLL_LOCAL void __attribute__((constructor))ctor();
DLL_LOCAL void __attribute__((destructor))dtor();
DLL_LOCAL static void ctor(){
}
DLL_LOCAL static void dtor(){
}
*/

EXTERN_C_BEG
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
EXTERN_C_END
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
DLL_LOCAL static qjs::Runtime runtime;
DLL_LOCAL static qjs::Context context(runtime);
DLL_LOCAL static bool qjs_initialized=false;
	if(!qjs_initialized){
		qjs_initialized=true;
		JSRuntime*rt;
		JSContext*ctx;
		rt=runtime.rt;
		ctx=context.ctx;
		js_std_init_handlers(rt);
		JS_SetModuleLoaderFunc(rt,nullptr,js_module_loader,nullptr);
		js_std_add_helpers(ctx,0,nullptr);
		js_init_module_std(ctx,"std");
		js_init_module_os(ctx,"os");
		context.eval(R"(
			import * as std from 'std';
			import * as os from 'os';
			globalThis.std = std;
			globalThis.os = os;
		)","<input>",JS_EVAL_TYPE_MODULE);
		{
			auto& module=context.addModule("Config");
			module.function("get_os",[](){
#ifdef _WIN32
					return "windows";
#else
					return "linux";
#endif
			});
			context.eval(R"(
				import * as config from 'Config';
				globalThis.config = config;
			)","<input>",JS_EVAL_TYPE_MODULE);

		}
		{
			auto& module=context.addModule("Mg");
			module.function("printf",[&c](std::string m){
				mg_printf(c,m.c_str());
			});
			module.function("printf_chunk",[&c](std::string m){
				mg_http_printf_chunk(c,m.c_str());
			});
			context.eval(R"(
				import * as mg from 'Mg';
				globalThis.mg = mg;
			)","<input>",JS_EVAL_TYPE_MODULE);

		}
	}else{
	}
	try{
		//context.evalFile("./js/a.js",JS_EVAL_TYPE_MODULE);
		context.eval(R"(
			mg.printf('HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n');
			a=typeof(a)=="undefined"?0:a;
			a++;
			console.log(a);
			/*
			for(var i=0;i<8;i++){
				for(var j=0;j<8;j++){
					mg.printf_chunk('['+i+']['+j+']');
				}
				mg.printf_chunk('\n');
			}
			*/
			mg.printf_chunk(a+'\n');
			mg.printf_chunk('');
		)");
		/*
		*/
	}catch(qjs::exception){
		mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n");
		mg_http_printf_chunk(c,"Error:\n");
		auto exc=context.getException();
		std::cerr<<(std::string)exc<<std::endl;
		mg_http_printf_chunk(c,"\t%s\n",((std::string)exc).c_str());
		if((bool)exc["stack"]){
			std::cerr<<(std::string)exc["stack"]<<std::endl;
			mg_http_printf_chunk(c,"\t%s\n",((std::string)exc["stack"]).c_str());
		}
		mg_http_printf_chunk(c,"");
	}
}

//CXXFLAGS+=-std=c++17

