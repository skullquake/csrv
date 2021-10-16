#include<stdio.h>
#include"mongoose.h"
extern int acc;
void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\n\r\n");
	/*
	mg_http_printf_chunk(c,"<table>");
	for(int i=0;i<8;i++){
		mg_http_printf_chunk(c,"<tr>");
		for(int j=0;j<8;j++){
			mg_http_printf_chunk(c,"<td>");
			mg_http_printf_chunk(c,"[%d,%d]",i,j);
			mg_http_printf_chunk(c,"</td>");
		}
		mg_http_printf_chunk(c,"</tr>");
	}
	mg_http_printf_chunk(c,"</table>");
	*/
	mg_http_printf_chunk(c,"acc:%d",acc);
	acc++;
	mg_http_printf_chunk(c,"");//end
}
