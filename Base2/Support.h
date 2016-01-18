#ifndef SUPPORT_H
#define SUPPORT_H

#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <math.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include "Figtree.h"
#include <fstream>


#define M_PI           3.14159265358979323846


//===========================================
//main typedefs
typedef std::vector< cv::Mat >							OFdataType;
typedef std::pair< cv::Mat_<float>, cv::Mat_<float> >	OFparMat;
typedef std::vector<OFparMat>							OFvecParMat;
//===========================================
//heritance for Optical flow 
struct OpticalFlowBase
{
	virtual void	compute(OFdataType &, OFvecParMat &) = 0;
};

//===========================================
//heritance for OF 


struct OpticalFlowOCV : public OpticalFlowBase
{
	virtual void	compute(OFdataType & /*in*/, OFvecParMat & /*out*/);
};


static inline void FillPointsOriginal(std::vector<cv::Point2f> &vecPoints, cv::Mat  fr_A, cv::Mat  fr_a, int thr = 30)
{
	vecPoints.clear();
	cv::cvtColor(fr_A, fr_A, CV_BGR2GRAY);
	cv::cvtColor(fr_a, fr_a, CV_BGR2GRAY);
	//frame substraction
	cv::Mat fg = fr_A - fr_a;
	for (int i = 0; i < fg.rows; ++i)
	{
		for (int j = 0; j< fg.cols; ++j)
		{
			if (abs(fg.at<uchar>(i, j))  >thr)
				vecPoints.push_back(cv::Point2f((float)j, (float)i));
		}
	}
}

static inline void VecDesp2Mat(std::vector<cv::Point2f> &vecPoints, std::vector<cv::Point2f> &positions, std::pair<cv::Mat_<float>, cv::Mat_<float> > & AMmat)
{
	float	magnitude,
			angle,
			catetoOpuesto,
			catetoAdjacente;
	//..................................................................................
	for (int i = 0; i < (int)positions.size(); ++i)	{
		catetoOpuesto	= vecPoints[i].y - positions[i].y;
		catetoAdjacente = vecPoints[i].x - positions[i].x;
		//determining the magnite and the angle
		magnitude = sqrt((catetoAdjacente	* catetoAdjacente) +
						(catetoOpuesto		* catetoOpuesto));
		//definir signed or unsigned---------------------------------------------------------!!!!
		angle = (float)atan2f(catetoOpuesto, catetoAdjacente) * 180 / CV_PI;
		if (angle<0) angle += 360;

		AMmat.first((int)positions[i].y,  (int)positions[i].x) = angle;
		AMmat.second((int)positions[i].y, (int)positions[i].x) = magnitude;
	}
}

void OpticalFlowOCV::compute(OFdataType & in, OFvecParMat & out)
{
	assert(in.size()>1);
	std::vector<cv::Point2f> pointsprev, pointsnext, basePoints;
	std::vector<uchar>		status;
	cv::Mat					err;
	cv::Size				winSize(31, 31);
	cv::TermCriteria		termcrit(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, 0.3);
	int						rows,
							cols;
	//.......................................................
	rows = in[0].rows;
	cols = in[0].cols;
	for (size_t i = 0; i < in.size() - 1; ++i){
		//std::cout << "image" << i<<std::endl;
		FillPointsOriginal(pointsprev, in[i + 1], in[i]);
		cv::Mat angles(rows, cols, CV_32FC1, cvScalar(0.));
		cv::Mat magni(rows, cols, CV_32FC1, cvScalar(0.));
		OFparMat data;
		data.first = angles;
		data.second = magni;
		//computing optical flow por each pixel
		if (pointsprev.size()>0){
			cv::calcOpticalFlowPyrLK(in[i], in[i + 1], pointsprev, pointsnext,
				status, err, winSize, 3, termcrit, 0, 0.001);
			VecDesp2Mat(pointsnext, pointsprev, data);
		}

		/*cv::Mat sc = in[i+1].clone();
		for (auto &p : pointsprev){
			cv::circle(sc, p, 0.5, cv::Scalar(0, 255, 0));
		}
		for (auto &p : pointsnext){
			cv::circle(sc, cv::Point((int)p.x, (int)p.y), 0.5, cv::Scalar(255, 0, 255));
		}/**/
		out.push_back(data);
	}
	
	basePoints.clear();


}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



