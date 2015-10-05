#ifndef HDESCRIPTOR_H
#define HDESCRIPTOR_H
#include "CUtil.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include <type_traits>

////////////////////////////////////////////////////////////////////
//==================================================================
//struct base for magnitude orientation descriptor
struct OFBasedDescriptorBase
{
	OFBasedDescriptorBase(){}
	virtual void setData(std::string file){}
	virtual void Describe(void *, void *) = 0;
};
//==================================================================
//==================================================================
//trait for MO descriptor
struct Trait_OM
{
	typedef std::pair< cv::Mat_<float>, cv::Mat_<float> >	DesparMat;
	typedef std::vector<DesparMat>							DesvecParMat;
	typedef std::vector<cutil_grig_point>					CuboTypeCont;
	typedef cv::Mat_<float>									HistoType;
	typedef std::pair<DesvecParMat, CuboTypeCont>			DesInData;	//input data type
	typedef std::vector<HistoType>							DesOutData;//output data type
};
//==================================================================
//descriptor magnitude orientation  
template <class tr>
struct OFBasedDescriptorMO : public OFBasedDescriptorBase
{
	typedef typename  tr::DesInData		DesInData;
	typedef typename  tr::DesOutData	DesOutData;
	typedef typename  tr::HistoType		HistoType;

	int		_orientNumBin,
			_magnitudeBin,
			_maxMagnitude;
	float	_thrMagnitude;
	//______________________________________________________________
	
	virtual void Describe(void * invoid, void *outvoid)
	{

		DesInData & in		= *((DesInData*)(invoid));
		DesOutData & out	= *((DesOutData*)(outvoid));

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
	virtual void setData(std::string file){
		cv::FileStorage fs(file, cv::FileStorage::READ);
		fs["descriptor_orientNumBin"] >> _orientNumBin;
		fs["descriptor_magnitudeBin"] >> _magnitudeBin;
		fs["descriptor_maxMagnitude"] >> _maxMagnitude;
		fs["descriptor_thrMagnitude"] >> _thrMagnitude;
	}
};
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//==================================================================
//==================================================================

//trait for MO descriptor
struct Trait_M
{
	typedef cv::Mat_<float>							HistoType;
	typedef std::vector<cutil_grig_point>			CuboTypeCont;
	typedef std::vector<cv::Mat_<float> >			vecMatMag;
	typedef std::pair<vecMatMag, CuboTypeCont>		DesInDataMag;	//input data type
	typedef std::vector<HistoType>					DesOutDataMag;//output data type
};

////////////////////////////////////////////////////////////////////////////////
//Simple magnitude descriptor...................................................
template <class tr>
struct OFBasedDescriptorMagnitude : public OFBasedDescriptorBase
{
	typedef typename  tr::DesInDataMag	DesInDataMag;
	typedef typename  tr::DesOutDataMag	DesOutDataMag;
	typedef typename  tr::HistoType		HistoType;
	int		_magnitudeBin;
	float	_thrMagnitude,
			_maxMagnitude;
	//________________________________________________________________

	void Describe(void * invoid, void *outvoid)
	{
		DesInDataMag & in	= *((DesInDataMag*)(invoid));
		DesOutDataMag & out	= *((DesOutDataMag*)(outvoid));

		double	binVelozRange	= _maxMagnitude / (float)_magnitudeBin;
		int		cubPos			= 0;
		for (auto & cuboid : in.second ) //for each cuboid
		{
			HistoType histogram(1, _magnitudeBin + 1);
			histogram = histogram * 0;
			for (auto & img : in.first) // for each image
			{
				for (int i = cuboid.xi; i < cuboid.xf; ++i)
				{
					for (int j = cuboid.yi; j < cuboid.yf; ++j)
					{
						if (img(i, j) > _thrMagnitude)
						{
							int s = (int)(img(i, j) / binVelozRange);
							if (s >= _magnitudeBin) s = _magnitudeBin;
							histogram(0, s)++;
						}
					}
				}
			}
			out[cubPos++].push_back(histogram);
		}
	}
	virtual void setData(std::string file)
	{
		cv::FileStorage fs(file, cv::FileStorage::READ);
		fs["descriptor_thrMagnitude"]	>> _thrMagnitude;
		fs["descriptor_magnitudeBin"]	>> _magnitudeBin;
		fs["descriptor_maxMagnitude"]	>> _maxMagnitude;
	}
};/**/

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//Control descriptor funtion.........................................
OFBasedDescriptorBase * selectChildDes(short opt, std::string file)
{
	OFBasedDescriptorBase *  res = nullptr;
	switch (opt)
	{
		case 1:{
			res = new OFBasedDescriptorMO<Trait_OM>;
			break;
		}
		case 2:{
			res = new OFBasedDescriptorMagnitude<Trait_M>;
			break;
		}
		default:{}
	}
	if(res) res->setData(file);
	return res;
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////


#endif//HDESCRIPTOR_H