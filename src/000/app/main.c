#include<stdio.h>
#include"./res/res.h"
#include"./srv/mongoose/srv.h"
int main(int argc,char*argv[]){
	printf("%s\n",binary_src_res_version_json_start);
	return mongoose_run(argc,argv);
}
