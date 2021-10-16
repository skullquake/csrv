#include<stdio.h>
#include<dirent.h>
#include"mongoose.h"
#include<string.h>
static struct mg_connection*c;
void list(const char*path){
        struct stat statbuf;
        stat(path,&statbuf);
        char tbuf[80];
        //time_t t=time(NULL);
        struct tm*p=localtime(&statbuf.st_mtime);
        strftime(tbuf,1000,"%m-%d-%Y %H:%M:%S",p);
	if(statbuf.st_mode&S_IFCHR)mg_http_printf_chunk(c,"c");
	else if(statbuf.st_mode&S_IFCHR)mg_http_printf_chunk(c,"c");
	else if(statbuf.st_mode&S_IFDIR)mg_http_printf_chunk(c,"d");
	else if(statbuf.st_mode&S_IFIFO)mg_http_printf_chunk(c,"p");
	else if(statbuf.st_mode&S_IFBLK)mg_http_printf_chunk(c,"b");
	else if(statbuf.st_mode&S_IFREG)mg_http_printf_chunk(c,"f");
	else if(statbuf.st_mode&S_IREAD)mg_http_printf_chunk(c,"r");
	else mg_http_printf_chunk(c," ");
	mg_http_printf_chunk(c,(statbuf.st_mode&S_IWRITE?"w":"-"));
	mg_http_printf_chunk(c,(statbuf.st_mode&S_IEXEC?"x":"-"));
	mg_http_printf_chunk(c,"------");
	mg_http_printf_chunk(c,"  %d",statbuf.st_nlink);
	mg_http_printf_chunk(c,"  dosuser  root");
	mg_http_printf_chunk(c,"  %8d",statbuf.st_size);
	mg_http_printf_chunk(c,"  %s",tbuf);
	mg_http_printf_chunk(c,"  %s",path);
	mg_http_printf_chunk(c,"\n");
}
void ls(char*path){
        if(path!=NULL){
                struct dirent*de;
                DIR *dr=opendir(path);
                if(dr==NULL){
			mg_http_printf_chunk(c,"Could not open current directory\n");
                }
                while((de=readdir(dr))!=NULL){
			char*fullpath=(char*)malloc(strlen(path)+1+strlen(de->d_name));
			fullpath[0]='\0';
			strcat(fullpath,path);
			strcat(fullpath,de->d_name);
			list(fullpath);
			free(fullpath);
                }
                closedir(dr);
        }
        return;
}
void entry(struct mg_connection*cp,int ev,void*ev_data,void*fn_data){
	c=cp;
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n");
	ls("./");
	ls("./bin/");
	ls("./lib/");
	ls("./libsrc/");
	ls("./www/");
	mg_http_printf_chunk(c,"");//end
}
