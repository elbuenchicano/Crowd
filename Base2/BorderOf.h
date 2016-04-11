#ifndef BORDER_OF
#define BORDER_OF

#include "Support.h"
#include <stdlib.h>
#include <stdio.h>
#include "gcg.h"
#include <iostream>

#define ZERO 0.0001
#define MAX_ITERATIONS 10



void smooth(gcgIMAGE* img) {
  float gmask[] = { .006, .061, .242, .383, .242, .061, .006 };
  gcgDISCRETE1D<float> mask(7, 3, gmask);
  img->convolutionX(img, &mask);
  img->convolutionY(img, &mask);
}

void computeDerivatives(gcgDISCRETE2D<double> *sigA, gcgDISCRETE2D<double> *sigB, gcgDISCRETE2D<double> *dx, gcgDISCRETE2D<double> *dy, gcgDISCRETE2D<double> *dt, gcgDISCRETE2D<double> *dxy, gcgDISCRETE1D<double> *lowPassMask, gcgDISCRETE1D<double> *highPassMask) {

  lowPassMask->extension = GCG_BORDER_EXTENSION_ZERO;
  highPassMask->extension = GCG_BORDER_EXTENSION_ZERO;

  sigA->extensionX = sigA->extensionY = sigB->extensionX = sigB->extensionY = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;

  ///compute dx, dy

  if (dx && dy) {

    dx->extensionX = dx->extensionY = dy->extensionX = dy->extensionY = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;

    dx->convolutionX(sigB, highPassMask);

    dy->convolutionY(sigB, highPassMask);

  }

  ///compute dxy

  if (dxy) {

    dxy->extensionX = dxy->extensionY = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;

    dxy->convolutionX(sigB, highPassMask);

    dxy->convolutionY(dxy, highPassMask);
  }


  ///compute dt

  if (dt) {
    dt->combineAdd(sigA, sigB, -0.5, 0.5);
  }
}

void computeDerivativesOrthogonal(gcgDISCRETE2D<double> *sigA, gcgDISCRETE2D<double> *sigB, gcgDISCRETE2D<double> *dx, gcgDISCRETE2D<double> *dy, gcgDISCRETE2D<double> *dt, gcgDISCRETE2D<double> *dxy, gcgDISCRETE1D<double> *lowPassMask, gcgDISCRETE1D<double> *highPassMask) {

  lowPassMask->extension = GCG_BORDER_EXTENSION_ZERO;
  highPassMask->extension = GCG_BORDER_EXTENSION_ZERO;

  gcgDISCRETE2D<double> sigA_, sigB_;

  sigA_.createSimilar(sigA);
  sigB_.createSimilar(sigA);

  sigA->extensionX = sigA->extensionY = sigB->extensionX = sigB->extensionY = sigA_.extensionX = sigA_.extensionY = sigB_.extensionX = sigB_.extensionY = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;

  ///compute dx, dy

  if (dx && dy) {
    dx->extensionX = dx->extensionY = dy->extensionX = dy->extensionY = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;

    dx->convolutionY(sigB, lowPassMask);

    dx->convolutionX(dx, highPassMask);

    dy->convolutionX(sigB, lowPassMask);

    dy->convolutionY(dy, highPassMask);

  }

  ///compute dxy

  if (dxy) {

    dxy->extensionX = dxy->extensionY = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;

    dxy->convolutionX(sigB, highPassMask);

    dxy->convolutionY(dxy, highPassMask);
  }


  ///compute dt

  if (dt) {
    sigA_.convolutionX(sigA, lowPassMask);

    sigA_.convolutionY(&sigA_, lowPassMask);

    sigB_.convolutionX(sigB, lowPassMask);

    sigB_.convolutionY(&sigB_, lowPassMask);

    dt->combineAdd(&sigA_, &sigB_, -0.5, 0.5);
  }
}

