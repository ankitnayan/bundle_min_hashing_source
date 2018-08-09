/*
	compile : g++ -o match_sketches_redis -O3 -pthread match_sketches_redis.cpp -g -ggdb -lhiredis -lm
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
#include <typeinfo>
#include <sstream>


#include "../sketch.h"
#include "hiredis/hiredis.h"
#include "opencv2/imgproc/imgproc.hpp"

#define CHECK(X) if ( !X || X->type == REDIS_REPLY_ERROR ) { printf("Error\n"); exit(-1); }


using namespace std;

string sketches_test_relpaths("./sketches_test_list_logosonly.txt");
string sketches_test_basepath("../data/FlickrLogos-v2/sketches_test/");
const char* result_file = "./results_sketches_logosonly.csv";
int select_database = 1;



string key_str="";

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
		if(sigbit == 1)	temp = temp1 + 1000000000000000;
		if(sigbit == 2)	temp = temp1 + 2000000000000000;
		if(sigbit == 3)	temp = temp1 + 3000000000000000;
		
		*(h+i) = temp;
		//cout<<a<<"\t"<<b<<"\t"<<temp1<<"\t"<<sigbit<<"\t"<<temp<<endl;exit(0);
	}

}


/*
	Read sketches from file and store them in array of sketch class
*/
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
	
	reply = (redisReply *) redisCommand(c,"SELECT %d",select_database);
	CHECK(reply);
	freeReplyObject(reply);

	ifstream infile;
	infile.open(sketches_test_relpaths.c_str());	// reading rel paths of test sketches
	string temp = "";

	int file_num=0;

	ofstream outfile(result_file, std::ofstream::out | std::ofstream::app);
	ifstream sketch_file;	
	int file_count = 1;
	while(getline(infile, temp))
	{
		sketch_file.open((sketches_test_basepath + temp).c_str());
		//sketch_file.open("../data/FlickrLogos-v2/sketches_test/no-logo/3520346399.txt");
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

		/* sk_ind1, sk_ind2 sk_ind3 contain hashes of sketches		*/
		hash_sketch(sketches_test1, sk_ind1, sketch_size1, 1);
		hash_sketch(sketches_test2, sk_ind2, sketch_size1, 2);
		hash_sketch(sketches_test3, sk_ind3, sketch_size1, 3);

		delete[] sketches_test1;
		delete[] sketches_test2;
		delete[] sketches_test3;
		

		/////////////////////////////////////////////////////////////////
		/*	Running redis commands in batches for 1st hashes of sketch	*/

		string sketch_matches = "";
		for (int i = 0; i < sketch_size1; ++i)
		{

			ostringstream ss;
			ss << *(sk_ind1 + i);
			key_str = "GET "+ss.str();
			redisAppendCommand(c, key_str.c_str());
		}

		/*	Getting redis reply in batches 	*/
		for(int i=sketch_size1; i>0; i--){
			int r = redisGetReply(c, (void **) &reply );
			if ( r == REDIS_ERR ) { printf("Error\n"); exit(-1); }
			CHECK(reply);
			if(reply->len != 0){
				//cout << reply->str <<endl;
				sketch_matches.append(reply->str);
			}		
			freeReplyObject(reply);
		}
		//////////////////////////////////////////////////////////////////////



		/////////////////////////////////////////////////////////////////
		/*	Running redis commands in batches for 2nd hashes of sketch	*/
        for (int i = 0; i < sketch_size1; ++i)
        {

                ostringstream ss;
                ss << *(sk_ind2 + i);
                key_str = "GET "+ss.str();
                redisAppendCommand(c, key_str.c_str());
        }
        /*	Getting redis reply in batches 	*/
        for(int i=sketch_size1; i>0; i--){
                int r = redisGetReply(c, (void **) &reply );
                if ( r == REDIS_ERR ) { printf("Error\n"); exit(-1); }
                CHECK(reply);
                if(reply->len != 0){
                        //cout << reply->str <<endl;
                        sketch_matches.append(reply->str);
                }
                freeReplyObject(reply);
        }

        /////////////////////////////////////////////////////////////////
	
		outfile << temp + "," + sketch_matches + "\n";
		cout << file_count << ") " << temp << endl;
		//cout << temp << " - " << sketch_matches<< endl;
		file_count++;	

		delete[] sk_ind1;
		delete[] sk_ind2;
		delete[] sk_ind3;
		
	}
	infile.close();

	redisFree(c);
	
	
return 0;
}