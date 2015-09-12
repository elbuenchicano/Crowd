// Base2.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "CrowdAnomalies.h"

int _tmain(int argc, char * argv[])
{
	if (argc < 2){
		CrowdAnomalies cr("e:/crowdb2/input.txt");
		//CrowdAnomalies cr("e:/input.txt");
		cr.Execute();
	}
	else{
		CrowdAnomalies cr(argv[1]);
		cr.Execute();
	}
	return 0;
}

