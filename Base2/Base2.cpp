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
    
    //CrowdAnomalies cr("D:/graderia/testmix_hoof/train.yml");
    //CrowdAnomalies cr("D:/graderia/testmix/test.yml");   
    //CrowdAnomalies cr("D:/peds2/preoftest.yml");
    //CrowdAnomalies cr("D:/CrowdB2/pruebasib/ofcm/ofcmtrain.yml");
    //CrowdAnomalies cr("D:/CrowdB2/PruebaSib/test6/sib6train.yml");
    //CrowdAnomalies cr("D:/CrowdB2/pruebasib/ofcm/outs/test1900000g.yml");      
    //CrowdAnomalies cr("D:/peds2/ofcm/outs/test1000000g.yml");      
		CrowdAnomalies cr("D:/subway/prueba1/outs/test150000.yml");      
    //CrowdAnomalies cr("D:/subway/prueba1/train.yml");      
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
