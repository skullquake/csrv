#include"util/util.h"
#include<stdio.h>
#include<iostream>
#include<string>
#include<sstream>
#include<SQLiteCpp/SQLiteCpp.h>
#include<SQLiteCpp/VariadicBind.h>
#include<cstdio>
#include"mongoose.h"
#include <string>
#include <sstream>
/*
https://stackoverflow.com/questions/12975341/to-string-is-not-a-member-of-std-says-g-mingw
redhat old cxxlib fix
g++ (GCC) 4.8.5 20150623 (Red Hat 4.8.5-39)
Copyright (C) 2015 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
namespace patch{
    template<typename T>std::string to_string(const T&n){
        std::ostringstream stm;
        stm<<n;
        return stm.str();
    }
}
DLL_LOCAL int reqidx=0;
static SQLite::Database*dbP;
DLL_LOCAL void __attribute__((constructor))ctor(){
	LOG(LL_DEBUG,("sqlitecpp:ctor:beg"));
	dbP=new SQLite::Database(":memory:",SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
	SQLite::Database&db=*dbP;
	try{
		{
			SQLite::Transaction txn(db);
			db.exec("DROP TABLE IF EXISTS A");
			txn.commit();
		}
		{
			SQLite::Transaction txn(db);
			db.exec("CREATE TABLE IF NOT EXISTS A (val TEXT)");
			txn.commit();
		}
	}catch(std::exception e){
		printf("%s",std::string(e.what()).c_str());
	}
	LOG(LL_DEBUG,("sqlitecpp:ctor:end"));
}
DLL_LOCAL void __attribute__((destructor))dtor(){
	LOG(LL_DEBUG,("sqlitecpp:dtor:beg"));
	delete dbP;
	LOG(LL_DEBUG,("sqlitecpp:dtor:end"));
}

EXTERN_C_BEG
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
EXTERN_C_END
DLL_LOCAL int test();
DLL_LOCAL mg_connection*c=NULL;
DLL_PUBLIC void entry(struct mg_connection*c_,int ev,void*ev_data,void*fn_data){
	c=c_;
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_http_printf_chunk(c,"sqlitecpp:%d\n",reqidx++);
	test();
	mg_http_printf_chunk(c,"");
}
int test(){
	SQLite::Database&db=*dbP;
	try{
		{
			SQLite::Transaction txn(db);
			SQLite::Statement stmt(
				db,
				"INSERT INTO A(val)VALUES(?)"
				/* no raw strings
				R"(
					INSERT INTO A
					(
						val
					)
					VALUES
					(
						?
					)
				)"
				*/
			);
			stmt.bind(1,std::string("a")+patch::to_string(0));
			stmt.exec();
			txn.commit();
		}
		{
			SQLite::Statement q0(
				db,
				"SELECT * FROM A"
				/* no raw strings
				R"(
					SELECT 
					*
					FROM A
				)"
				*/
				
			);
			while(q0.executeStep()){
				std::ostringstream oss;
				oss<<"(";
				for(int colidx=0;colidx<q0.getColumnCount();colidx++){
					oss<<q0.getColumn(colidx)<<",";
				}
				oss<<")"<<std::endl;
				mg_http_printf_chunk(c,oss.str().c_str());
			}
		}
	}catch(std::exception e){
		std::cerr<<e.what()<<std::endl;
	}
	return 0;
}
