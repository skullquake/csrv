#include"duktape/duk_os.hpp"
extern "C"{
void duk_os_init(duk_context*ctx){
	duk_push_c_function(ctx,native_os,1);
	duk_put_global_string(ctx,"os");
}
duk_ret_t native_os(duk_context*ctx){
	duk_push_string(ctx,
#ifdef _WIN32
	"windows"
#elif _WIN64
	"windows"
#elif unix
	"unix"
#elif __unix
	"unix"
#elif __unix__
	"unix"
#elif __APPLE__
	"apple"
#elif __MACH__
	"apple"
#elif __linux__
	"linux"
#elif linux
	"linux"
#elif __linux
	"linux"
#elif __FreeBSD__
	"freebsd"
#elif __ANDROID__
	"android"
#else
	"unknown"
#endif
	);
	return 1;
}
}

