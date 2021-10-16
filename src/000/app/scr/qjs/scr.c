#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>
#include<quickjs.h>
#include<quickjs.h>
#include<quickjs-libc.h>
static JSValue js_nop(JSContext*ctx,JSValueConst this_val,int argc,JSValueConst*argv){
	return JS_UNDEFINED;
}
static JSValue js_add(JSContext*ctx,JSValueConst this_val,int argc,JSValueConst*argv){
	int64_t a;
	int64_t b;
	JS_ToInt64(ctx,&a,argv[0]);
	JS_ToInt64(ctx,&b,argv[1]);
	return JS_NewInt64(ctx,a+b);
}
int qjs_run(int argc,char*argv[]){
	if(argc<2)exit(1);
	char*scr=NULL;
	{
		FILE*fp;
		fp=fopen(argv[1],"rb");
		if(fp!=NULL){
			char buf[4096];
			size_t n;
			size_t len=0;
			while(n=fread(buf,1,sizeof(buf),fp)){
				if(n<0){
					if(errno=EAGAIN)
						continue;
					break;
				}
				scr=realloc(scr,len+n+1);
				memcpy(scr+len,buf,n);
				len+=n;
				scr[len]='\0';
			}
			fclose(fp);//really slows down 3000vs900 if not closed
		}else{
			fprintf(stderr,"Failed to open script\n");
			exit(1);
		}
	}
	if(scr==NULL){
		fprintf(stderr,"Failed to open file\n");
		exit(1);
	}
	JSRuntime*rt;
	JSContext*ctx;
	rt=JS_NewRuntime();
	ctx=JS_NewContextRaw(rt);
	JS_AddIntrinsicBaseObjects(ctx);
	JS_AddIntrinsicDate(ctx);
	JS_AddIntrinsicEval(ctx);
	JS_AddIntrinsicStringNormalize(ctx);
	JS_AddIntrinsicRegExp(ctx);
	JS_AddIntrinsicJSON(ctx);
	JS_AddIntrinsicProxy(ctx);
	JS_AddIntrinsicMapSet(ctx);
	JS_AddIntrinsicTypedArrays(ctx);
	JS_AddIntrinsicPromise(ctx);
	js_std_init_handlers(rt);
        JS_SetModuleLoaderFunc(rt,NULL,js_module_loader,NULL);
        js_std_add_helpers(ctx,0,NULL);
        js_init_module_std(ctx,"std");
        js_init_module_os(ctx,"os");
	{
		const char*scr="import * as std from 'std';import * as os from 'os';globalThis.std=std;globalThis.os=os;";
		JSValue ret=JS_Eval(ctx,scr,strlen(scr),"<evalScript>",JS_EVAL_TYPE_MODULE);
		if(JS_IsException(ret)){
			js_std_dump_error(ctx);
			JS_ResetUncatchableError(ctx);
		}
	}
	{
		JSValue global_obj,sg;
		global_obj=JS_GetGlobalObject(ctx);
		sg=JS_NewObject(ctx);
		JS_SetPropertyStr(ctx,sg,"foo",JS_NewInt64(ctx,(int64_t)42));
		JS_SetPropertyStr(ctx,sg,"nop",JS_NewCFunction(ctx,js_nop,"nop",1));
		JS_SetPropertyStr(ctx,sg,"add",JS_NewCFunction(ctx,js_add,"add",1));
		JS_SetPropertyStr(ctx,global_obj,"test",sg);
		JS_FreeValue(ctx,global_obj);
	}
	{
		JSValue ret=JS_Eval(ctx,scr,strlen(scr),"<evalScript>",JS_EVAL_TYPE_MODULE);
		if(JS_IsException(ret)){
			js_std_dump_error(ctx);
			JS_ResetUncatchableError(ctx);
		}
	}
	if(scr!=NULL)free(scr);
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);
	exit(0);
}
