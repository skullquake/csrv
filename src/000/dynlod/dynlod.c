#include"dynlod/dynlod.h"
#include<stdio.h>
#ifdef _WIN32
#include"windows.h"
static char errbuf[512];
void*dlopen(const char*name,int mode){
	//printf("LOADLIB:%s\n",name);
	HINSTANCE hdll;
	hdll=LoadLibrary(name);
#ifdef _WIN32
	if(!hdll){
		sprintf(errbuf,"error code %d loading library %s",GetLastError(),name);
		fprintf(stderr,"%s\n",errbuf);
		return NULL;
	}else{
	}
#else
	if((UINT)hdll<32){
		sprintf(errbuf,"error code %d loading library %s",(UINT)hdll,name);
		fprintf(stderr,"%s\n",errbuf);
		return NULL;
	}
#endif
	return (void*)hdll;
}
void*dlsym(void*lib,const char*name){
	HMODULE hdll=(HMODULE)lib;
	void *symAddr;
	symAddr=(void*)GetProcAddress(hdll,name);
	if(symAddr==NULL)
		sprintf(errbuf,"can't find symbol %s",name);
	return symAddr;
}
int dlclose(void*lib){
	HMODULE hdll=(HMODULE)lib;
#ifdef _WIN32
	//https://stackoverflow.com/questions/26482721/remove-file-locked-by-another-process
	//https://forums.codeguru.com/showthread.php?128415-FreeLibrary-Does-not-free-Resources
	FreeLibrary(hdll);//call twice for file locks
	if(FreeLibrary(hdll))
		return 0;
	else{
		sprintf(errbuf, "error code %d closing library", GetLastError());
		return -1;
	}
#else
	FreeLibrary(hdll);
	return 0;
#endif
}
char*dlerror(){
	return errbuf;
}
#endif
