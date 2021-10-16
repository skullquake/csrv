#include"util/util.h"
#include<stdio.h>
#include"mongoose.h"
static int reqidx=0;
EXTERN_C_BEG
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
EXTERN_C_END
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"tpl:%d",reqidx++);
	mg_http_printf_chunk(c,"");//end
}
