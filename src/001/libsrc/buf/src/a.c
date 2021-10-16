#include<stdio.h>
#include"mongoose.h"
void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"<!DOCTYPE html><html lang=\"en-US\"><head><title>Index</title><meta charset=\"utf-8\"><style>*{background:#000000;color:#00FF00;font-family:monospace;}</style></head><body><pre>");
	mg_http_printf_chunk(c,"buf:\n");
	char**buf=NULL;
	unsigned int bufsz=32;
	mg_http_printf_chunk(c,"allocating\n");
	buf=(char**)malloc(bufsz*sizeof(char*));
	if(buf!=NULL){
		mg_http_printf_chunk(c,"allocated\n");
		mg_http_printf_chunk(c,"populating\n");
		for(int i=0;i<bufsz;i++)*(buf+i)=NULL;
		for(int i=0;i<bufsz;i++){
			int bufsz2=32;
			mg_http_printf_chunk(c,"allocating[%d]...",i);
			buf[i]=(char*)malloc(bufsz2*sizeof(char*));
			if(buf[i]){
				mg_http_printf_chunk(c,"done\n");
			}else{
				mg_http_printf_chunk(c,"failed\n");
			}
		}
		for(int i=0;i<bufsz;i++){
			mg_http_printf_chunk(c,"deallocating[%d]...",i);
			if(buf[i]!=NULL){
				free(buf[i]);
				mg_http_printf_chunk(c,"done\n");
			}else{
				mg_http_printf_chunk(c,"skipping\n");
			}
		}
		//snprintf(buf,bufsz,"testdata");
		//mg_http_printf_chunk(c,"%s\n",buf);
		mg_http_printf_chunk(c,"freeing\n");
		free(buf);
	}else{
		mg_http_printf_chunk(c,"failed\n");
	}
	mg_http_printf_chunk(c,"done\n");
	mg_http_printf_chunk(c,"</pre></body></html>");
	mg_http_printf_chunk(c,"");//end
}
