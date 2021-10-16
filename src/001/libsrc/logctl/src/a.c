#include<stdio.h>
#include"mongoose.h"
#include"map.h"
void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"<!DOCTYPE html><html lang=\"en-US\"><head><meta charset=\"utf-8\"><style>*{background:#000000;color:#00FF00;white-space:pre;font-family:monospace;}</style></head><body>");
	mg_http_printf_chunk(c,"<h3>Current: %s</h3>",mg_log_get());
	mg_http_printf_chunk(c,"<a href=\"?level=%d\">Set to %d [disable logging]</a><br/>",0,0);
	mg_http_printf_chunk(c,"<a href=\"?level=%d\">Set to %d [log errors only]</a><br/>",1,1);
	mg_http_printf_chunk(c,"<a href=\"?level=%d\">Set to %d [log errors and info messages]</a><br/>",2,2);
	mg_http_printf_chunk(c,"<a href=\"?level=%d\">Set to %d [log errors, info and debug messages]</a><br/>",3,3);
	mg_http_printf_chunk(c,"<a href=\"?level=%d\">Set to %d [log everything]</a><br/>",4,4);
	{
		static char buf[32];
		struct mg_http_message*hm=(struct mg_http_message*)ev_data;
		mg_http_get_var(&hm->query, "level",buf,sizeof(buf));
		if(strlen(buf)>0){
			mg_log_set(buf);
		}else{
			mg_http_printf_chunk(c,"<div>level not specified</div>\n");
		}
	}
	mg_http_printf_chunk(c,"</body></html>");
	mg_http_printf_chunk(c,"");//end
}
