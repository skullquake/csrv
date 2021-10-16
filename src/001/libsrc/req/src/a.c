#if defined _WIN32 || defined __CYGWIN__
	#ifdef BUILDING_DLL
		#ifdef __GNUC__
			#define DLL_PUBLIC __attribute__ ((dllexport))
		#else
			#define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
		#endif
	#else
		#ifdef __GNUC__
			#define DLL_PUBLIC __attribute__ ((dllimport))
		#else
			#define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
		#endif
	#endif
	#define DLL_LOCAL
#else
	#if __GNUC__ >= 4
		#define DLL_PUBLIC __attribute__ ((visibility ("default")))
		#define DLL_LOCAL	__attribute__ ((visibility ("hidden")))
	#else
		#define DLL_PUBLIC
		#define DLL_LOCAL
	#endif
#endif
#include<stdio.h>
#include"mongoose.h"
#include"mjson.h"
static int reqidx=0;
static void ctor();
static void dtor();
void __attribute__((constructor))ctor();
void __attribute__((destructor))dtor();
static void ctor(){
	reqidx=0;
	//printf("ctor:beg:%p\n",ctor);
	//printf("ctor:end:%p\n",ctor);
}
static void dtor(){
	reqidx=0;
	//printf("dtor:beg:%p\n",dtor);
	//printf("dtor:end:%p\n",dtor);
}
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
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

