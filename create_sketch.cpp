/*
hash values restored using long long int

only 1 sketch file per image
neighborhood = 3*4*SIGMA

we keep only unique VWs in each bundle
keypoints are associated to VW using fastann library

compile: g++ -O2 `pkg-config --cflags opencv` -o create_sketch create_sketch.cpp `pkg-config --libs opencv` -lfastann

*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>

#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

//#include "bundle.h"
#include "sketch.h"
#include <fastann/fastann.hpp>

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include<sys/stat.h>
#include <functional>


using namespace std;
using namespace cv;

// Dictionary in txt format for simplicity
const char *dictionary_path = "../Dictionary1M_iter10.txt";

// Folder where the key files are
const char *dir = "../data/FlickrLogos-v2/keys_train/";

// Folder to store sketches of key files
string sketch_folder = string("../data/FlickrLogos-v2/sketches_train/");

# define BUNDLE_SIZE 4 // min of 4 vw in a bundle - paper mentions 3
# define NEIGHBORHOOD_RATIO 2 //ratio of radius of central feature and cutoff radius for bundling - paper mentions to use 1  

# define DICTIONARY_SIZE 1000003
# define hasha1 234511
# define hasha2	462901
# define hasha3	678383



static inline uint64_t rdtsc()
{
    #ifdef __i386__
    uint32_t a, d;
#elif defined __x86_64__
    uint64_t a, d;
#endif

    asm volatile ("rdtsc" : "=a" (a), "=d" (d));

    return ((uint64_t)a | (((uint64_t)d)<<32));
}

template<class Float>
vector<unsigned>
test_kdtree(unsigned D, double min_accuracy, Float* pnts, unsigned pnts_size, Float*qus, unsigned qus_size, fastann::nn_obj<Float>* nnobj_kdt )
{
//    Float* pnts = fastann::gen_unit_random<Float>(N, D, 42);
//    Float* qus = fastann::gen_unit_random<Float>(N, D, 43);   

    std::vector<Float> mins_kdt(qus_size);
    std::vector<unsigned> argmins_kdt(qus_size);

//    fastann::nn_obj<Float>* nnobj_exact = fastann::nn_obj_build_exact(pnts, pnts_size, D);
//    fastann::nn_obj<Float>* nnobj_kdt = fastann::nn_obj_build_kdtree(pnts, pnts_size, D, 8, 768);

//    nnobj_exact->search_nn(qus, qus_size, &argmins_exact[0], &mins_exact[0]);
    nnobj_kdt->search_nn(qus, qus_size, &argmins_kdt[0], &mins_kdt[0]);
    
    return argmins_kdt;
    

}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}


/*	creates single sketch from a bundle 
	b is a bundle which is vector of VWs
*/
sketch* create_sketch(vector<unsigned>& b, int prime, int hasha)
{
	int sketch_b=0;
	int minhash=DICTIONARY_SIZE, k=0;
	unsigned long long int j=0;

	for(int i=1; i<b.size(); i++)
	{
		j = hasha*b[i];
		k = j%prime;
		if(k < minhash) { minhash=k; sketch_b=b[i]; }
	
	}
//	cout<< sketch_a<<"\t"<<sketch_b<<endl;
	return new sketch(b[0], sketch_b);

}


/*
	Reads rootSift features from key file
	fills array of descriptor and vector of keypoints

	size of keypoint is multiplied by factor of 4*3 as read in paper

*/
void readKeyDesFileFloat_rootsift(char *filename, float* desc, vector<KeyPoint>::iterator it,int numkeys){

	ifstream in;
	in.open(filename);

	float x=0,y=0,sigma=0,angle=0,a=0;

	int oc=-8;
	unsigned char c;
	int count=0;

	in>>x;	// ignore num_columns from first line of key file
	in>>y;	// ignore num_rows from first line of key file

	while(!in.eof() && count<numkeys){

		in>>x;
		in>>y;
		in>>sigma;
		in>>angle;
		in>>oc;

		it->pt.x = x;
		it->pt.y = y;
		it->size = 4*3.0*sigma;
		it->angle = angle*180/3.14;
		it->octave = oc;
		it++;

		for(int i=0;i<128;i++)	{in>>a; *(desc+count*128+i)=a; }

		    if(!in){
		      in.close();
		    }

		count++;

	}

	in.close();
  
}



