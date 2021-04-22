#include"duktape/duk_load.hpp"
//#include<iostream>
#include<fstream>
#include<sstream>
extern "C"{
void duk_load_init(duk_context*ctx){
	duk_push_c_function(ctx,native_load,1);
	duk_put_global_string(ctx,"load");
}
duk_ret_t native_load(duk_context*ctx){
	const char*filename=duk_to_string(ctx,0);
	if(filename!=nullptr){
		//std::cerr<<"Loading "<<filename<<"..."<<std::endl;
		std::ifstream ifs(filename);
		std::ostringstream oss;
		if(ifs){
			oss<<ifs.rdbuf();
			duk_push_string(ctx,oss.str().c_str());
			if(duk_peval(ctx)!=0){
				fprintf(stderr,"%s",duk_safe_to_string(ctx,-1));
			}else{
			}
		//fprintf(stderr,"Info: native_load: done loading %s\n",filename);
		}else{
			//std::cerr<<"error:failed to open file "<<std::string(filename)<<std::endl;
		}
	}else{
		//std::cerr<<"error:filename null"<<std::endl;
	}
	return 0;
}
}

