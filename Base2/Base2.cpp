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
		//CrowdAnomalies cr("D:/CrowdB3/boat-h/test1/train.yml");
		//CrowdAnomalies cr("D:/CrowdB2/pregabortest-5-8-n.yml");
		//CrowdAnomalies cr("D:/CrowdB3/train/test1/outs/test200000.yml");
    //CrowdAnomalies cr("D:/CrowdB3/boat-s/test1/test.yml");
    //CrowdAnomalies cr("D:/CrowdB2/pruebasib/test7/trainthr.yml");
    //CrowdAnomalies cr("D:/CrowdB2/pruebasib/test7/outs/test2000.yml");
    //CrowdAnomalies cr("D:/CrowdB2/pruebasib/testEntropy/outs/test10000.yml");
    
    CrowdAnomalies cr("D:/graderia/prueba/outs/test50000.yml");
   
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
