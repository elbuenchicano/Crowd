// Base2.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "CrowdAnomalies.h"
#include "Descriptors2D.h"


/*
int main(int argc, char *argv[]) {
  ///variáveis da biblioteca GCGLib
  /*gcgIMAGE previousFrame, currentFrame, nuevo, nuevo2; ///objetos para guardar as imagens
  gcgDISCRETE2D<double> u, v; /// u representa a componente horizontal do fluxo e v a componente vertical
  gcgDISCRETE2D<int> convergence; ///matriz que indica a convergência de cada ponto (1 se o ponto converge, 0 caso contrário)

                                  ///parâmetros para o cálculo do fluxo
  float alpha = 1.0; ///peso que será dado a equação da onda
  float maxFlow = 7.0; ///vetores maiores que esse valor serão filtrados
  int orthogonal = 1; ///1 se optar por fazer a filtragem ortogonal no cálculo das derivadas, 0 caso contrário

                      ///as imagens para o cálculo do fluxo são carregadas aqui
                      ///as imagens devem ser do tipo .bmp
  previousFrame.loadBMP("e:/crowd/peds2/0001.bmp");
  currentFrame.loadBMP("e:/crowd/peds2/0002.bmp");
  
    /**/
  ///função que calcula o fluxo
  ///retorna 3 matrizes do tipo gcgDISCRETE2D:
  ///componente horizontal do fluxo (u)
  ///componente vertical do fluxo (v)
  ///matriz de convergência: 1 se o ponto converge, 0 caso contrário
  //computeFlow(&previousFrame, &currentFrame, alpha, maxFlow, orthogonal, &u, &v, &convergence);
  
  ///para acessar um elemento das matrizes basta utilizar o método getDataSample(int coluna, int linha)
  ///Exemplo:
  /*
  //computeOfMarcelo("e:/crowd/peds2/train", "bmp", "e:/crowd/peds2/ofm/train");
  computeOfMarcelo("e:/crowd/peds2/test", "bmp", "e:/crowd/peds2/ofm/test");

  return 0;
}
*/


int main(int argc, char ** argv)
{
	switch(argc){
	case 1:
		{
		//CrowdAnomalies cr("S:/Backup/antiguavm/CrowdB2/PruebaSib/testEntropy/validentropy.yml");          
		CrowdAnomalies cr("S:/Backup/antiguavm/CrowdB2/PruebaSib/testEntropy/outs/test350000.yml");          
		//CrowdAnomalies cr("S:/Backup/antiguavm/CrowdB2/PruebaSib/testEntropy/outs/test350000.yml");          
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
      cout << "Nothing to do";
		}
	}
	return 0;
}
/**/