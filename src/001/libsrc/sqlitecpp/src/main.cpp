#include"util/util.h"
#include<stdio.h>
#include<iostream>
#include<string>
#include<sstream>
#include<SQLiteCpp/SQLiteCpp.h>
#include<SQLiteCpp/VariadicBind.h>
#include<cstdio>
#include"mongoose.h"
DLL_LOCAL int reqidx=0;
static SQLite::Database*dbP;
DLL_LOCAL void __attribute__((constructor))ctor(){
	LOG(LL_DEBUG,("sqlitecpp:ctor:beg"));
	dbP=new SQLite::Database(":memory:",SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
	SQLite::Database&db=*dbP;
	try{
		{
			SQLite::Transaction txn(db);
			db.exec(R"(DROP TABLE IF EXISTS A)");
			txn.commit();
		}
		{
			SQLite::Transaction txn(db);
			db.exec(R"(
				CREATE TABLE IF NOT EXISTS A
				(
					val		TEXT
				))");
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
			);
			stmt.bind(1,std::string("a")+std::to_string(0));
			stmt.exec();
			txn.commit();
		}
		{
			SQLite::Statement q0(
				db,
				R"(
					SELECT 
					*
					FROM A
				)"
				
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
