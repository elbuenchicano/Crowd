// Base2.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"

//#include "opencv2/highgui/highgui.hpp"
//#include <iostream>
#include "CrowdAnomalies.h"

using namespace std;
using namespace cv;




int _tmain(int argc, char * argv[])
{

	//Mat img = imread("e:/chela.JPG", 0);
	//imshow("Original Image", img);

	// Wait until user press some key
	//waitKey(0);
	if (argc < 2){
		cout << "hola";
		CrowdAnomalies cr("e:/input.txt");
		cr.Execute();
	}
	else{
		CrowdAnomalies cr(argv[1]);
		cr.Execute();
	}
	return 0;
}

