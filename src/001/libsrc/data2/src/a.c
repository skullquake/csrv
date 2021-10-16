#include<stdio.h>
static void ctor();
static void dtor();
void __attribute__((constructor))ctor();
void __attribute__((destructor))dtor();
static void ctor(){
	printf("ctor:beg:%p\n",ctor);
	printf("ctor:end:%p\n",ctor);
}
static void dtor(){
	printf("dtor:beg:%p\n",dtor);
	printf("dtor:end:%p\n",dtor);
}
