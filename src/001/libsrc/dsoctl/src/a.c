#include<stdio.h>
#include"mongoose.h"
#include"map.h"
extern hashmap*mglb;
extern hashmap*mloc;
static void p(void*key,size_t ksize,uintptr_t*value,void*usr);
static int reqidx=9;
void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"req:%d\n",reqidx++);
	mg_http_printf_chunk(c,"dsoctl:\n");
	mg_http_printf_chunk(c,"global:\n");
	hashmap_iterate(mglb,p,c);
	mg_http_printf_chunk(c,"local:\n");
	hashmap_iterate(mloc,p,c);
	mg_http_printf_chunk(c,"");//end
}
static void p(void*key,size_t ksize,uintptr_t*value,void*c){
	//printf("%s:%p\n",key,(void*)value);
//	mg_http_printf_chunk(c,"%s:%p",key,(void*)value);
	mg_http_printf_chunk((struct mg_connection*)c,"%-16s: %p\n",key,(void*)value);
}
