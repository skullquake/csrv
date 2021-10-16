#include"./hdl.h"
extern struct mg_serve_http_opts s_http_server_opts;
void ev_handler(struct mg_connection*c,int ev,void *p){
	if(ev==MG_EV_HTTP_REQUEST){
		mg_serve_http(c,(struct http_message*)p,s_http_server_opts);
	}
}
