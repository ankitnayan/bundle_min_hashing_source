/*
	Uses opencv to convert jpg to grayscale pgm files needed as input to rootsift
	Compile : g++ -O2 `pkg-config --cflags opencv` -o jpg2pgm jpg2pgm.cpp `pkg-config --libs opencv`
*/


#include <cv.h>
#include <highgui.h>

#include <fstream>
#include <string>

using namespace cv;
using namespace std;

int main( int argc, char** argv )
{
	char* imageName = argv[1];
	//cout << imageName << "\n";

	Mat image = imread(imageName, 0);
	imwrite( argv[2], image );
	

	return 0;
}

