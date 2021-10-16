#include<stdio.h>
#include"mongoose.h"
extern int running;
void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"stopping server...");
	mg_http_printf_chunk(c,"");//end
	running=0;
}
