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
#ifdef _WIN32
#endif
#ifndef _WIN32
#include<sys/mman.h>
#endif
#include"mongoose.h"
static int reqidx=0;
static void ctor();
static void dtor();
void __attribute__((constructor))ctor();
void __attribute__((destructor))dtor();
static void ctor(){
	reqidx=0;
	//printf("ctor:beg:%p\n",ctor);
	//printf("ctor:end:%p\n",ctor);
}
static void dtor(){
	reqidx=0;
	//printf("dtor:beg:%p\n",dtor);
	//printf("dtor:end:%p\n",dtor);
}
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data){
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"tpl:%d\n",reqidx++);
#ifndef _WIN32
	/*
	int fd = open("/proc/uptime", O_RDONLY);
	int len = lseek(fd, 0, SEEK_END);
	void *buf= mmap(0,len,PROT_READ,MAP_PRIVATE,fd,0);
	if(len==0||buf==NULL){
		mg_http_printf_chunk(c,"error: /proc/uptime not found\n");
	}else{
		//mg_http_write_chunk(c,(char*)buf,len);
		_http_printf_chunk(c,"ok\n");
		close(fd);
	}
	*/
	/*
1
10
1017
1018
1019
1021
105351
11
111
111457
111458
111648
111829
111866
111894
13
14
1477
15
16
17
171129
18
19
2
20
21
22
23
24
256
257
258
283
285
286
30
31
32
33
357
358
367
368
383
384
4
41
42
43
43299
44
446
45
464
481
494
50787
50789
513
53425
53802
53839
54487
547
548
55073
55237
55240
55290
55385
55386
55387
55393
55394
55395
55396
572
58
592
593
594
599
6
619
624
627
629
7
8
9
925
927
930
932
936
937
acpi
buddyinfo
bus
cgroups
cmdline
consoles
cpuinfo
crypto
devices
diskstats
dma
driver
execdomains
fb
filesystems
fs
interrupts
iomem
ioports
irq
kallsyms
kcore
keys
key-users
kmsg
kpagecount
kpageflags
loadavg
locks
mdstat
meminfo
misc
modules
mounts
mtrr
net
pagetypeinfo
partitions
sched_debug
schedstat
scsi
self
slabinfo
softirqs
stat
swaps
sys
sysrq-trigger
sysvipc
timer_list
timer_stats
tty
uptime
version
vmallocinfo
vmstat
zoneinfo
	*/
	//uptime
	{
		FILE*fp=fopen("/proc/uptime", "r");
		char ch;
		if(fp==NULL){
			mg_http_printf_chunk(c,"not\n");
		}else{
			while((ch=fgetc(fp))!=EOF)mg_http_write_chunk(c,&ch,1);
			mg_http_write_chunk(c,"\n",1);
			fclose(fp);
		}
	}
	{//meminfo
		FILE*fp=fopen("/proc/meminfo", "r");
		char ch;
		if(fp==NULL){
			mg_http_printf_chunk(c,"not\n");
		}else{
			while((ch=fgetc(fp))!=EOF)mg_http_write_chunk(c,&ch,1);
			mg_http_write_chunk(c,"\n",1);
			fclose(fp);
		}
	}
	{//cpuinfo
		FILE*fp=fopen("/proc/cpuinfo", "r");
		char ch;
		if(fp==NULL){
			mg_http_printf_chunk(c,"not\n");
		}else{
			while((ch=fgetc(fp))!=EOF)mg_http_write_chunk(c,&ch,1);
			mg_http_write_chunk(c,"\n",1);
			fclose(fp);
		}
	}
	{//stat
		FILE*fp=fopen("/proc/stat", "r");
		char ch;
		if(fp==NULL){
			mg_http_printf_chunk(c,"not\n");
		}else{
			while((ch=fgetc(fp))!=EOF)mg_http_write_chunk(c,&ch,1);
			mg_http_write_chunk(c,"\n",1);
			fclose(fp);
		}
	}


#endif
#ifdef _WIN32
	mg_http_printf_chunk(c,"unimplemented\n");
#endif
	mg_http_printf_chunk(c,"");//end
}

