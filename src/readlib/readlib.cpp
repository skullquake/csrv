#include"readlib.hpp"
#include<iostream>
#include<algorithm>
#include<iomanip>
#ifdef __linux
#include<elf.h>		//so
#include<assert.h>	//..
#include<fcntl.h>	//..
#include<unistd.h>	//..
#include<dlfcn.h>	//..
#include<link.h>	//..
#include<cstring>	//..
#elif _WIN32
#include<time.h>
#include<windows.h>	//dll
#include<fcntl.h>	//..
#include<stdio.h>	//..
//#include<sys/mman.h>	//..
#include"mman/mman.h"	//..
#include<sys/stat.h>	//..
#include<sys/types.h>	//..
#include<unistd.h>	//..
#endif
std::string getModulePath(void){
#ifdef __linux
	Dl_info dl_info;
	dladdr((void*)getModulePath,&dl_info);
	return std::string(dl_info.dli_fname);
#elif _WIN32
	char path[MAX_PATH];
	HMODULE hm=NULL;
	if(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCSTR) &getModulePath,&hm)==0){
	    int ret=GetLastError();
	    fprintf(stderr,"GetModuleHandle failed, error = %d\n",ret);
	}
	if(GetModuleFileName(hm,path,sizeof(path))==0){
		int ret=GetLastError();
		fprintf(stderr,"GetModuleFileName failed, error = %d\n",ret);
	}
	return std::string(path);
#endif
}
//--------------------------------------------------------------------------------
#ifdef __linux
typedef struct{
	const char*libname;
	std::vector<std::string>*vsym;
}dl_iterate_phdr_data;
static int callback(struct dl_phdr_info*info,size_t size,void*data){
	const char*libname=((dl_iterate_phdr_data*)data)->libname;
	std::vector<std::string>*vsym=((dl_iterate_phdr_data*)data)->vsym;
	if(strstr(info->dlpi_name,libname)){
		//printf("loaded %s from: %s\n",libname,info->dlpi_name);
		for(int j=0;j<info->dlpi_phnum;j++){
			if(info->dlpi_phdr[j].p_type==PT_DYNAMIC){
				Elf64_Sym*symtab=nullptr;
				char*strtab=nullptr;
				int symentries=0;
				auto dyn=(Elf64_Dyn*)(info->dlpi_addr+info->dlpi_phdr[j].p_vaddr);
				/*
				 *src/readlib/readlib.cpp: In function ‘int callback(dl_phdr_info*, size_t, void*)’:
src/readlib/readlib.cpp:60:18: warning: comparison between signed and unsigned integer expressions [-Wsign-compare]
     for(int k=0;k<info->dlpi_phdr[j].p_memsz/sizeof(Elf64_Dyn);++k){
                 ~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

				 *
				 */
				for(int k=0;k<info->dlpi_phdr[j].p_memsz/sizeof(Elf64_Dyn);++k){
					if(dyn[k].d_tag==DT_SYMTAB){
						symtab=(Elf64_Sym *)dyn[k].d_un.d_ptr;
					}
					if(dyn[k].d_tag==DT_STRTAB){
						strtab=(char*)dyn[k].d_un.d_ptr;
					}
					if(dyn[k].d_tag==DT_SYMENT){
						symentries=dyn[k].d_un.d_val;
					}
				}
				int size=strtab-(char*)symtab;
				for(int k=0;k<size/symentries;++k){
					auto sym=&symtab[k];
					auto str=std::string(&strtab[sym->st_name]);
					if(str.length()>0)
						vsym->push_back(str);
				}
				break;
			}
		}
	}else{
	}
	return 0;
}
std::vector<std::string>readlib(std::string path){
	//path="test0.so";//path="./"+path;
	std::vector<std::string>vsym;
	dl_iterate_phdr_data data{path.c_str(),&vsym};
	dl_iterate_phdr(callback,(void*)&data);
	std::sort(vsym.begin(),vsym.end());
	return vsym;
}
//--------------------------------------------------------------------------------
#elif _WIN32
std::vector<std::string>readlib(std::string path){
	//see also
	//https://stackoverflow.com/questions/1128150/win32-api-to-enumerate-dll-export-functions
	//http://www.vijaymukhi.com/security/windows/pefile.htm
	//https://www.codenong.com/cs106161934/
	std::vector<std::string>vsym;
	//HMODULE lib=LoadLibraryEx(path.c_str(),NULL,DONT_RESOLVE_DLL_REFERENCES);
	HMODULE lib=LoadLibrary(path.c_str());
	if((((PIMAGE_DOS_HEADER)lib)->e_magic==IMAGE_DOS_SIGNATURE)){
		PIMAGE_NT_HEADERS header=(PIMAGE_NT_HEADERS)((BYTE *)lib + ((PIMAGE_DOS_HEADER)lib)->e_lfanew);
		if((header->Signature==IMAGE_NT_SIGNATURE)){
			if((header->OptionalHeader.NumberOfRvaAndSizes>0)){
				PIMAGE_EXPORT_DIRECTORY exports=(PIMAGE_EXPORT_DIRECTORY)((BYTE *)lib+header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
				if(exports->AddressOfNames!=0){
					//BYTE**names=(BYTE**)((int)lib+exports->AddressOfNames);
					/*
					 *src/readlib/readlib.cpp:110:32: error: cast from ‘HMODULE {aka HINSTANCE__*}’ to ‘int’ loses precision [-fpermissive]
      DWORD*names=(DWORD*)((int)lib+exports->AddressOfNames);

					 *
					 */
					DWORD*names=(DWORD*)((int64_t)lib+exports->AddressOfNames);
					for(int i=0;i<(int)(exports->NumberOfFunctions);i++){
						std::string s((char*)((BYTE*)lib+(int)names[i]));
						vsym.push_back(s);
					}
				}else{
					std::cerr<<"error:readlib:failed to obtain names address"<<std::endl;
				}
			}else{
				std::cerr<<"error:readlib:invalid NumberOfRvaAndSizes"<<std::endl;
			}
		}else{
			std::cerr<<"error:readlib:invalid header signature"<<std::endl;
		}
	}else{
		std::cerr<<"error:readlib:invalid magic bytes"<<std::endl;
	}
	return vsym;
}
//--------------------------------------------------------------------------------
#else
#error UKNOWN OS not supported
//--------------------------------------------------------------------------------
#endif