void solveSystemIterative(gcgDISCRETE2D<double> *dx, gcgDISCRETE2D<double> *dy, gcgDISCRETE2D<double> *dt, gcgDISCRETE2D<double> *dxy,
  gcgDISCRETE2D<double> *dx2, gcgDISCRETE2D<double> *dy2, float alpha,
  gcgDISCRETE2D<double> *u, gcgDISCRETE2D<double> *v, gcgDISCRETE2D<int> *converge) {

  gcgDISCRETE2D<double> u0, v0;
  double auxU, auxV;
  double Ix, Iy, It, Ix2, Iy2, Ixy;
  double denominatorU, denominatorV, numeratorU, numeratorV;

  int cont = 0;

  u0.createSimilar(u);
  v0.createSimilar(v);
  converge->createSimilar(u);

  for (int y = 0; y < dx->height; y++) {
    for (int x = 0; x < dx->width; x++) {
      u0.setDataSample(x, y, 0.0);
      v0.setDataSample(x, y, 0.0);

      Ix = dx->getDataSample(x, y);
      Iy = dy->getDataSample(x, y);
      It = dt->getDataSample(x, y);
      Ix2 = dx2->getDataSample(x, y);
      Iy2 = dy2->getDataSample(x, y);
      Ixy = dxy->getDataSample(x, y);

      denominatorU = 2.0*Ix*Ix - alpha*(Ix2*Ix + Ixy*Iy + Ixy*Ix + Iy2*Iy);
      denominatorV = 2.0*Iy*Iy - alpha*(Ix2*Ix + Ixy*Iy + Ixy*Ix + Iy2*Iy);

      if ((fabs(denominatorU) > ZERO) && (fabs(denominatorV) > ZERO) && (sqrt((4.0*Ix*Ix*Iy*Iy) / fabs(denominatorU*denominatorV)) < 1.0)) {
        converge->setDataSample(x, y, 1);
      }
      else {
        converge->setDataSample(x, y, 0);
        cont++;
      }
    }
  }

  for (int k = 0; k < MAX_ITERATIONS; k++) {
    for (int y = 0; y < dx->height; y++) {
      for (int x = 0; x < dx->width; x++) {
        if (converge->getDataSample(x, y)) {
          auxU = 0;
          auxV = 0;

          Ix = dx->getDataSample(x, y);
          Iy = dy->getDataSample(x, y);
          It = dt->getDataSample(x, y);
          Ix2 = dx2->getDataSample(x, y);
          Iy2 = dy2->getDataSample(x, y);
          Ixy = dxy->getDataSample(x, y);

          denominatorU = 2.0*Ix*Ix - alpha*(Ix2*Ix + Ixy*Iy + Ixy*Ix + Iy2*Iy);
          numeratorU = -2.0*Ix*Iy*v0.getDataSample(x, y) - 2.0*Ix*It;

          auxU = numeratorU / denominatorU;

          denominatorV = 2.0*Iy*Iy - alpha*(Ix2*Ix + Ixy*Iy + Ixy*Ix + Iy2*Iy);
          numeratorV = -2.0*Ix*Iy*u0.getDataSample(x, y) - 2.0*Iy*It;

          auxV = numeratorV / denominatorV;

          u0.setDataSample(x, y, auxU);
          v0.setDataSample(x, y, auxV);
        }
      }
    }
  }

  u->duplicateSignal(&u0);
  v->duplicateSignal(&v0);
}

