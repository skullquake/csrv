/*
 * https://github.com/nlohmann/json
 */
#include"util/util.h"
#include<cstdio>
#include<iostream>
#include"nlohmann/json/json.hpp"
#include"mongoose.h"
//--------------------------------------------------------------------------------
static void ctor();
static void dtor();
void __attribute__((constructor))ctor();
void __attribute__((destructor))dtor();
EXTERN_C_BEG
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
EXTERN_C_END
//--------------------------------------------------------------------------------
static void ctor(){}
static void dtor(){}
void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	nlohmann::json j;
	j["pi"]=3.141;
	j["happy"]=true;
	j["name"]="Niels";
	j["nothing"]=nullptr;
	j["answer"]["everything"]=42;
	j["list"]={1,0,2};
	j["object"]={{"currency","USD"},{"value",42.99}};
	j["list2"]={};
	for(int i=0;i<8;i++){
		j["list2"].push_back(i);
	}
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"%s",j.dump().c_str());
	mg_http_printf_chunk(c,"");
}
