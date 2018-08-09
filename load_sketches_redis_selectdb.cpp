/*
	compile : g++ -o load_sketches_redis_selectdb -O3   -I ../hiredis -pthread load_sketches_redis_selectdb.cpp -g -ggdb -L../hiredis -lhiredis -lm
*/

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cstring>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <functional>
#include <string>
#include <fstream>
#include <inttypes.h>

#include "sketch.h"
#include "hiredis/hiredis.h"
#include "opencv2/imgproc/imgproc.hpp"

#define CHECK(X) if ( !X || X->type == REDIS_REPLY_ERROR ) { printf("Error\n"); exit(-1); }


using namespace std;

string sketches_train_relpaths("./sketches_train_list.txt");
string sketches_train_basepath("./data/FlickrLogos-v2/sketches_train/");

int select_database = 1;

void fill_db( redisContext *c, unsigned long long int *a1, unsigned long long int *a2, unsigned long long int *a3, int size, char* filename)
{
	redisReply *reply;	
	
	reply = (redisReply *) redisCommand(c,"SELECT %d",select_database);
	CHECK(reply);
	freeReplyObject(reply);
	
	int cmd=0;
	for (int i=0; i<size; i++ )
	{
		redisAppendCommand(c,"APPEND %llu %s", *(a1+i), filename);
		redisAppendCommand(c,"APPEND %llu %s", *(a2+i), filename);
		redisAppendCommand(c,"APPEND %llu %s", *(a3+i), filename);

		cmd += 3;
	}
	while ( cmd-- > 0 )
	{
		int r = redisGetReply(c, (void **) &reply );
		if ( r == REDIS_ERR ) { printf("Error at cmd = %d and filename : %s\n", cmd, filename); exit(-1); }
		CHECK(reply);        
		freeReplyObject(reply);
	}
	
}


/*
	Hashing a single sketch tuple (central_VW, minHash1)
	sigbit is used to keep all 3 sketches separate by adding corresponding long int
*/
void hash_sketch(sketch* s, unsigned long long int *h, int size, int sigbit)
{
	unsigned long long int temp=0, temp1=0;
	unsigned int a=0, b=0;
	
	for(int i=0; i<size; i++)
	{
		a = (s+i)->a;
		b = (s+i)->b;
		
		temp1 = a+b;
		temp1 = temp1*(temp1+1)/2 + a;
		if(sigbit == 1)	temp = temp1 + 1000000000000000; // to keep sketch from hash1 separate from hash2 and hash3
		if(sigbit == 2)	temp = temp1 + 2000000000000000; // to keep sketch from hash1 separate from hash1 and hash3
		if(sigbit == 3)	temp = temp1 + 3000000000000000; // to keep sketch from hash1 separate from hash1 and hash2
		
		*(h+i) = temp;
		//cout<<a<<"\t"<<b<<"\t"<<temp1<<"\t"<<sigbit<<"\t"<<temp<<endl;exit(0);
	}

}

void read_sketches(ifstream& in, sketch* s_coll1,  sketch* s_coll2,  sketch* s_coll3, int num){

	unsigned int xa=0, xb=0, yb=0, zb=0;
	int count=0;

	while(count<num){

		in >> xa;
		in >> xb;
		in >> yb;
		in >> zb;
	//	cout<<x<<"\t"<<y<<endl;
		(s_coll1+count)->a = xa;
		(s_coll1+count)->b = xb;

		(s_coll2+count)->a = xa;
		(s_coll2+count)->b = yb;

		(s_coll3+count)->a = xa;
		(s_coll3+count)->b = zb;

		count++;

	}

}

int main(int argc, char *argv[] ) {
    	
	redisContext *c;
	redisReply *reply;
	int n = 0;
	int db = 0;


	c = redisConnect( "localhost", 6379);
	/* c = redisConnectUnix("/tmp/redis.sock"); */
	if (c->err) {
	printf("Connection error: %s\n", c->errstr);
	exit(1);
	}

	reply = (redisReply *) redisCommand(c,"CONFIG RESETSTAT");
	CHECK(reply);
	freeReplyObject(reply);
	
	ifstream infile;
	infile.open(sketches_train_relpaths.c_str());
	string temp;

	int file_num=0;
	string name_store;
	ifstream sketch_file;	
	while(getline(infile, temp))
	{

		sketch_file.open((sketches_train_basepath + temp).c_str());
		
		int sketch_size1 = count(istreambuf_iterator<char>(sketch_file), istreambuf_iterator<char>(), '\n');
		sketch_file.seekg(0, sketch_file.beg);
		

		sketch *sketches_test1 = new sketch[sketch_size1];
		sketch *sketches_test2 = new sketch[sketch_size1];
		sketch *sketches_test3 = new sketch[sketch_size1];
		
		read_sketches(sketch_file, sketches_test1, sketches_test2, sketches_test3, sketch_size1);
		sketch_file.close();
				
		unsigned long long int *sk_ind1 = new long long unsigned int[sketch_size1];
		unsigned long long int *sk_ind2 = new long long unsigned int[sketch_size1];
		unsigned long long int *sk_ind3 = new long long unsigned int[sketch_size1];

		hash_sketch(sketches_test1, sk_ind1, sketch_size1, 1);
		hash_sketch(sketches_test2, sk_ind2, sketch_size1, 2);
		hash_sketch(sketches_test3, sk_ind3, sketch_size1, 3);

		delete[] sketches_test1;
		delete[] sketches_test2;
		delete[] sketches_test3;
		
		name_store = temp + ",";
		fill_db(c, sk_ind1, sk_ind2, sk_ind3, sketch_size1, (char*)name_store.c_str());
		
		/* 	printing hashes once ever 100 files			*/
		if(file_num>100 && file_num%100==0)
		cout<<*(sk_ind1+0)<<"\t"<<*(sk_ind2+0)<<"\t"<<*(sk_ind3+0)<<"\n"
			<<name_store<<endl;
		///////////////////////////////////////////////////////////

		

		delete[] sk_ind1;
		delete[] sk_ind2;
		delete[] sk_ind3;


		cout<<"Completed filling DB for file num : "<<++file_num<<endl;

	}

	
	infile.close();

	redisFree(c);
	
	
return 0;
}