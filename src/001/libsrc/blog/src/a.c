#include"dll.h"
#include<stdio.h>
#include"mongoose.h"
#include"mjson.h"
DLL_LOCAL static int reqidx=0;
DLL_LOCAL static void ctor();
DLL_LOCAL static void dtor();
DLL_LOCAL void __attribute__((constructor))ctor();
DLL_LOCAL void __attribute__((destructor))dtor();
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
DLL_PUBLIC void test0(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
DLL_PUBLIC void test1(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
static void ctor(){
	reqidx=0;
}
static void dtor(){
	reqidx=0;
}
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	test1(c,ev,ev_data,fn_data);
}
DLL_PUBLIC void test1(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	struct mg_http_message*hm=(struct mg_http_message*)ev_data;
	mg_http_reply(c,200,"Content-Type: application/json\r\n","%s","{\"a\":[]}");
}
DLL_PUBLIC void test0(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	struct mg_http_message*hm=(struct mg_http_message*)ev_data;
	char*uri=NULL;
	char*query=NULL;
	char*method=NULL;
	char*proto=NULL;
	char*body=NULL;
	char*message=NULL;
	{
		uri=(char*)malloc(hm->uri.len+1);if(!uri)return;
		memcpy(uri,hm->uri.ptr,hm->uri.len);uri[hm->uri.len]='\0';
	}
	{
		method=(char*)malloc(hm->method.len+1);if(!method){free(uri);return;}
		memcpy(method,hm->method.ptr,hm->method.len);method[hm->method.len]='\0';
	}
	{
		query=(char*)malloc(hm->query.len+1);if(!query){free(uri);free(method);return;}
		memcpy(query,hm->query.ptr,hm->query.len);query[hm->query.len]='\0';
	}
	{
		proto=(char*)malloc(hm->proto.len+1);if(!proto){free(uri);free(method);free(query);return;}
		memcpy(proto,hm->proto.ptr,hm->proto.len);proto[hm->proto.len]='\0';
	}
	{
		body=(char*)malloc(hm->body.len+1);if(!body){free(uri);free(method);free(query);free(proto);return;}
		memcpy(body,hm->body.ptr,hm->body.len);body[hm->body.len]='\0';
	}
	{
		message=(char*)malloc(hm->message.len+1);if(!message){free(uri);free(method);free(query);free(proto);free(message);return;}
		memcpy(message,hm->message.ptr,hm->message.len);message[hm->message.len]='\0';
	}
	char*json=NULL;
	mjson_printf(mjson_print_dynamic_buf,&json,"{%Q:%Q,%Q:%Q,%Q:%Q,%Q:%Q,%Q:%Q,%Q:%Q}","uri",uri,"method",method,"query",query,"proto",proto,"body",body,"message",message);
	mg_http_reply(c,200,"Content-Type: application/json\r\n","%s",json);
	free(json);
	free(uri);
	free(method);
	free(query);
	free(proto);
	free(message);
	free(body);
}

