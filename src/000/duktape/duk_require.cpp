/*
 * https://stackoverflow.com/questions/36610630/duktape-modsearch-native-implementation-in-c
 * https://github.com/svaarala/duktape/issues/194
 */
#include"duktape/duk_require.hpp"
//#include"dukglue/dukglue.h"
//#include<iostream>
#include<fstream>
#include<iterator>
#include<map>
std::multimap<duk_context*,void*>dsomap;
#ifdef SQ_DYNLOD
#include"dynlod/dynlod.h"
#endif
static inline bool ends_with(std::string const&value,std::string const&ending){
    if(ending.size()>value.size())return false;
    return std::equal(ending.rbegin(),ending.rend(),value.rbegin());
}
void duk_require_cleanup(duk_context*ctx){
#ifdef SQ_DYNLOD
	for(auto itr=dsomap.begin();itr!=dsomap.end();itr++)
		if(itr->first==ctx){
			//std::cout<<"DSO:erase:["<<itr->first<<","<<itr->second<<"]"<<std::endl;
			dlclose(itr->second);
		}else{
			//std::cout<<"DSO:hold :["<<itr->first<<","<<itr->second<<"]"<<std::endl;
		}
	dsomap.erase(ctx);
#endif
}
#ifdef __cplusplus
extern "C"{
#endif
void duk_require_init(duk_context*ctx){
	if(ctx!=nullptr){
		//Nargs was given as 4 and we get the following stack arguments:
		//index 0: id
		//index 1: require
		//index 2: exports
		//index 3: module
		duk_get_global_string(ctx,"Duktape");
		duk_push_c_function(ctx,[](duk_context*ctx)->int{
			try{
				char *src=NULL;
				FILE *f=NULL;
				const char*filename=duk_get_string(ctx,0);//duk_require_string
				auto path=std::string(filename);
				//std::cout<<"loading "<<path<<std::endl;
				if(
					ends_with(path,".js")||
					ends_with(path,".sjs")
				){
					std::ifstream ifs(path);
					if(ifs.fail()){
						//std::cerr<<"error: failed to open file"<<std::endl;
						return -1;
					}else{
						std::string src((std::istreambuf_iterator<char>(ifs)),std::istreambuf_iterator<char>());
						duk_push_string(ctx,src.c_str());//duk_push_lstring(ctx,src.c_str());
						return 1;
					}
				}else if(
					ends_with(path,".so")||
					ends_with(path,".dll")
				){
#ifdef SQ_DYNLOD
					std::vector<std::string>vmsg;
					void*hdl=dlopen(path.c_str(),RTLD_LAZY);
					if(!hdl){
						//std::cerr<<std::string(dlerror())<<std::endl;
						return -1;
					}else{
						int(*fn)(void*)=(int(*)(void*))(dlsym(hdl,"init"));
						if(fn!=NULL){
							dsomap.insert(std::pair<duk_context*,void*>(ctx,hdl)); 
							return (*fn)(ctx);
						}else{
							dlclose(hdl);
							return -1;
						}
					}
#else
					//std::cerr<<"DYNLOD disable"<<std::endl;
					return -1;
#endif
				}else{
					//std::cerr<<"Invalid extension"<<std::endl;
					return -1;
				}
			}catch(const std::exception&e){
				//std::cerr<<"error: "<<e.what()<<std::endl;
				return -1;
			}
		},4);
		duk_put_prop_string(ctx,-2,"modSearch");
		duk_pop(ctx);
	}else{
		//std::cerr<<"error: ctx null"<<std::endl;
	}
}
#ifdef __cplusplus
}
#endif

