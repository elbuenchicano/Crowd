// Base2.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "CrowdAnomalies.h"

int _tmain(int argc, char * argv[])
{
	switch(argc){
	case 1:
		{
		cout << "Case 1:"<<endl;
		CrowdAnomalies cr("e:/crowdb2/input.txt");
		cr.Execute();
		break;
		}
	case 2:
		{
		cout << "Case 2:"<<endl;
		cout << argv[1]<<endl;
		CrowdAnomalies cr(argv[1]);
		cr.Execute();
		break;
		}
	case 3:
		{
		string token = argv[1];
		if (token == "-build")
		{

		}
		}
	}
	return 0;
}
