#ifndef HDESCRIPTOR_H
#define HDESCRIPTOR_H
#include "CUtil.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include <type_traits>
#include <math.h>

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
	typedef std::vector<DesparMat>							          DesvecParMat;
	typedef std::vector<cutil_grig_point>					        CuboTypeCont;
	typedef cv::Mat_<float>									              HistoType;
	typedef std::pair<DesvecParMat, CuboTypeCont>			    DesInData;	//input data type
	typedef std::vector<HistoType>							          DesOutData;//output data type
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
			  _magnitudeBin;
	float	_maxMagnitude,
			  _thrMagnitude;
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
				Mat frm(imgPair.second.rows,imgPair.second.cols, CV_8SC3);
				frm = frm * 0;
				cv::rectangle(	frm,
					cv::Point(cuboid.yi , cuboid.xi),
					cv::Point(cuboid.yf, cuboid.xf), 
					cv::Scalar(0, 0, 255) );/**/
				for (int i = cuboid.xi; i <= cuboid.xf; ++i)
				{
					for (int j = cuboid.yi; j <= cuboid.yf; ++j)
					{
						if (imgPair.second(i, j) > _thrMagnitude)
						{
							int p = (int)(imgPair.first(i, j) / binRange);
							int s = (int)(imgPair.second(i, j) / binVelozRange);
              if (p >= _orientNumBin) p = 0;
							if (s >= _magnitudeBin) s = _magnitudeBin;
							//histogram(0, p*(_magnitudeBin+1) + s) += imgPair.second(i, j);
							++histogram(0, p*(_magnitudeBin+1) + s);
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

//trait for M descriptor
struct Trait_M
{
	typedef cv::Mat_<float>							      HistoType;
	typedef std::vector<cutil_grig_point>			CuboTypeCont;
	typedef std::vector<cv::Mat_<float> >			vecMatMag;
	typedef std::pair<vecMatMag, CuboTypeCont>		DesInDataMag;	//input data type
	typedef std::vector<HistoType>					  DesOutDataMag;//output data type
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
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//==================================================================
//==================================================================

//trait for M descriptor
struct Trait_Gabor
{
	typedef cv::Mat_< float >						HistoType;
	typedef std::vector< cutil_grig_point >			CuboTypeCont;
	typedef std::vector< cv::Mat_<float> >			VecMat;
	typedef std::vector< VecMat >					Vec_Vec_Mat;
	typedef std::pair< Vec_Vec_Mat, CuboTypeCont>	DesInData;	//input data type
	typedef std::vector< HistoType >				DesOutData;//output data type
};

////////////////////////////////////////////////////////////////////////////////
//Simple magnitude descriptor...................................................
template <class tr>
struct OFBasedDescriptorGabor : public OFBasedDescriptorBase
{
	typedef typename  tr::DesInData		DesInData;
	typedef typename  tr::DesOutData	DesOutData;
	typedef typename  tr::HistoType		HistoType;

	int		_orientNumBin,
			  _magnitudeBin,
			  _gaborNumBin;
	float	_thrMagnitude,
			  _maxMagnitude;
	//__________________________________________________________________________
	//invoid = vec vec mat, where vec mat is of_orientation, of_magnitude, other
	//		   mats correspond to n gabor scales................................
	//out	 = single histogram.................................................
	void Describe(void * invoid, void * outvoid)
	{
		DesInData & in		= *((DesInData*)(invoid));
		DesOutData & out	= *((DesOutData*)(outvoid));

		double	binRange		= 360 / _orientNumBin,
				binVelozRange	= _maxMagnitude / (float)_magnitudeBin;
		int		cubPos			= 0;
		int		step			= _orientNumBin * (_magnitudeBin + 1);
		for (auto & cuboid : in.second) //for each cuboid
		{
			/*Mat frm(in.first[0][0].rows, in.first[0][0].cols, CV_8SC3);
			frm = frm * 0;
			cv::rectangle(frm,
			cv::Point(cuboid.yi, cuboid.xi),
			cv::Point(cuboid.yf, cuboid.xf),
			cv::Scalar(0, 0, 255));/**/
			HistoType histogram(1, _orientNumBin * (_magnitudeBin + 1) * _gaborNumBin);
			histogram = histogram * 0;
			for (auto & imgVec : in.first) // for each image
			{
				
				
				for (int i = cuboid.xi; i <= cuboid.xf; ++i)
				{
					for (int j = cuboid.yi; j <= cuboid.yf; ++j)
					{
						if (imgVec[1](i, j) > _thrMagnitude)
						{
							int p = (int)(imgVec[0](i, j) / binRange);
							int s = (int)(imgVec[1](i, j) / binVelozRange);
							if (s >= _magnitudeBin) s = _magnitudeBin;
							int pos_in_mat = (p * (_magnitudeBin + 1) + s);
							
							float	maxval = imgVec[2](i,j);
							int		posmax = 2;
							for (size_t k = 3; k < imgVec.size(); ++k)
							{
								if (maxval < imgVec[k](i, j)){
									posmax = k;
									maxval = imgVec[k](i, j);
								}
							}
							++histogram(0, ((posmax - 2) * step) + pos_in_mat);
						}
					}
				}
			}
			double total = cv::sum(histogram)[0] + FLT_MIN;
			histogram	 = histogram / total;
			out[cubPos++].push_back(histogram);
		}
	}
	virtual void setData(std::string file)
	{
		cv::FileStorage fs(file, cv::FileStorage::READ);
		fs["descriptor_orientNumBin"] >> _orientNumBin;
		fs["descriptor_gaborNumBin"]  >> _gaborNumBin;
		fs["descriptor_magnitudeBin"] >> _magnitudeBin;
		fs["descriptor_maxMagnitude"] >> _maxMagnitude;
		fs["descriptor_thrMagnitude"] >> _thrMagnitude;
	}
};/**/


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double entropyOfImgRegion(cv::Mat_<float> & ori, cv::Mat_<float> & mag,
                          float thr,  int x, int y, int size,
                           int numbin, double binRange){
  int posx, posy;
  double acum = 0;
  cv::Mat_<double> histogram(1,numbin);
  histogram = histogram * 0;
  for (int i = -size; i <= size; ++i){
    for (int j = -size; j <= size; ++j){
      posx = x + i;
      posy = y + j;
      if (posx >= 0 && 
          posy >= 0 &&
          posx < ori.rows   && 
          posy < ori.cols ){
        if (mag(posx, posy) > thr){
          int bin = floor(ori(posx, posy) / binRange);
          ++histogram(0, bin);
        }
      }
    }
  }
  histogram = histogram / sum(histogram)[0] + FLT_MIN;
  for (int i = 0; i < histogram.cols; ++i)
    acum += histogram(0, i) * log(histogram(0, i));
  return -acum;
}
////////////////////////////////////////////////////////////////////////////////
//Entropy descriptor using magnitude orientation................................ 
//the main idea is taking acount the orientation 
//entropy neeigborhood of a pixel (x,y) 
template <class tr>
struct OFBasedDescriptorEntropyMO : public OFBasedDescriptorBase
{
	typedef typename  tr::DesInData		DesInData;
	typedef typename  tr::DesOutData	DesOutData;
	typedef typename  tr::HistoType		HistoType;

	int		_orientNumBin,
			  _magnitudeBin,
        entropyBin_,
        neighborTam_;
	float	_maxMagnitude,
			  _thrMagnitude,
        base_;
	//______________________________________________________________
	
	virtual void Describe(void * invoid, void *outvoid)
	{

		DesInData & in		= *((DesInData*)(invoid));
		DesOutData & out	= *((DesOutData*)(outvoid));

		double	binRange		  = 360 / _orientNumBin,
				    binVelozRange	= _maxMagnitude / (float)_magnitudeBin,
            maxEntropy    = log2(_orientNumBin),
            entropyRange  = maxEntropy / entropyBin_;
		
    int		  cubPos			  = 0;
		

		for (auto & cuboid : in.second ) //for each cuboid
		{

			HistoType histogram( 1, _orientNumBin * 
                              (_magnitudeBin + 1) * 
                              entropyBin_ );
			histogram = histogram * 0;
			for (auto & imgPair : in.first) // for each image
			{
				cv::Mat frm(imgPair.second.rows,imgPair.second.cols, CV_8SC3);
				frm = frm * 0;
				cv::rectangle(	frm,
					cv::Point(cuboid.yi , cuboid.xi),
					cv::Point(cuboid.yf, cuboid.xf), 
					cv::Scalar(0, 0, 255) );/**/
				for (int i = cuboid.xi; i <= cuboid.xf; ++i)
				{
					for (int j = cuboid.yi; j <= cuboid.yf; ++j)
					{
            if (imgPair.second(i, j) > _thrMagnitude)
						{
              int e = (int)(entropyOfImgRegion(imgPair.first, 
                        imgPair.second, _thrMagnitude,
                        i, j, neighborTam_,
                       _orientNumBin, binRange) / entropyRange);
							int p = (int)(imgPair.first(i, j) / binRange);
							int s = (int)(imgPair.second(i, j) / binVelozRange);
							if (s >= _magnitudeBin) s = _magnitudeBin;
							//histogram(0, p*(_magnitudeBin+1) + s) += imgPair.second(i, j);
							++histogram(0,  e* _orientNumBin * (_magnitudeBin+1)  + 
                              p*(_magnitudeBin+1)                   + 
                              s);
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
    fs["descriptor_entropyBin"]   >> entropyBin_;
    fs["descriptor_base"]         >> base_;
    fs["descriptor_neighborTam"]  >> neighborTam_;
	}
};
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//Classical hoof descriptor
template <class tr>
struct OFBasedDescriptorHoof : public OFBasedDescriptorBase
{
  typedef typename  tr::DesInData		DesInData;
	typedef typename  tr::DesOutData	DesOutData;
	typedef typename  tr::HistoType		HistoType;

  int   numbin_orient_;

  virtual void Describe(void * invoid, void *outvoid)
  {
    DesInData & in		= *((DesInData*)(invoid));
		DesOutData & out	= *((DesOutData*)(outvoid));

    double  binRange = 360 / numbin_orient_;

    int     cubPos   = 0;

    for (auto & cuboid : in.second ) //for each cuboid
		{

			HistoType histogram( 1, numbin_orient_ );
			histogram = histogram * 0;
			for (auto & imgPair : in.first) // for each image
			{
				/*cv::Mat frm(imgPair.second.rows,imgPair.second.cols, CV_8SC3);
				frm = frm * 0;
				cv::rectangle(	frm,
					cv::Point(cuboid.yi , cuboid.xi),
					cv::Point(cuboid.yf, cuboid.xf), 
					cv::Scalar(0, 0, 255) );/**/
				for (int i = cuboid.xi; i <= cuboid.xf; ++i)
				{
					for (int j = cuboid.yi; j <= cuboid.yf; ++j)
					{
            if (imgPair.second(i, j) > 0.5)
						{
							int p = (int)(imgPair.first(i, j) / binRange);
							histogram(0, p) += imgPair.second(i, j);
						}
					}
				}
			}
			out[cubPos++].push_back(histogram);
		}
  }

  virtual void setData(std::string file){
    cv::FileStorage fs(file, cv::FileStorage::READ);
    fs["descriptor_orientNumBin"] >> numbin_orient_;
  }
};

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
			res = new OFBasedDescriptorGabor<Trait_Gabor>;
			break;
		}
		case 3:{
			res = new OFBasedDescriptorMagnitude<Trait_M>;
			break;
		}
    case 4:{
			res = new OFBasedDescriptorEntropyMO<Trait_OM>;
			break;
		}
    case 5:{
			res = new OFBasedDescriptorHoof<Trait_OM>;
			break;
		}
		default:{}
	}
	if(res) res->setData(file);
	return res;
}

#endif//HDESCRIPTOR_H


/*

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//==================================================================
//==================================================================
//trait for MO descriptor
struct Trait_OMA
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
struct OFBasedDescriptorMOA : public OFBasedDescriptorBase
{
	typedef typename  tr::DesInData		DesInData;
	typedef typename  tr::DesOutData	DesOutData;
	typedef typename  tr::HistoType		HistoType;

	int		_orientNumBin,
			_magnitudeBin,
			_maxMagnitude,
			_densityNumBin;
	float	_thrMagnitude;
	//______________________________________________________________
	
	virtual void Describe(void * invoid, void *outvoid)
	{

		DesInData & in		= *((DesInData*)(invoid));
		DesOutData & out	= *((DesOutData*)(outvoid));

		double	binRange		= 360 / _orientNumBin,
				binVelozRange	= _maxMagnitude / (float)_magnitudeBin;

		int		cubPos			= 0,
				numPixels		= (in.second[0].xf - in.second[0].xi) *
								  (in.second[0].yf - in.second[0].yi);
		
		for (auto & cuboid : in.second ) //for each cuboid
		{

			HistoType histogram(1,	_orientNumBin * 
									(_magnitudeBin + 1) *
									_densityNumBin);
			histogram = histogram * 0;
			for (auto & imgPair : in.first) // for each image
			{
				/*Mat frm(imgPair.second.rows,imgPair.second.cols, CV_8SC3);
				frm = frm * 0;
				cv::rectangle(	frm,
					cv::Point(cuboid.yi , cuboid.xi),
					cv::Point(cuboid.yf, cuboid.xf) ,
					cv::Scalar(0, 0, 255) );
				for (int i = cuboid.xi; i <= cuboid.xf; ++i)
				{
					for (int j = cuboid.yi; j <= cuboid.yf; ++j)
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
		fs["descriptor_densityNumBin"]	>> _densityNumBin;
	}
};

*/