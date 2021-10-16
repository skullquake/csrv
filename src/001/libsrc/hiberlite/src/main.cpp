#include"util/util.h"
#include<stdio.h>
#include"mongoose.h"
static int reqidx=0;
EXTERN_C_BEG
DLL_PUBLIC void entry(struct mg_connection*c,int ev,void*ev_data,void*fn_data);
EXTERN_C_END
DLL_LOCAL int test();
DLL_LOCAL mg_connection*c=NULL;
DLL_PUBLIC void entry(struct mg_connection*c_,int ev,void*ev_data,void*fn_data){
	c=c_;
	mg_printf(c,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n");
	test();
	mg_http_printf_chunk(c,"tpl:%d",reqidx++);
	mg_http_printf_chunk(c,"");//end
}




#include<iostream>
#include<vector>
#include"hiberlite.h"
class Person_{//no hiberlite
	public:
		std::string name;
		int age;
		std::vector<std::string> bio;
};
class Person{//hiberliteized
	friend class hiberlite::access;
	template<class Archive>
	void hibernate(Archive&ar){
		ar & HIBERLITE_NVP(name);
		ar & HIBERLITE_NVP(age);
		ar & HIBERLITE_NVP(bio);
	}
	public:
		std::string name;
		double age;
		std::vector<std::string>bio;
};
HIBERLITE_EXPORT_CLASS(Person)
int test(){
	mg_http_printf_chunk(c,"%s\n","main:beg");
	static bool initialized=false;
	static hiberlite::Database db;
	if(!initialized){
		mg_http_printf_chunk(c,"%s\n","main:initializing");
		//open db
		db.open(":memory:");
		//reg mod
		db.registerBeanClass<Person>();
		db.dropModel();
		db.createModel();
		//mutate mod
		for(int i=0;i<8;i++){//ex sav
			mg_http_printf_chunk(c,"creating person %d\n",i);
			Person x;
			x.name="Person"+std::to_string(i);
			x.age=i;
			for(int j=0;j<32;j++){
				x.bio.push_back(std::string("bio ")+std::to_string(1900+j));
			}
			hiberlite::bean_ptr<Person>p=db.copyBean(x);
			x.age=-1; //no mutate record
			p->age=22;//mutate record
		}
		initialized=true;
	}
	{//ex lod
		int pidx=0;
		int bidx=0;
		hiberlite::bean_ptr<Person> p=db.loadBean<Person>(pidx+1);
		mg_http_printf_chunk(c,"%s,%s,%d\n","p[0]:",p->name.c_str(),p->age);
		bidx=0;for(auto&b:p->bio){
			mg_http_printf_chunk(c,"p[%d]:bio[%d]:%s\n",pidx,bidx++,b.c_str());
		}
		std::vector< hiberlite::bean_ptr<Person>>v=db.getAllBeans<Person>();
		mg_http_printf_chunk(c,"found %d persons in database\n",v.size());
		pidx=0;for(auto&p:v){
			mg_http_printf_chunk(c,"p[%d]:%s,%d\n",pidx++,p->name.c_str(),p->age);
			bidx=0;for(auto&b:p->bio){
				mg_http_printf_chunk(c,"p[%d\:bio[%d]:%s\n",pidx,bidx++,b.c_str());
			}
		}
	}
	mg_http_printf_chunk(c,"main:end\n");
	return 0;
}