void compute_bow(char *filename, string dir_name, string filename_cut, float*  dictionary, int file_num, string sketch_folder, fastann::nn_obj<float>* nnobj_kdt)
{
	cout<<"computing BOW -> Processing file "<<file_num<<" : "<<filename<<endl;
	ifstream inFile(filename);
//	cout<<filename<<endl;
	int no_keypoints = count(istreambuf_iterator<char>(inFile), istreambuf_iterator<char>(), '\n')-1;

	vector<KeyPoint> keypoints(no_keypoints);
	vector<KeyPoint> :: iterator iter = keypoints.begin();

	float* desc_fl = new float[no_keypoints*128];
	readKeyDesFileFloat_rootsift(filename, desc_fl, iter, no_keypoints);

	vector<unsigned> keypoints_vw = test_kdtree<float>(128, 36, dictionary, DICTIONARY_SIZE, desc_fl, no_keypoints, nnobj_kdt);
    
	vector<sketch> sketch_coll1, sketch_coll2, sketch_coll3;
	
	for(int i=0; i<no_keypoints; i++)
	{
		float center_x = keypoints[i].pt.x;
		float center_y = keypoints[i].pt.y;
		float center_size = keypoints[i].size;
		
 	        float spatial_dist = 0.0;
		
                vector<unsigned> b;
		b.push_back(keypoints_vw[i]);
		
		for(int j=0; j<no_keypoints; j++)
		{
			if(j != i && keypoints[j].size>0.5*center_size && keypoints[j].size<2*center_size)
			{
				
				spatial_dist = sqrt((center_x-keypoints[j].pt.x)*(center_x-keypoints[j].pt.x) + (center_y-keypoints[j].pt.y)*(center_y-keypoints[j].pt.y));
			
				if(NEIGHBORHOOD_RATIO*spatial_dist<center_size) 
				{
					bool flag_vw=true;
					
		///////////////		UNCOMMENT TO KEEP ONLY UNIQUE VISUAL WORDS IN A BUNDLE	////////////////
					for(int v=0;v<b.size();v++)
						if(b[v] == keypoints_vw[j])
							flag_vw=false;
					if(flag_vw)	
						b.push_back(keypoints_vw[j]);
				}
			}
		
		}
		if(b.size()>=BUNDLE_SIZE)
		{
			sketch_coll1.push_back(*create_sketch(b, DICTIONARY_SIZE, hasha1));
			sketch_coll2.push_back(*create_sketch(b, DICTIONARY_SIZE, hasha2));
			sketch_coll3.push_back(*create_sketch(b, DICTIONARY_SIZE, hasha3));
		}


	}

	vector<string> dir_split = split(dir_name, '/');
	vector<string> filename_split = split(filename_cut, '.');
		
//	cout<<dir_split[dir_split.size()-1]<<endl;
	mkdir((sketch_folder+dir_split[dir_split.size()-1]).c_str(),S_IRWXU);
	string sketch_path1 = sketch_folder+dir_split[dir_split.size()-1]+"/"+filename_split[0]+".txt";

//	cout<<sketch_path<<endl;
	ofstream myfile1;
	myfile1.open (sketch_path1.c_str());

	
	
	for(int i=0;i<sketch_coll1.size();i++)
	{
		myfile1 << sketch_coll1[i].a<<"\t"<<sketch_coll1[i].b << "\t"<<sketch_coll2[i].b << "\t"<<sketch_coll3[i].b;		
		myfile1<<"\n";
	}
	
	myfile1.close();

}

void searchDir_create_bundles(const char * dir, float* dictionary, int count, string sketch_folder, fastann::nn_obj<float>* nnobj_kdt)
{	
        DIR *d;
        struct dirent * entry;
        struct stat buf;
        d = opendir(dir);
        entry = readdir(d);
        while(entry != NULL)
        {
                if(0 != strcmp( ".", entry->d_name) && //Skip those directories
                   0 != strcmp( "..", entry->d_name) )
                {

		string t_name = string(dir) + "/" + string(entry->d_name);
		char *name = new char[t_name.length() + 1];
		strcpy(name, t_name.c_str());
		
//                char * name = string(dir)+entry->d_name;
                stat(name, &buf);
                if(name[t_name.length()-4]=='.')
                	{
                		count++;
               			compute_bow(name, string(dir), string(entry->d_name), dictionary, count, sketch_folder, nnobj_kdt);
      				
               		}
                if(S_ISDIR(buf.st_mode))        //Check if sub directory
                {
                //        cout << "/";            //Formatting
                        searchDir_create_bundles(name, dictionary, count, sketch_folder, nnobj_kdt);
                }
               
                }
                entry = readdir(d);             //Next file in directory        
        }
        closedir(d);
}

void readDictionary_float(const char *filename, float* dict, int dict_size){

	ifstream in;
	in.open(filename);

	float a=0.0;
	int count=0;
	while(!in.eof() && count<dict_size){

		for(int i=0;i<128;i++)	{in>>a; *(dict+count*128+i)=a;}

		    if(!in){
		      in.close();
		    }
	
		count++;

	}

	in.close();
}

int main(int argc, char* argv[]){

	cout<<"Reading Dictionary !!!"<<endl;

	float* dict_fl = new float[DICTIONARY_SIZE*128];
	readDictionary_float(dictionary_path, dict_fl, DICTIONARY_SIZE);
	   
	cout<<"Dictionary Read !!!"<<endl;

	fastann::nn_obj<float>* nnobj_kdt = fastann::nn_obj_build_kdtree(dict_fl, DICTIONARY_SIZE , 128, 8, 768);

	int count=0;
	searchDir_create_bundles(dir, dict_fl, count, sketch_folder, nnobj_kdt);

	return 0;
}

