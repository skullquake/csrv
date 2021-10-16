#include<dirent.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include"mongoose.h"
#include"map.h"
#include"./dlfcn/dlfcn.h"
#ifdef _WIN32
#define DSOEXT ".dll"
#define PORT "8001"
#endif
#ifndef _WIN32
#define DSOEXT ".so"
#define PORT "8000"
#endif

struct DSOEntry{
	void*hdl;
	void*usr;
}typedef DSOEntry;


int running=true;//bad
int val;//ideally in other modules
hashmap*mglb;//bad
hashmap*mloc;//bad
void*context;//passed in when event handler is called

//static const char*s_web_root_dir="./www,/alt=./www2";//does not work
static const char*s_web_root_dir="./www";
static const char*s_listening_address="http://0.0.0.0:"PORT;
static const char*s_dsouripfx="";
static const char*s_dsopath="./www";

static void ls(char*path);
static void*lodlib(const char*path);
static void lodlibs(const char*path);
static int endswith(const char*str,const char*suffix);
static int begswith(const char*str,const char*prefix);
static void cb(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
static void hdl_dso(struct mg_connection*c,int ev,void*ev_data,void*fn_data,char*,struct mg_http_serve_opts*);
static void print_entry(void*key,size_t ksize,uintptr_t*value,void*usr);
static void free_map_hdl_entry(void* key,size_t ksize,uintptr_t value,void*usr);
static void free_map_dso_entry(void* key,size_t ksize,uintptr_t value,void*usr);
static void ov_free(void*key,size_t ksize,uintptr_t value,void*usr);
int appmain(int argc,char*argv[]){
	printf("appmain:beg\n");
	printf("appmain:initializing dso map...\n");
	mglb=hashmap_create();
	mloc=hashmap_create();
	printf("appmain:loading dsos...\n");
	lodlibs("./lib/");
	hashmap_iterate(mglb,(hashmap_callback)print_entry, NULL);
	//ls("./lib/");//does not work under mingw
	printf("appmain:starting server...\n");
	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	struct mg_connection*c=mg_http_listen(&mgr,s_listening_address,cb,context);//&mgr);
	mg_log_set("3");
	if(c!=NULL){
		while(running)mg_mgr_poll(&mgr,1000);
	}else{
		printf("appmain:failed to start server...\n");
	}
	printf("appmain:stopping server...\n");
	mg_mgr_free(&mgr);
	printf("appmain:unloading dsos...\n");
	hashmap_iterate(mglb,free_map_hdl_entry, NULL);
	hashmap_iterate(mloc,free_map_dso_entry, NULL);
	printf("appmain:freeing map...\n");
	hashmap_free(mglb);
	hashmap_free(mloc);
	printf("appmain:end\n");
	return 0;
}
static void cb(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
/*
// Parameter for mg_http_serve_dir()
struct mg_http_serve_opts {
  const char *root_dir;       // Web root directory, must be non-NULL
  const char *ssi_pattern;    // SSI file name pattern, e.g. #.shtml
  const char *extra_headers;  // Extra HTTP headers to add in responses
  const char *mime_types;     // Extra mime types, ext1=type1,ext2=type2,..
  struct mg_fs *fs;           // Filesystem implementation. Use NULL for POSIX
};

*/
	const char*extra_headers=
		"Access-Control-Allow-Origin: *\r\n"
		//"Access-Control-Allow-Headers: X-PINGOTHER, Content-Type, Authorization\r\n"
		"Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
		"Access-Control-Allow-Headers: *\r\n"
	;
	struct mg_http_serve_opts opts={
		.root_dir=s_web_root_dir
		,.ssi_pattern="#.shtml"
		,.extra_headers=extra_headers
	};
	if(ev==MG_EV_HTTP_MSG){
		struct mg_http_message*hm=(struct mg_http_message*)ev_data;
		char*path=(char*)malloc(hm->uri.len+1);
		LOG(LL_INFO,("PATH: %.*s",hm->uri.len,hm->uri.ptr));
		if(!path)return;
		memcpy(path,hm->uri.ptr,hm->uri.len);
		path[hm->uri.len]='\0';
		//if(mg_http_match_uri(hm,"/asdf*"))
		if(begswith(path,s_dsouripfx)){
			hdl_dso(c,ev,ev_data,fn_data,path,&opts);
		}else{
			mg_http_serve_dir(c,ev_data,&opts);
		}
		free(path);
	}
}
static void hdl_dso(struct mg_connection*c,int ev,void*ev_data,void*fn_data,char*path,struct mg_http_serve_opts*opts){
	char*dsopath;
	if(strchr(path,'.')!=NULL||endswith(path,DSOEXT)){
		dsopath=(char*)malloc(strlen(s_dsopath)+strlen(path)-strlen(s_dsouripfx)+1);
	}else{
		dsopath=(char*)malloc(strlen(s_dsopath)+strlen(path)-strlen(s_dsouripfx)+strlen(DSOEXT)+1);
	}


	if(!dsopath){
		mg_http_reply(c,200,"","ealloc");
		return;
	}
	dsopath[0]='\0';
	strcat(dsopath,s_dsopath);
	strcat(dsopath,path+strlen(s_dsouripfx));
	if(strchr(path,'.')==NULL&&!endswith(path,DSOEXT))strcat(dsopath,DSOEXT);
	if(access(dsopath,F_OK)==0){
		if(endswith(dsopath,DSOEXT)){
			exlib(dsopath,c,ev,ev_data,fn_data);
		}else{
			mg_http_serve_dir(c,ev_data,opts);
		}
	}else{
		mg_http_serve_dir(c,ev_data,opts);
	}
	free(dsopath);
}
int exlib(const char*path,struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	/*
	//----------------------------------------
	//http://localhost:8001/tpl?flush=false
	//----------------------------------------
	 *nix: bombardier -c 1 http://localhost:8000/tpl
Statistics        Avg      Stdev        Max
  Reqs/sec      2952.82     134.05    3137.64
  Latency      334.99us    61.46us     5.72ms
  HTTP codes:
    1xx - 0, 2xx - 29526, 3xx - 0, 4xx - 0, 5xx - 0
    others - 0
  Throughput:   444.06KB/s
	 *nix: bombardier http://localhost:8000/tpl
tatistics        Avg      Stdev        Max
  Reqs/sec      3560.79     980.09    5621.11
  Latency       35.06ms    29.22ms      1.15s
  HTTP codes:
    1xx - 0, 2xx - 35720, 3xx - 0, 4xx - 0, 5xx - 0
    others - 0
  Throughput:   535.74KB/s
	 *mingw: bombardier -c 1 http://localhost:8000/tpl
Statistics        Avg      Stdev        Max
  Reqs/sec      2166.31     128.17    2882.00
  Latency      454.87us    42.41us     4.04ms
  HTTP codes:
    1xx - 0, 2xx - 21672, 3xx - 0, 4xx - 0, 5xx - 0
    others - 0
  Throughput:   275.11KB/s
	 *mingw: bombardier http://localhost:8000/tpl
Statistics        Avg      Stdev        Max
  Reqs/sec     18449.67   13070.01   32733.47
  Latency        6.76ms    77.48ms      6.01s
  HTTP codes:
    1xx - 0, 2xx - 23464, 3xx - 0, 4xx - 0, 5xx - 0
    others - 161187
  Errors:
    dial tcp 127.0.0.1:8000: connect: connection refused - 161187
  Throughput:   298.35KB/s
	//----------------------------------------
	//http://localhost:8001/tpl
	//http://localhost:8001/tpl?flush=false
	//----------------------------------------
	 *nix: bombardier http://localhost:8000/tpl
tatistics        Avg      Stdev        Max
  Reqs/sec     31905.06    3364.57   39244.04
  Latency        3.91ms     1.01ms   110.61ms
  HTTP codes:
    1xx - 0, 2xx - 319103, 3xx - 0, 4xx - 0, 5xx - 0
    others - 0
  Throughput:     4.68MB/s
	 *nix: bombardier -c 1 http://localhost:8000/tpl
Statistics        Avg      Stdev        Max
  Reqs/sec     12374.15     559.06   13173.71
  Latency       78.32us    27.70us     5.37ms
  HTTP codes:
    1xx - 0, 2xx - 123734, 3xx - 0, 4xx - 0, 5xx - 0
    others - 0
  Throughput:     1.82MB/s
	 *mingw: bombardier http://localhost:8000/tpl
Statistics        Avg      Stdev        Max
  Reqs/sec     35409.93    7525.83   46895.17
  Latency        3.53ms    54.44ms      6.02s
  HTTP codes:
    1xx - 0, 2xx - 205276, 3xx - 0, 4xx - 0, 5xx - 0
    others - 148774
  Errors:
    dial tcp 127.0.0.1:8000: connect: connection refused - 148774
  Throughput:     2.55MB/s
	 *mingw: bombardier -c http://localhost:8000/tpl
Statistics        Avg      Stdev        Max
  Reqs/sec      9830.23     715.50   18115.94
  Latency       99.07us    30.04us     5.02ms
  HTTP codes:
    1xx - 0, 2xx - 98152, 3xx - 0, 4xx - 0, 5xx - 0
    others - 0
  Throughput:     1.22MB/s
	 */
	struct mg_http_message*hm=(struct mg_http_message*)ev_data;
	int flush=0;
	{
		char buf[32];
		mg_http_get_var(&hm->query, "flush",buf,sizeof(buf));
		if(strcmp(buf,"true")==0){
			flush=1;
		}
	}
	char*path2;
	if(endswith(path,DSOEXT)){
		path2=(char*)malloc(strlen(path)+1);
	}else{
		path2=(char*)malloc(strlen(path)+strlen(DSOEXT)+1);
	}
	//mg_http_reply(c,200,"","skip");	//test
	//return;				//test
	if(!path2){
		fprintf(stderr,"exlib:ealloc:\n");
		mg_http_reply(c,200,"","exlib:ealloc\n");
		return-1;
	}
	path2[0]='\0';
	strcat(path2,path);
	if(!endswith(path,DSOEXT))strcat(path2,DSOEXT);
	//cache control
	//exlib(dsopath,c,ev,ev_data,fn_data);
	if(flush){
		DSOEntry*entry=NULL;
		if(hashmap_get(mloc,path2,strlen(path2),&entry)){
			dlclose(entry->hdl);
			//printf("removing from cache...\n");
			hashmap_remove(mloc,path2,strlen(path2));
		}
	}
	//cache control - end
	void*handle=NULL;
	//printf("testing cache %s...\n",path2);
	void(*sym)(struct mg_connection*,int,void*,void*);
	DSOEntry*entry=NULL;
	if(!hashmap_get(mloc,path2,strlen(path2),&entry)){
		//printf("caching %s...\n",path2);
		handle=dlopen(path2,RTLD_LAZY|RTLD_GLOBAL);//LAZY GLOBAL NOW
		if(!handle){
			mg_http_reply(c,200,"","exlib:elod:%s\n",dlerror());
			free(path2);
			return-1;
		}
		dlerror();//reset
		sym=(void(*)(struct mg_connection*,int,void*,void*))dlsym(handle,"entry");
		if(!sym){
			mg_http_reply(c,200,"","exlib:esym:%s\n",dlerror());
			return-1;
		}
		//skull
		if(!flush){
			//printf("not caching\n");
			char*path3=(char*)malloc(strlen(path2)+1);
			if(!path3){
				free(path2);
				mg_http_reply(c,200,"","exlib:esym:%s\n",dlerror());
				return-1;
			}
			path3[0]='\0';
			strcat(path3,path2);
			entry=(DSOEntry*)malloc(sizeof(DSOEntry));
			if(!entry){
				free(path2);
				free(path3);
				mg_http_reply(c,200,"","exlib:eent:%s\n",dlerror());
				return-1;
			}
			entry->hdl=handle;
			entry->usr=sym;
			hashmap_set_free(mloc,(void*)path3,strlen(path3),(uintptr_t)entry,ov_free,(void*)path3);
			//free(entry);
		}
	}else{
		//printf("cached %s...\n",path2);
		sym=(void(*)(struct mg_connection*,int,void*,void*))(entry->usr);
	}
	sym(c,ev,ev_data,fn_data);
	dlerror();
	if(flush){
		dlclose(handle);
	}
	free(path2);
	return 0;
}
static void*lodlib(const char*path){
	printf("loading %s...\n",path);
	char*path2;
	if(endswith(path,DSOEXT)){
		path2=(char*)malloc(strlen(path)+1);
	}else{
		path2=(char*)malloc(strlen(path)+strlen(DSOEXT)+1);
	}
	if(!path2){
		fprintf(stderr,"exlib:ealloc:\n");
		return NULL;
	}
	path2[0]='\0';
	strcat(path2,path);
	if(!endswith(path,DSOEXT))strcat(path2,DSOEXT);
	void*handle=dlopen(path2,RTLD_LAZY|RTLD_GLOBAL);//LAZY GLOBAL NOW
	if(!handle){
		fprintf(stderr,"exlib:elod:%s\n",dlerror());
		free(path2);
		return NULL;
	}
	dlerror();//reset
	free(path2);
	//dlerror();//reset
	//dlclose(handle);
	return handle;
}
static void lodlibs(const char*path){
	printf("lodlibs:beg\n");
        if(path!=NULL){
                struct dirent*entry;
                DIR*dr=opendir(path);
                if(dr==NULL){
			fprintf(stderr,"Could not open current directory\n");
                }
                while((entry=readdir(dr))){
			//entry->d_name not reading in correctly under mingw
			char*fullpath=(char*)malloc(strlen(path)+1+strlen(entry->d_name));
			fullpath[0]='\0';
			strcat(fullpath,path);
			strcat(fullpath,entry->d_name);
			if(endswith(fullpath,DSOEXT)){
				void*handle=lodlib(fullpath);
				if(handle!=NULL)hashmap_set_free(mglb,(void*)fullpath,strlen(fullpath),(uintptr_t)handle,ov_free,(void*)fullpath);
			}else{
			}
			//free(fullpath);
                }
                closedir(dr);
        }
	printf("lodlibs:end\n");
}
int endswith(const char*str,const char*suffix){
	if(!str||!suffix)return 0;
	size_t lenstr=strlen(str);
	size_t lensuffix=strlen(suffix);
	if(lensuffix>lenstr)return 0;
	return strncmp(str+lenstr-lensuffix,suffix,lensuffix)==0;
}
int begswith(const char*str,const char*prefix){
	if(!str||!prefix)return 0;
	size_t lenstr=strlen(str);
	size_t lenprefix=strlen(prefix);
	if(lenprefix>lenstr)return 0;
	return strncmp(str,prefix,lenprefix)==0;
}
static void print_entry(void*key,size_t ksize,uintptr_t*value,void*usr){
	printf("%s:%p\n",(char*)key,(void*)value);
}
static void free_map_hdl_entry(void*key,size_t ksize,uintptr_t val,void*usr){
	printf("free_map_hdl_entry:beg\n");
	printf("free_map_hdl_entry:val:%p\n",val);
	dlclose((void*)val);
	if(key!=usr)free(key);
	printf("free_map_hdl_entry:end\n");
}
static void free_map_dso_entry(void*key,size_t ksize,uintptr_t val,void*usr){
	printf("free_map_dso_entry:beg\n");
	DSOEntry*entry=(DSOEntry*)val;
	printf("free_map_hdl_entry:val:%p\n",entry);
	printf("free_map_hdl_entry:val:hdl:%p\n",entry->hdl);
	printf("free_map_hdl_entry:val:usr:%p\n",entry->usr);
	dlclose(entry->hdl);
	if(key!=usr)free(key);
	printf("free_map_dso_entry:end");
}
static void ov_free(void*key,size_t ksize,uintptr_t value,void*usr){
	if(key!=usr)free(key);
}
static void ls(char*path){
        if(path!=NULL){
                struct dirent*de;
                DIR *dr=opendir(path);
                if(dr==NULL){
			fprintf(stderr,"Could not open current directory\n");
                }
                while((de=readdir(dr))!=NULL){
			char*fullpath=(char*)malloc(strlen(path)+1+strlen(de->d_name));
			fullpath[0]='\0';
			strcat(fullpath,path);
			strcat(fullpath,de->d_name);
			printf("%s\n",fullpath);
			free(fullpath);
                }
                closedir(dr);
        }
        return;
}