void computeFlow(gcgIMAGE *previousFrame, gcgIMAGE *currentFrame, float alpha, float maxFlow, int orthogonal, gcgDISCRETE2D<double> *u, gcgDISCRETE2D<double> *v, gcgDISCRETE2D<int> *convergence) {
  gcgDISCRETE2D<double> dx, dy, dx2, dy2, dt, dxy;
  gcgDISCRETE2D<double> sigA, sigB;

  smooth(previousFrame);
  if (!previousFrame->exportChannels(&sigA, NULL, NULL, NULL)) printf("ERROR: exportChannels(). line %d\n", __LINE__);

  smooth(currentFrame);
  if (!currentFrame->exportChannels(&sigB, NULL, NULL, NULL)) printf("ERROR: exportChannels(). line %d\n", __LINE__);

  u->createSimilar(&sigA);
  v->createSimilar(&sigA);

  dt.createSimilar(&sigA);
  dx.createSimilar(&sigA);
  dy.createSimilar(&sigA);
  dx2.createSimilar(&sigA);
  dy2.createSimilar(&sigA);
  dxy.createSimilar(&sigA);

  gcgDISCRETE1D<double> lowPassMask1, highPassMask1, lowPassMask2, highPassMask2, lowPassMask3, highPassMask3;

  double lowPassAux1[] = { 0.5, 0.5 };
  double highPassAux1[] = { -0.5, 0.5 };

  lowPassMask1.createSignal(2, 1, lowPassAux1);
  highPassMask1.createSignal(2, 1, highPassAux1);

  if (orthogonal) {
    computeDerivativesOrthogonal(&sigA, &sigB, &dx, &dy, &dt, &dxy, &lowPassMask1, &highPassMask1);
  }
  else {
    computeDerivatives(&sigA, &sigB, &dx, &dy, &dt, &dxy, &lowPassMask1, &highPassMask1);
  }

  double lowPassAux2[] = { 0.25, 0.5, 0.25 };
  double highPassAux2[] = { 0.25, -0.5, 0.25 };

  lowPassMask2.createSignal(3, 1, lowPassAux2);
  highPassMask2.createSignal(3, 1, highPassAux2);

  if (orthogonal) {
    computeDerivativesOrthogonal(&sigA, &sigB, &dx2, &dy2, NULL, NULL, &lowPassMask2, &highPassMask2);
  }
  else {
    computeDerivatives(&sigA, &sigB, &dx2, &dy2, NULL, NULL, &lowPassMask2, &highPassMask2);
  }

  solveSystemIterative(&dx, &dy, &dt, &dxy, &dx2, &dy2, alpha, u, v, convergence);

  ///filtering by the norm
  for (int y = 0; y < sigA.height; y++) {
    for (int x = 0; x < sigA.width; x++) {
      double norm = sqrt(SQR(u->getDataSample(x, y)) + SQR(v->getDataSample(x, y)));
      if (norm > maxFlow) {
        u->setDataSample(x, y, 0.0);
        v->setDataSample(x, y, 0.0);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//must be carefull the image index is invert it means the image is headlong
cv::Mat_<float> supp_gcgMatrix2ocvMatrix(gcgDISCRETE2D<double> & src) {
  cv::Mat_<float> dst = cv::Mat_<float>(src.height, src.width);
  for (int i = 0; i < src.width; ++i) {
    for (int j = 0, k = src.height-1; j < src.height; ++j, --k) {
      dst(k, i) = src.getDataSample(i, j);
    }
  }
  return dst;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//optical flow class for Prof Marcelo border optical flow
struct OpticalFlowBorder : public OpticalFlowBase
{
  virtual void	compute(OFdataType & /*in*/, OFvecParMat & /*out*/);
};

void OpticalFlowBorder::compute(OFdataType & in, OFvecParMat & out)
{
  float alpha = 1.0; ///peso que será dado a equação da onda
  float maxFlow = 7.0; ///vetores maiores que esse valor serão filtrados
  int orthogonal = 1; ///1 se optar por fazer a filtragem ortogonal no cálculo das derivadas, 0 caso contrário

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
using namespace std;


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void computeAMmat(cv::Mat_<float> &u, cv::Mat_<float> &v, std::pair<cv::Mat_<float>, cv::Mat_<float> > & AMmat) {
  float	magnitude,
        angle,
        catetoOpuesto,
        catetoAdjacente;
  //...........................................................................
  for (int i = 0; i < u.rows; ++i) {
    for (int j = 0; j < v.cols; ++j) {
      catetoOpuesto   = v(i, j);
      catetoAdjacente = u(i, j);
      //determining the magnite and the angle
      magnitude = sqrt((catetoAdjacente	* catetoAdjacente) +
        (catetoOpuesto		* catetoOpuesto));
      //definir signed or unsigned---------------------------------------------------------!!!!
      angle = (float)atan2f(catetoOpuesto, catetoAdjacente) * 180 / CV_PI;
      if (angle<0) angle += 360;

      AMmat.first(i,j)  = angle;
      AMmat.second(i,j) = magnitude;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void computeOfMarcelo(string src, string file_ext, string out_directory) {

  //.............................................................
  //choosing the optical flow technique
  //.............................................................
  //loading the images into a vector
  float alpha     = 1.0; ///peso que será dado a equação da onda
  float maxFlow   = 7.0; ///vetores maiores que esse valor serão filtrados
  int orthogonal  = 1; ///1 se optar por fazer a filtragem ortogonal no cálculo das derivadas, 0 caso contrário
  OFvecParMat   of_out;

  cutil_file_cont		file_list;
  list_files_all(file_list, src.c_str(), file_ext.c_str());
  std::vector<gcgIMAGE> image_vector;

  for (size_t i = 0; i < file_list.size()-1; ++i)
  {
    //loading images pair by pair
    gcgDISCRETE2D<double> u, v; /// u representa a componente horizontal do fluxo e v a componente vertical
    gcgDISCRETE2D<int> convergence; ///matriz que indica a convergência de cada ponto (1 se o ponto converge, 0 caso cont

    gcgIMAGE img;
    img.loadBMP(file_list[i].c_str());
    
    gcgIMAGE img2;
    img2.loadBMP(file_list[i+1].c_str());

    //computing the border optical flow........................................
    computeFlow(&img, &img2, alpha, maxFlow, orthogonal, &u, &v, &convergence);

    auto dst  = supp_gcgMatrix2ocvMatrix(u);
    auto dst2 = supp_gcgMatrix2ocvMatrix(v);
    
    //creating our scheme......................................................
    cv::Mat angles(dst.rows, dst.cols, CV_32FC1, cvScalar(0.));
    cv::Mat magni(dst.rows, dst.cols, CV_32FC1, cvScalar(0.));
    OFparMat data;
    data.first = angles;
    data.second = magni;

    //computing the angle and the magnitude....................................
    computeAMmat(dst, dst2, data);

    of_out.push_back(data);

  }

  
  for (size_t i = 0; i < of_out.size(); ++i)
  {
    stringstream outfile;
    outfile << out_directory << "/opticalflow_" << insert_numbers(i + 1, of_out.size()) << "of.yml";
    cout << outfile.str()<<endl;
    cv::FileStorage fs(outfile.str(), cv::FileStorage::WRITE);
    fs << "angle" << of_out[i].first;
    fs << "magnitude" << of_out[i].second;
    fs.release();
  }
}



#endif