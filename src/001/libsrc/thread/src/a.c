#if defined _WIN32 || defined __CYGWIN__
	#ifdef BUILDING_DLL
		#ifdef __GNUC__
			#define DLL_PUBLIC __attribute__ ((dllexport))
		#else
			#define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
		#endif
	#else
		#ifdef __GNUC__
			#define DLL_PUBLIC __attribute__ ((dllimport))
		#else
			#define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
		#endif
	#endif
	#define DLL_LOCAL
#else
	#if __GNUC__ >= 4
		#define DLL_PUBLIC __attribute__ ((visibility ("default")))
		#define DLL_LOCAL	__attribute__ ((visibility ("hidden")))
	#else
		#define DLL_PUBLIC
		#define DLL_LOCAL
	#endif
#endif
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
//#include<stdatomic.h>
#include"mongoose.h"
static int reqidxthread=0;
static void ctor();
static void dtor();
void __attribute__((constructor))ctor();
void __attribute__((destructor))dtor();
static void ctor(){
	reqidxthread=0;
	//printf("ctor:beg:%p\n",ctor);
	//printf("ctor:end:%p\n",ctor);
}
static void dtor(){
	reqidxthread=0;
	//printf("dtor:beg:%p\n",dtor);
	//printf("dtor:end:%p\n",dtor);
}
#define NTHREADS 8
static int running=0;
static pthread_mutex_t lock;
static useconds_t period=1000000;
static pthread_t tbuf[NTHREADS];
void*tf0(void*data){
	printf("thread:%p:beg\n",pthread_self());
	pthread_mutex_init(&lock,NULL);//!=0 check
	int acc=0;
	int tidx=0;
	for(;tidx<NTHREADS;tidx++){
		if(pthread_equal(pthread_self(),tbuf[tidx]))break;
	}
	while(running){
		pthread_mutex_lock(&lock);
		printf("thread:[%d]:%p:%d:%p:%s\n",tidx,pthread_self(),reqidxthread,data,(char*)data);
		reqidxthread++;
		acc++;
		pthread_mutex_unlock(&lock);
		usleep(period);
	}
	free(data);
	printf("thread:%p:end\n",pthread_self());
	int*ret=(int*)malloc(sizeof(int));
	*ret=acc;
	pthread_exit((void*)ret);
}
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	printf("entry:%p:beg\n",pthread_self());
	struct mg_http_message*hm=(struct mg_http_message*)ev_data;
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"<style>*{background:#000000;color:#888888;}</style>");
	mg_http_printf_chunk(c,"<h3>");
	mg_http_printf_chunk(c,"Threading");
	mg_http_printf_chunk(c,"</h3>");
	mg_http_printf_chunk(c,"<hr/>");
	mg_http_printf_chunk(c,"<div>");
	mg_http_printf_chunk(c,"<a href=\"?\">refresh</a><br/>");
	mg_http_printf_chunk(c,"<a href=\"?fn=stop\">stop</a><br/>");
	mg_http_printf_chunk(c,"<a href=\"?fn=start\">start</a><br/>");
	mg_http_printf_chunk(c,"<a href=\"?fn=setperiod&period=1000\">1000</a><br/>");
	mg_http_printf_chunk(c,"<a href=\"?fn=setperiod&period=5000\">5000</a><br/>");
	mg_http_printf_chunk(c,"<a href=\"?fn=setperiod&period=10000\">10000</a><br/>");
	mg_http_printf_chunk(c,"<a href=\"?fn=setperiod&period=50000\">50000</a><br/>");
	mg_http_printf_chunk(c,"<a href=\"?fn=setperiod&period=100000\">100000</a><br/>");
	mg_http_printf_chunk(c,"<a href=\"?fn=setperiod&period=500000\">500000</a><br/>");
	mg_http_printf_chunk(c,"<a href=\"?fn=setperiod&period=1000000\">1000000</a><br/>");
	mg_http_printf_chunk(c,"<a href=\"?fn=setperiod&period=2000000\">2000000</a><br/>");
	for(int i=0;i<NTHREADS;i++){
		if(tbuf[i]!=NULL)mg_http_printf_chunk(c,"<a href=\"?fn=cancel&tid=%d\">cancel thread %d</a><br/>",i,i);
	}
	mg_http_printf_chunk(c,"</div>");
	mg_http_printf_chunk(c,"<hr/>");
	mg_http_printf_chunk(c,"<pre>");
	mg_http_printf_chunk(c,"thread:%d\n",reqidxthread);
	{
		char buf[32];
		mg_http_get_var(&hm->query,"fn",buf,sizeof(buf));
		if(strcmp(buf,"start")==0){
			mg_http_printf_chunk(c,"cmd:start\n");
			pthread_mutex_lock(&lock);
			if(!running){
				running=true;
				for(int i=0;i<NTHREADS;i++){
					char*data=(char*)malloc(sizeof(char)*8);
					snprintf(data,8,"hello");
					pthread_create(tbuf+i,NULL,tf0,data);
					//pthread_join(tid,NULL);
					mg_http_printf_chunk(c,"status:started:%p\n",tbuf+i);
				}
			}else{
				mg_http_printf_chunk(c,"error:already running\n");
			}
			pthread_mutex_unlock(&lock);
		}else if(strcmp(buf,"stop")==0){
			mg_http_printf_chunk(c,"cmd:stop\n");
			pthread_mutex_lock(&lock);
			if(running){
				running=false;
				for(int i=0;i<NTHREADS;i++){
					int*ret=NULL;
					pthread_join(tbuf[i],(void**)&ret);
					mg_http_printf_chunk(c,"status:stopped:%p:return:%i\n",tbuf+i,*ret);
					free(ret);
					tbuf[i]=NULL;
				}
			}else{
				mg_http_printf_chunk(c,"error:already stopped\n");
			}
			pthread_mutex_unlock(&lock);
		}else if(strcmp(buf,"setperiod")==0){
			mg_http_printf_chunk(c,"cmd:setperiod\n");
			mg_http_get_var(&hm->query,"period",buf,sizeof(buf));
			if(strcmp(buf,"period")!=0){
				int qperiod=atoi(buf);
				pthread_mutex_lock(&lock);
				period=qperiod;
				pthread_mutex_unlock(&lock);
				mg_http_printf_chunk(c,"status:period set to %d\n",qperiod);
			}else{
				mg_http_printf_chunk(c,"error:period not provided\n");
			}
		}else if(strcmp(buf,"cancel")==0){
			mg_http_printf_chunk(c,"cmd:cancel\n");
			mg_http_get_var(&hm->query,"tid",buf,sizeof(buf));
			if(strcmp(buf,"tid")!=0){
				int qtid=atoi(buf);
				if(qtid>=0&&qtid<NTHREADS){
					pthread_mutex_lock(&lock);
					//period=qperiod;
					pthread_cancel(tbuf[qtid]);
					tbuf[qtid]=NULL;
					pthread_mutex_unlock(&lock);
					mg_http_printf_chunk(c,"status:thread %d canceled\n",qtid);
				}else{
					mg_http_printf_chunk(c,"error:invalid thread id\n");
				}
			}else{
				mg_http_printf_chunk(c,"error:period not provided\n");
			}
		}


	}
	mg_http_printf_chunk(c,"</pre>");
	mg_http_printf_chunk(c,"");//end
	printf("entry:%p:end\n",pthread_self());

}
/*
 * create threads as JOINABLE
 *
 */
