// Base2.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "CrowdAnomalies.h"

int main(int argc, char ** argv)
{
	switch(argc){
	case 1:
		{
		cout << "Case 1:"<<endl;
		CrowdAnomalies cr("d:/crowdb2/pruebas/test3.yml");
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
	default:
		{
		}
	}
	return 0;
}
