#include"util/util.h"
#include<cstdio>
#include<iostream>
#include"cpr/cpr.h"
#include"mongoose.h"
static int reqidx=0;
static void ctor();
static void dtor();
void __attribute__((constructor))ctor();
void __attribute__((destructor))dtor();
EXTERN_C_BEG
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
EXTERN_C_END
static void ctor(){
	reqidx=0;
}
static void dtor(){
	reqidx=0;
}
void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"tpl:%d",reqidx++);
	cpr::Response r = cpr::Get(cpr::Url{"http://skullquake.dedicated.co.za"},
		//cpr::Authentication{"user", "pass"},
		cpr::Parameters{{"anon", "true"}, {"key", "value"}}
	);
	r.status_code;                  // 200
	r.header["content-type"];       // application/json; charset=utf-8
	r.text;                         // JSON text string
	mg_http_printf_chunk(c,"%s",r.text.c_str());//end
	mg_http_printf_chunk(c,"");//end
}