struct OpticalFlowWill : public OpticalFlowBase
{
	virtual void	compute(OFdataType & /*in*/, OFvecParMat & /*out*/);
};

void OpticalFlowWill::compute(OFdataType & in, OFvecParMat & out)
{

}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//===========================================
//heritance for 
template <class t>
void supp_vectorMat2YML(std::vector<t> &vec, std::string dest, std::string token)
{
	int cont = 0;
	FileStorage fs(dest, FileStorage::WRITE);
	fs << "CuboidNumber" << (int)vec.size();
	for (auto & item : vec)
	{
		std::stringstream strcub;
		strcub << token << cont++;
		fs << strcub.str() << item;
	}
}

//-------------------------------------------
////////////////////////////////////////////////////////////////////////////////
struct MyVideoCapture : public cv::VideoCapture
{
	MyVideoCapture(std::string f){
		open(f);
	}
	bool increment(int s, cv::Mat & res){
		cv::Mat img;
		if (isOpened()){
			for (int sa = 0; sa < (s - 1); ++sa)
				read(img);
			return read(res) ? true : false;
		}
		return false;
	}
	~MyVideoCapture(){
		release();
	}
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double supp_euclidean_distance(const cv::Mat_<float> & a,const cv::Mat_<float> & b)
{
	assert(a.cols == b.cols);
	double cum=0;
	for (auto i = 0; i < a.cols; ++i)
		cum	+= (b(0, i) - a(0, i)) * (b(0, i) - a(0, i));
	return sqrt(cum);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void	supp_cov_matrix(cv::Mat_<float> src, cv::Mat_<float> & dst, std::vector<double> & meanvec)
{
	meanvec.clear();
	meanvec.resize(src.cols);
	for (int i = 1; i < src.cols; ++i)
	{
		double meanval = 0;
		for (int j = 0; j < src.rows; ++j)
			meanval += src(j, i);
		meanval /= src.rows;
		for (int j = 0; meanval && j < src.rows; ++j)
			src(j, i) = src(j, i) / meanval;
		meanvec[i] = meanval;
	}
	dst = src.t() *src;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//gabor filter //

//the sum of matrix vector
static cv::Mat supp_vecSum(std::vector<cv::Mat> & vec)
{
	assert(vec.size());
	auto mt = vec[0];
	for (size_t i = 1; i < vec.size(); ++i)
		mt = mt + vec[i];
	return mt;
}
//finding the max value per element of matri

static cv::Mat supp_vecMax(std::vector<cv::Mat> & vec)
{
	assert(vec.size());
	auto mt = vec[0];
	for (size_t i = 1; i < vec.size(); ++i)
		mt = cv::max(mt, vec[i]);
	return mt;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
typedef		std::vector<cv::Mat>	gabor_res;
std::mutex	mtx;

//normalize array or matriz A using percentil
cv::Mat_<float> supp_norm_percentil(cv::Mat img, int base, int top){
	//cv::Mat rsh = img.reshape(0, 1);
	cv::Mat_<float> A = img;
	std::vector<double> vec_ord;
	for (int i = 0; i < img.rows; i++)
	{
		std::vector<double> row;
		img.row(i).copyTo(row);
		vec_ord.insert(vec_ord.end(), row.begin(), row.end() );
	}
	std::sort(vec_ord.begin(), vec_ord.end());
	int	pos_base = std::floor(vec_ord.size() * base / 100),
		pos_top	 = std::floor(vec_ord.size() * top  / 100);
	double	val_base = vec_ord[pos_base],
			val_top	 = vec_ord[pos_top];
	A = A - val_base;
	A = A / val_top;
	for (int i = 0; i < A.rows; ++i){
		for (int j = 0; j < A.cols; ++j){
			if (A(i, j) < 0)
				A(i, j) = 0;
			else
			{
				if (A(i, j) > 1)
					A(i, j) = 1;
			}
		}
	}/**/
	return A;
}

//gabor x scale 
void gaborxscale(gabor_res &features, cv::Mat & gray, short scales, short orientation, cv::Size kernel, int u,bool uniontype = true)
{
	/*float f = 1.414213562;
	float k = M_PI/ pow(f, u);
	std::vector<cv::Mat> scalevec(orientation);
	// test for 8 orientations
	for (int v = 0; v < orientation; ++v) {

		float phi = v * M_PI / orientation;

		// build kernel
		cv::Mat rk(kernel.width * 2 + 1, kernel.height * 2 + 1, CV_32F); // real		part
		cv::Mat ik(kernel.width * 2 + 1, kernel.height * 2 + 1, CV_32F); // imaginary	part
		for (int x = -kernel.width; x <= kernel.width; ++x) {
			for (int y = -kernel.height; y <= kernel.height; ++y) {

				float _x = x * cos(phi) + y * sin(phi);
				float _y = -x * sin(phi) + y * cos(phi);
				float c = exp(-(_x * _x + _y * _y) / (4 * M_PI * M_PI));
				rk.at<float>(x + kernel.width, y + kernel.height) = c * cos(k * _x);
				ik.at<float>(x + kernel.width, y + kernel.height) = c * sin(k * _x);
			} 
		}


		// convolve with image
		cv::Mat i, r;
		cv::filter2D(gray, r, CV_32F, rk);
		cv::filter2D(gray, i, CV_32F, ik);

		// calc mag
		cv::Mat mag;
		cv::pow(i, 2, i);
		cv::pow(r, 2, r);
		cv::pow(i + r, 0.5, mag);

		// downsampling
		//cv::resize(mag, mag, cv::Size(), downscale, downscale, CV_INTER_NN);

		// store
		scalevec[v] = mag;
	}
	//using the sum
	cv::Mat mt	= uniontype? supp_vecMax(scalevec): supp_vecSum(scalevec);
	
	//================normalysing==============================================
	
	mt = supp_norm_percentil(mt, 10, 99);

	//------------------in case of doubs using mutex---------------------------
	//i think it does not necessary 
	mtx.lock();
	features[u] = mt;
	mtx.unlock();*/
}

gabor_res supp_gabor_filter(cv::Mat & image, short scales, short orientation, cv::Size kernel, bool uniontype = true)
//gabor_res supp_gabor_filter(cv::Mat & image, short scales, short orientation, float downscale, cv::Size kernel, bool uniontype = true)
{
	
	gabor_res	features(scales);
	std::vector<std::thread> t(scales);
	for (int u = 0; u < scales; ++u) 
		t[u] =  std::thread(gaborxscale, std::ref(features), std::ref(image), scales, orientation, kernel, u, uniontype);
	for (int u = 0; u < scales; ++u)
		t[u].join();
	return features;
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//Metric units
class Metric_units{
public:
	double tp_ = 0, tn_ = 0, fp_ = 0, fn_ = 0;
	Metric_units(){
		ptr = &tp_;
	}
	double & operator [](int pos){
		assert(pos >= 0 && pos < 4);
		return ptr[pos];
	}
private: //if i want create a c construct 
	double * ptr;
};

double supp_Precision(Metric_units uni){
	double den = (uni.tp_ + uni.fp_) + FLT_MIN;
	return uni.tp_ / den ;
	//return ( uni.tp_ / (uni.tp_ + uni.fp_) );
}

double supp_Recall(Metric_units uni){
	double den = (uni.tp_ + uni.fn_ + FLT_MIN );
	return uni.tp_ / den;
	//return ( uni.tp_ / (uni.tp_ + uni.fn_) );
}

double supp_FalsePositiveRate(Metric_units uni){
	double den = (uni.fp_ + uni.tn_) + FLT_MIN;
	return uni.fp_ / den;
	//return ( uni.fp_ / (uni.fp_ + uni.tn_) );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static inline void ssupp_floatp2doublep(double * a, float * b, int tam)
{
	for (int i = 0; i < tam; ++i)
		a[i] = b[i];
}

static bool supp_match(cv::Mat &src, cv::Mat &trgt, float thr)
{
	bool ok = false, ok2 = false;
	for (int i = 0; i < trgt.rows && !ok; ++i)
	{
		for (int j = 0; j < trgt.cols; ++j)
		{
			if (trgt.at<float>(i, j)){
				ok = true;
				break;
			}
		}
	}

	for (int j = 0; j < trgt.cols; ++j)
	{
		if (src.at<float>(0, j)){
			ok2 = true;
			break;
		}
	}
	
	double	h		= thr,
			epsilon = 0.02;
	// The dimensionality of each sample vector.
	int d = trgt.cols;
	// The number of targets (vectors at which gauss transform is evaluated).
	int M = trgt.rows;
	// The number of sources which will be used for the gauss transform.
	int N = src.rows;
	//src and trgt were exchanged to campare all the histograms in the NN match
	cv::Mat q = cv::Mat::ones(cv::Size(1, M), CV_64F);
	double *qPtr = (double*)q.ptr();
	
	int		W	= 1;
	float	*xt = (float*)src.ptr(),
			*yt = (float*)trgt.ptr();

	double	*x	= new double [ src.rows * src.cols ];
	double  *y	= new double [ trgt.rows * trgt.cols ];
	
	ssupp_floatp2doublep(x, xt, src.rows * src.cols);
	ssupp_floatp2doublep(y, yt, trgt.rows * trgt.cols);

	cv::Mat prob = cv::Mat::ones(cv::Size(1,M),CV_64F); //main matrix result
    double *probPtr = (double*)prob.ptr();

	figtree( d, N, M, W, x, h, qPtr, y, epsilon, probPtr);

	delete[]x;
	delete[]y;
	
	for (int i = 0; i < prob.rows; ++i)
		if (prob.at<double>(i, 0) > 0.85)
			return true;
	return false;
}

//-------------------------------------------------------------------------
//compare a sample with train data, similar to euclidean but using KDE tree
//train  : a priori information (matrix)
//sample : posteriori information 
void	supp_ComputeDistaceSamples_Train_Figtree(cv::Mat & train, cv::Mat & test, std::vector<bool> & out, float thr)
{
	//cv::Mat source = train.t();
	for (int i = 0; i < test.rows; ++i){
		auto line = test.row(i);
		//line = line.t();
		auto res = supp_match(line, train, thr);
		out[i]	 = res;
	}
}


//------------------------------------------------------------------------
//compare patterns using euclidean distance
//
void supp_SimpleDistance(cv::Mat & train, cv::Mat & test, std::vector<bool> & out, float thr)
{
	for (auto i = 0; i < test.rows; ++i)
	{
		out[i] = false;
		cv::Mat_<float> pattern = test.row(i);
		for (int j = 0; j < train.rows; ++j)
		{
			cv::Mat_<float> trainPat = train.row(j);
			if (supp_euclidean_distance(trainPat, pattern) < thr){
				out[i] = true;
				break;
			}
		}
	}
}
//-----------------------------------------------------------------------------
//compare patters using mahalanobis distance

void supp_mahalanobisDistanceFunction(cv::Mat & train, cv::Mat & test, std::vector<bool> & out, float thr)
{
	cv::Mat_<float>		covmat,
		icov,
		trainf = train;
	std::vector<double>	means;
	supp_cov_matrix(trainf, covmat, means);
	icov = covmat.inv(cv::DECOMP_SVD);
	for (auto i = 0; i < test.rows; ++i)
	{
		cv::Mat_<float> pattern = test.row(i);
		cv::Mat_<float> temp = pattern * icov;
		cv::Mat_<float> res = temp * pattern.t();
		out[i] = (res(0, 0) < thr) ? true : false;
	}
}
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
double supp_computeMeanDistanceTrain(cv::Mat & train, float amount){
  cv::Mat_<double>  distances (1, train.rows * (train.rows-1));
  int mposdistance = 0;
  for (int i = 0; i < train.rows; ++i){
    for (int j = 0; j < train.rows; ++j){
      if (i != j){
        auto distance = supp_euclidean_distance(train.row(i), train.row(j));
        distances(0, mposdistance++) = distance;
      }
    }
  }
  cv::Scalar mean, stddev;
  meanStdDev(distances, mean, stddev);
  return amount * stddev[0];
}

//------------------------------------------------------------------------------
//comapre the paterns using the  distance based in the training threshold
//it means we are going to compute the mean an the 
void supp_meanThrBasedDistance(cv::Mat & train, cv::Mat & test, 
  std::vector<bool> & out, float thr){
  
  double THR = supp_computeMeanDistanceTrain(train, thr) + 5;
  for (auto i = 0; i < test.rows; ++i)
	{
		out[i] = false;
		cv::Mat_<float> pattern = test.row(i);
		for (int j = 0; j < train.rows; ++j)
		{
			cv::Mat_<float> trainPat = train.row(j);
			if (supp_euclidean_distance(trainPat, pattern) <= THR){
				out[i] = true;
				break;
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif 