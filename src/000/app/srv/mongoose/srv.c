#include"mongoose.h"
#include"./hdl/hdl.h"
#include"./hdl/home/home.h"
#ifdef _WIN32
#define POLLTIME 100
#else
#define POLLTIME 1
#endif
const char*s_http_port="8999";
struct mg_serve_http_opts s_http_server_opts;
int mongoose_run(int argc,char*argv[]){
	struct mg_mgr mgr;
	struct mg_connection*c;
	mg_mgr_init(&mgr,NULL);
	printf("Starting web server on port %s\n",s_http_port);
	c=mg_bind(&mgr,s_http_port,ev_handler);
	mg_register_http_endpoint(c,"/home",hdl_home);
	mg_set_protocol_http_websocket(c);
	s_http_server_opts.document_root=".";
	s_http_server_opts.cgi_file_pattern="**.ps1|**.py|**.cgi|**.bat|**.sh|**.exe$|**.lua$|**.out$";
#ifdef _WIN32
	//s_http_server_opts.cgi_interpreter="C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe -File ";
	//s_http_server_opts.cgi_interpreter="C:\\Windows\\System32\\cmd.exe /c ";
#endif
	s_http_server_opts.enable_directory_listing="yes";
	int pollidx=0;
	for(;;){
		//printf("POLLIDX:%d\n",pollidx++);
		mg_mgr_poll(&mgr,POLLTIME);
	}
	mg_mgr_free(&mgr);
	return 0;
}
