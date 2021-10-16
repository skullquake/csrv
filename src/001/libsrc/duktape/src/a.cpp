//https://duktape.org/
#include"util/util.h"
#include<stdio.h>
#include"duktape/duktape.h"
#include"duktape/extras/console/duk_console.h"
#include"mongoose.h"
static int reqidx=0;
EXTERN_C_BEG
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
EXTERN_C_END
static duk_ret_t native_print(duk_context *ctx) {
  printf("%s\n", duk_to_string(ctx, 0));
  return 0;  /* no return value (= undefined) */
}
/* Adder: add argument values. */
static duk_ret_t native_adder(duk_context *ctx) {
  int i;
  int n = duk_get_top(ctx);  /* #args */
  double res = 0.0;

  for (i = 0; i < n; i++) {
    res += duk_to_number(ctx, i);
  }

  duk_push_number(ctx, res);
  return 1;  /* one return value */
}
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	duk_context*ctx=duk_create_heap_default();
	duk_console_init(ctx,
		  DUK_CONSOLE_PROXY_WRAPPER
		//||DUK_CONSOLE_FLUSH
		//||DUK_CONSOLE_STDOUT_ONLY
		//||DUK_CONSOLE_STDERR_ONLY
	);
	duk_push_c_function(ctx, native_print, 1 /*nargs*/);
	duk_put_global_string(ctx, "print");
	duk_push_c_function(ctx, native_adder, DUK_VARARGS);
	duk_put_global_string(ctx, "adder");
	duk_push_c_function(ctx, [](duk_context *ctx)->duk_ret_t {
		duk_push_number(ctx,42);
		return 1;
	}, DUK_VARARGS);
	duk_put_global_string(ctx, "magick");
	duk_push_c_function(ctx,[](duk_context *ctx)->duk_ret_t {
		struct mg_connection*c=(struct mg_connection*)duk_get_pointer(ctx,0);
		const char*fmt;
		fmt=duk_get_string(ctx,1);
		if(fmt){
			mg_printf(c,fmt);
		}
		return 0;

	},2);
	duk_put_global_string(ctx,"mg_printf");
	duk_push_c_function(ctx,[](duk_context *ctx)->duk_ret_t {
		struct mg_connection*c=(struct mg_connection*)duk_get_pointer(ctx,0);
		const char*fmt;
		fmt=duk_get_string(ctx,1);
		if(fmt){
			mg_http_printf_chunk(c,fmt);
		}
		return 0;

	},2);
	duk_put_global_string(ctx,"mg_http_printf_chunk");
	duk_push_pointer(ctx,(void*)c);
	duk_put_global_string(ctx,"c");
	const char*unsafe_code=R"(
		mg_printf(c,'HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n');
		for(var i=0;i<8;i++){
			for(var j=0;j<8;j++){
				mg_http_printf_chunk(c,'['+i+']['+j+']');
			}
			mg_http_printf_chunk(c,'\n');
		}
		mg_http_printf_chunk(c,'');
	)"
	;
	duk_push_string(ctx,unsafe_code);
	if(duk_peval(ctx)){
		mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n");
		mg_http_printf_chunk(c,"Unexpected error: %s\n",duk_safe_to_string(ctx,-1));
		mg_http_printf_chunk(c,"");
	}else{
	}
	duk_destroy_heap(ctx);
}
