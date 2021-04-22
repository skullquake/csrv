#ifndef hab6cfe68e4b911eaacc9c3043eafb280
#define hab6cfe68e4b911eaacc9c3043eafb280
#ifdef __linux__ 
#include<dlfcn.h>
#elif _WIN32
#define RTLD_LAZY       0x00001 /* Lazy function call binding.  */
#define RTLD_NOW        0x00002 /* Immediate function call binding.  */
#define RTLD_BINDING_MASK   0x3 /* Mask of binding time value.  */
#define RTLD_NOLOAD     0x00004 /* Do not load the object.  */
#define RTLD_DEEPBIND   0x00008 /* Use deep binding.  */
#ifdef __cplusplus
extern "C"{
#endif
void*dlopen(const char*name,int mode);
void*dlsym(void*lib,const char*name);
int dlclose(void*lib);
char*dlerror();
#ifdef __cplusplus
}
#endif
#endif
#endif
