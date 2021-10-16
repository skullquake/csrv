#include<stdio.h>
#include"mongoose.h"
extern int val;
void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	mg_printf(c,"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"val:%d",val);
	mg_http_printf_chunk(c,"");//end
}
