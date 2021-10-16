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
	char*json=NULL;
	mjson_printf(mjson_print_dynamic_buf,&json,"{%Q:%d}","name",1234);
	mg_http_reply(c,200,"Content-Type: application/json\r\n","%s",json);
	free(json);
}

