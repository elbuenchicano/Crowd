#ifndef HDESCRIPTOR_H
#define HDESCRIPTOR_H
#include "CUtil.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include <type_traits>

////////////////////////////////////////////////////////////////////
//==================================================================
//struct base for magnitude orientation descriptor
template<class tr>
struct OFBasedDescriptorBase
{
	typedef typename  tr::trDesInData	trDesInData;
	typedef typename  tr::trDesOutData	trDesOutData;
	int		_orientNumBin,
			_magnitudeBin,
			_maxMagnitude;
	float	_thrMagnitude;
	OFBasedDescriptorBase(){}
	void setData(std::string file){
		cv::FileStorage fs(file, cv::FileStorage::READ);
		fs["descriptor_orientNumBin"] >> _orientNumBin;
		fs["descriptor_magnitudeBin"] >> _magnitudeBin;
		fs["descriptor_maxMagnitude"] >> _maxMagnitude;
		fs["descriptor_thrMagnitude"] >> _thrMagnitude;
	}
	virtual void Describe(trDesInData &, trDesOutData &) = 0;
};
//==================================================================
//==================================================================
//main typedefs
typedef std::pair< cv::Mat_<float>, cv::Mat_<float> >	DesparMat;
typedef std::vector<DesparMat>							DesvecParMat;
typedef std::vector<cutil_grig_point>					CuboTypeCont;
typedef cv::Mat_<float>									HistoType;
typedef std::pair<DesvecParMat, CuboTypeCont>			DesInData;	//input data type
typedef std::vector<HistoType>							DesOutData;//output data type
//trait for MO descriptor
struct OFBased_Trait
{
	typedef DesInData	trDesInData;
	typedef DesOutData	trDesOutData;
};
//==================================================================
//descriptor magnitude orientation  
template <class tr>
struct OFBasedDescriptorMO : public OFBasedDescriptorBase<tr>
{
	typedef typename  tr::trDesInData	trDesInData;
	typedef typename  tr::trDesOutData	trDesoutData;
	virtual void Describe(trDesInData & in, trDesOutData & out)
	{
		double	binRange		= 360 / _orientNumBin,
				binVelozRange	= _maxMagnitude / (float)_magnitudeBin;
		int		cubPos			= 0;
		for (auto & cuboid : in.second ) //for each cuboid
		{
			HistoType histogram(1, _orientNumBin * (_magnitudeBin + 1));
			histogram = histogram * 0;
			for (auto & imgPair : in.first) // for each image
			{
				for (int i = cuboid.xi; i < cuboid.xf; ++i)
				{
					for (int j = cuboid.yi; j < cuboid.yf; ++j)
					{
						if (imgPair.second(i, j) > _thrMagnitude)
						{
							int p = (int)(imgPair.first(i, j) / binRange);
							int s = (int)(imgPair.second(i, j) / binVelozRange);
							if (s >= _magnitudeBin) s = _magnitudeBin;
							histogram(0, p*_magnitudeBin + s)++;
						}
					}
				}
			}
			out[cubPos++].push_back(histogram);
		}
	}
};

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//Control descriptor funtion.........................................
template <class tr>
OFBasedDescriptorBase<tr> * selectChildDes(short opt, std::string file)
{
	OFBasedDescriptorBase<tr> *  res = nullptr;
	switch (opt)
	{
		case 1:{
			res = new OFBasedDescriptorMO<tr>;
			res->setData(file);
			break;
		}
		default:
		{}
	}
	return res;
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////


#endif//HDESCRIPTOR_H