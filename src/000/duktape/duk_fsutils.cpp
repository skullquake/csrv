#include"duktape/duk_load.hpp"
//#include<iostream>
#include<fstream>
#include<sstream>
extern "C"{
duk_ret_t native_file2string(duk_context*ctx){
	const char*filename=duk_to_string(ctx,0);
	if(filename!=nullptr){
		//std::cerr<<"Loading "<<filename<<"..."<<std::endl;
		std::ifstream ifs(filename);
		std::ostringstream oss;
		if(ifs){
			oss<<ifs.rdbuf();
			duk_push_string(ctx,oss.str().c_str());
			return 1;
		}else{
			//std::cerr<<"error:failed to open file "<<std::string(filename)<<std::endl;
		}
	}else{
		//std::cerr<<"error:filename null"<<std::endl;
	}
	return 0;
}
void duk_fsutils_init(duk_context*ctx){
	duk_push_c_function(ctx,native_file2string,1);
	duk_put_global_string(ctx,"file2string");
}

}

