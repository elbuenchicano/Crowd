// Base2.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "CrowdAnomalies.h"
#include "Descriptors2D.h"




int main(int argc, char ** argv)
{
	switch(argc){
	case 1:
		{
		CrowdAnomalies cr("D:/CrowdB2/PruebaSib/Test6/outs/testO400000.yml");
		//CrowdAnomalies cr("D:/CrowdB2/pregabortest-5-8-n.yml");
		//CrowdAnomalies cr("D:/CrowdB2/PruebaSib/Test6/sib6test.yml");
		cr.Execute();
		break;
		}
	case 2:
		{
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
