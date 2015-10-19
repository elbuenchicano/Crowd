#ifndef SUPPORT_H
#define SUPPORT_H

#include <vector>
#include <math.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"

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

typedef std::vector<cv::Mat>	gabor_res;
gabor_res supp_gabor_filter(cv::Mat & image, short scales, short orientation, cv::Size kernel, bool uniontype = true)
//gabor_res supp_gabor_filter(cv::Mat & image, short scales, short orientation, float downscale, cv::Size kernel, bool uniontype = true)
{
	cv::Mat gray;
	gabor_res features;

	// Gabor Filter
	if (image.channels() == 3) cv::cvtColor(image, gray, CV_RGB2GRAY);

	// test for 5 scales
	for (int u = 0; u < scales; ++u) {

		float f = 1.414213562;
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
		auto mt	= uniontype? supp_vecMax(scalevec): supp_vecSum(scalevec);
		features.push_back(mt);
	}
	return features;
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//Metric units
class Metric_units{
public:
	short tp_ = 0, tn_ = 0, fp_ = 0, fn_ = 0;
	Metric_units(){
		ptr = &tp_;
	}
	short & operator [](int pos){
		assert(pos >= 0 && pos < 4);
		return ptr[pos];
	}
private: //if i want create a c construct 
	short * ptr;
};

double supp_Precision(Metric_units uni){
	double den = (uni.tp_ + uni.fp_);
	return uni.tp_ / den ;
	//return ( uni.tp_ / (uni.tp_ + uni.fp_) );
}

double supp_Recall(Metric_units uni){
	double den = (uni.tp_ + uni.fn_);
	return uni.tp_ / den;
	//return ( uni.tp_ / (uni.tp_ + uni.fn_) );
}

double supp_FalsePositiveRate(Metric_units uni){
	double den = (uni.fp_ + uni.tn_);
	return uni.fp_ / den;
	//return ( uni.fp_ / (uni.fp_ + uni.tn_) );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



#endif 