#pragma once

#include "opencv2/highgui/highgui.hpp"
#include <vector>
#include <map>
#include <string.h>
#include <fstream>
#include <iostream>
#include <assert.h>
#include "Support.h"
#include "CUtil.h"
#include "Descriptor.h"
#include <queue>
#include <assert.h>
#include <thread>
#include <functional>
#include <algorithm>
#include "OFCM\ofcm_features.hpp"
#include "OFCM\descriptor_temporal.hpp"
#include "BorderOf.h"
#include "DataStructures.h"

using namespace std;
using namespace cv;

static const int num_threads = 10;
void determinePatterns	( Mat & train, Mat & test, vector<bool> & out, float thr, int type);
void loadImages2Vec		( std::string &, int, std::string &, float,
						  int, std::vector< cv::Mat > &);
//=================================================================
//CrowdAnomalies main class of the project.........................
class CrowdAnomalies
{
	int	_main_frame_interval,	//same 4 test & train * numframes-1
      _main_frame_range,		//4 spatiotemporal range
      _main_cuboid_width,		//cuboids width
      _main_cuboid_height,	//cuboids height
      //if the there is no overlap then thya have the same dim
      //as cubois iwdth and height
      _main_cuboid_over_width,	//cuboids width overlap
      _main_cuboid_over_height,	//cuboids height overlap
      _main_descriptor_type,
      _main_descriptor_type_extract;
	double		  _scale;
  FileStorage _fs;
  string		  _mainfile;

	//MAIN FUNCTIONS....................................................
	void	Precompute_OF();

	void	Precompute_Gabor();

	void	Feat_Extract();

	void	Test_Offline();

	void	Test_Inline();

	void	GTValidation(vector<vector<bool> > &);
  
  void	GTValidation_Ranges(vector<vector<bool> > &);

  void  ComputeThrValueForTrain();

  void  Feat_Extract_OFCM();


	//SUPPORT FUNCTIONS.................................................
	void	Feat_Extract_OM();

	void	DescribeSeq		( DirectoryNode &, vector<cutil_grig_point> &, Mat &,
							  string &, string & );

	void	Feat_Extract_Gabor();

	void	Feat_Extract_Online();

  void  RepairPeds2();
  
	//..................................................................
	void	CommonLoadInfo	( cutil_file_cont &, string &, string, const char *, 
						      vector<cutil_grig_point> &, short &, short & );

	bool	CommonLoadInfo	( DirectoryNode *, string, const char *,
							  vector<cutil_grig_point> &, short &, short & );

	void	Graphix			( vector<vector<bool> > & );

  

	
public:
	CrowdAnomalies(string featf);
	~CrowdAnomalies(){}
	void Execute();
};

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
////////////////MAIN    FUNCTIONS////////////////////////////////////////

CrowdAnomalies::CrowdAnomalies(string featf){
	_mainfile = featf;
	cout << _mainfile << endl;
	_fs = FileStorage(featf, FileStorage::READ);
	_fs["main_frame_interval"]		        >> _main_frame_interval;
	_fs["main_frame_range"]			          >> _main_frame_range;
	_fs["main_cuboid_width"]		          >> _main_cuboid_width;
	_fs["main_cuboid_height"]		          >> _main_cuboid_height;
	_fs["main_cuboid_over_width"]       	>> _main_cuboid_over_width;
	_fs["main_cuboid_over_height"]	      >> _main_cuboid_over_height;
	_fs["main_descriptor_type"]		        >> _main_descriptor_type;
	_fs["main_descriptor_type_extract"]		>> _main_descriptor_type_extract;
	_fs["main_image_scale"]			          >> _scale;

}

//=================================================================
//
void CrowdAnomalies::Execute()
{
	int	 op = -1;
	_fs["main_execute_op"] >> op;
	switch (op)
	{
		case 0:{
			Precompute_Gabor();
			break;
		}
		case 1:{
			Precompute_OF();
			break;
		}
		case 2:{ 
		  Feat_Extract();
			break;
		}
		case 3:{
			Test_Offline();
			break;
		}
		case 4:{
			Test_Inline();
			break;
		}
    case 5:{
      ComputeThrValueForTrain();
      break;
    }
    case 6:{
      RepairPeds2();
      break;
    }
    case 7:{
      Feat_Extract_OFCM();
      break;
    }
		case 10:{//for test offline
			Feat_Extract();
			Test_Offline();
			break;
		}
		default:{}
	}
}
//==================================================================
//Function to precompute the optical flow of some directory to other
//requeriments in the configuration file............................
//main_precompute_of_dir = directory................................
//main_precompute_of_ext = file image extension.....................
//main_precompute_of_out = directory out............................
//main_precompute_of_type = type of source /1 folder /2 video.......
void CrowdAnomalies::Precompute_OF()
{
  cout << "Precompute_oF" << endl;
  string	directory,
          out_directory,
          file_extension;
  int			type,
          video_step;
  OFdataType	      image_vector;
  OFvecParMat	      of_out;
  OpticalFlowBase		*oflow = nullptr;
  //load info....................................................

  _fs["main_precompute_of_type"] >> type;       //
  _fs["main_precompute_of_video_step"] >> video_step; //
  _fs["main_precompute_of_dir"] >> directory;
  _fs["main_precompute_of_ext"] >> file_extension;
  _fs["main_precompute_of_out"] >> out_directory;

  //.............................................................
  //choosing the optical flow technique
  switch (type){
  case 0:
           oflow = new OpticalFlowOCV; // opencv pyramid of
           break;
  
  case 1: 
          oflow = new OpticalFlowBorder; // william's friend 
          break;
  }

  //.............................................................
  //loading the images into a vector
	loadImages2Vec(directory, type, file_extension, _scale, video_step, image_vector);
  //computing the optical flow of all images
	oflow->compute(image_vector, of_out);
  //for two images we have a magnitude and orientation
	cutil_create_new_dir_all(out_directory);
	for (size_t i = 0; i < of_out.size(); ++i)
	{
		stringstream outfile;
		outfile << out_directory<< "/opticalflow_" << insert_numbers(i+1, of_out.size()) << "of.yml";
		FileStorage fs(outfile.str(), FileStorage::WRITE);
		fs << "angle" << of_out[i].first;
		fs << "magnitude" << of_out[i].second;
		fs.release();
	}
	delete oflow;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//______________________________________________________________________________
//function that extract gabor information of directory of images
//inputs..................................................................	
//directory : directory with images.......................................
//ext : file extension....................................................
//orientation scales : for gabor filter ..................................
//gaborType : if want the sum or max value for scales....................
void CrowdAnomalies::Precompute_Gabor()
{
	cout << "Precompute_gabor" << endl;
	string	directory,
		      ext,
			    out_directory;
	int		  orientation,
          scales,
          gaborType,
          wdsize;
	cutil_file_cont fileList;
	//...................................................................
	//loading info
	_fs["main_precompute_gabor_dir"]			>> directory;
	_fs["main_precompute_gabor_out_dir"]		>> out_directory;
	_fs["main_precompute_gabor_ext"]			>> ext;
	_fs["main_precompute_gabor_orientation"]	>> orientation;
	_fs["main_precompute_gabor_scales"]			>> scales;
	_fs["main_precompute_gabor_type"]			>> gaborType;
	_fs["main_precompute_gabor_wdsize"]			>> wdsize;
	//...................................................................
	list_files_all(fileList, directory.c_str(), ext.c_str());
	cutil_create_new_dir_all(out_directory);
	//introducing the thread
	for (auto & filename : fileList)
	{
		cout << filename << endl;
		Mat		img = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
		if (_scale > 0)
			resize(img, img, Size(), _scale, _scale, INTER_CUBIC);
		auto	vecGabor = supp_gabor_filter(img, scales, orientation, cv::Size(wdsize, wdsize), gaborType);
		string  saveFile = out_directory + "/" + 
						   cutil_LastName(filename) + "_gabor.yml";
		FileStorage fs(saveFile, FileStorage::WRITE);
		for (size_t i = 0; i < vecGabor.size(); ++i)
		{
			stringstream ss;
			ss << "scale" << i;
			fs << ss.str() << vecGabor[i];
		}
		
	}
}
//==================================================================
//Train usin precomputed optical flow images........................
//requeriments in the configuration file............................
//main_train_dir = directory................................
void CrowdAnomalies::Feat_Extract()
{
	switch (_main_descriptor_type_extract)
	{
	case 1:{
		Feat_Extract_OM();
		break;
	}
	case 2:{
	   Feat_Extract_Gabor();
	   break;
	}
	case 3:
	{
		Feat_Extract_Online();
	}
  }
}
//==================================================================
//Test using precomputed histograms.................................
//requeriments in the configuration file............................
//main_test_of_dir = test directory (optical flow)..................
//main_test_of_train_file = train histograms file...................
//returns 
//main_test_of_output = output metrics
void CrowdAnomalies::Test_Offline()
{
	string		testFile,
				    trainFile,
            flagGraphix,
            flagGtval;
	Mat			  train,
				    test;
	short		  cuboidnumber, 
				    distancetype,
            validation_type;
	float		  threshold;
	vector<Mat_<float> >	info_out;
	
	
	//-------------------------------------------------------------
	//load info....................................................
	_fs["main_test_of_test_file"]	  >> testFile;
	_fs["main_test_of_train_file"]	>> trainFile;
	_fs["main_test_of_threshold"]	  >> threshold;
	_fs["main_test_of_distance_type"]	>> distancetype;
	_fs["main_test_of_graphix_flag"]	>> flagGraphix;
	_fs["main_test_of_gtvalidate_flag"]	>> flagGtval;
  _fs["main_test_of_validation_type"] >> validation_type;

	//-------------------------------------------------------------
	FileStorage testfs(testFile, FileStorage::READ);
	FileStorage trainfs(trainFile, FileStorage::READ);
	trainfs["CuboidNumber"] >> cuboidnumber;
	vector<vector<bool> > finaloutvec(cuboidnumber);

	//adding threads...............................................
	vector<thread> t(num_threads);
	for (auto i = 0; i < cuboidnumber; ++i){
		stringstream keyphrase;
		keyphrase << "cuboid" << i;
		trainfs[keyphrase.str()]  >> train;
		testfs[keyphrase.str()]   >> test;
		determinePatterns(train, test, finaloutvec[i], threshold, distancetype);
		cout << keyphrase.str()   << endl;
		//thread increment________________________________________
		/*t[i % num_threads] = thread(determinePatterns, ref(train), ref(test), ref(finaloutvec[i]), threshold, i);
		if ( (i + 1) % num_threads == 0)
			for (auto thr = 0; thr < num_threads; ++thr)
				t[thr].join();
		//________________________________________________________/**/
	}
	if (flagGtval == "true"){
    switch (validation_type) {
    case 0:
      GTValidation(finaloutvec);
      break;
    case 1:
      GTValidation_Ranges(finaloutvec);
      break;
    }
	}
	if (flagGraphix == "true"){
		Graphix(finaloutvec);
	}
	//------------------------------------------------------------
	
	
}
/////////////////////////////////////////////////////////////////////////
//validation function with GT
static inline void	gridGt(Mat_<int> &img, vector<cutil_grig_point> &grid, vector<bool>  &res)
{
	res.clear();
	res.resize(grid.size());
	int cont = 0;
	for (auto & cuboid : grid ) //for each cuboid
	{
		res[cont] = false;
		/*Mat frm(img.rows,img.cols, CV_8SC3);
		frm = frm * 0;
		cv::rectangle(	frm,
			cv::Point(cuboid.yi , cuboid.xi),
			cv::Point(cuboid.yf, cuboid.xf), 
			cv::Scalar(0, 0, 255) );/**/
		for (int i = cuboid.xi; i <= cuboid.xf; ++i)
		{
			for (int j = cuboid.yi; j <= cuboid.yf; ++j)
			{
				if (img(i, j))
					res[cont] = true;
			}
		}
		++cont;
	}
}
/////////////////////////////////////////////////////////////////////////
//validation function with GT
static inline void	gridGt2(Mat_<int> &img, vector< vector<bool> >  &res, int nres )
{
	int pos = 0;
	for (int i = 0; i < img.rows; ++i)
	{
		for (int j = 0; j < img.cols; ++j)
		{
			img(i, j) = res[pos++][nres] ? 0 : 255;
		}
	}
	
}
////////////////////////////////////////////////////////////////////////////////
//result : 
//0 = tp, 
//1 = tn, 
//2 = fp, 
//3 = fn;
static inline void	Validate_FrmLvl_UCSD(Mat_<int> &img, vector<cutil_grig_point> &grid, 
							vector<vector<bool>> &resGrid, int pos, Metric_units & munit)
{
  cout << "Validate Frame Level UCSD img: " << pos << endl;
	vector<bool>	gt_grid;
	gridGt(img, grid, gt_grid);
	bool	gt_anomaly	= false,
			res_anomaly = false,
			both		= false;
	for (size_t i = 0; i < gt_grid.size(); ++i)
	{
		if (gt_grid[i])
			gt_anomaly = true;
		if (!resGrid[i][pos])
			res_anomaly = true;
		if (!resGrid[i][pos] && gt_grid[i])
			both = true;
	}
	int mtype;

	if ( !gt_anomaly && !res_anomaly )	mtype = 1;

	if ( !gt_anomaly &&  res_anomaly )	mtype = 2;

	if ( gt_anomaly  && !res_anomaly )	mtype = 3;

	if ( gt_anomaly  && res_anomaly  )  mtype = 0;

	++munit[mtype];
}

////////////////////////////////////////////////////////////////////////////////
//result : 
//0 = tp, 
//1 = tn, 
//2 = fp, 
//3 = fn;
static inline void Validate_PixelLvl_Train(Mat_<int> &img, vector<cutil_grig_point> &grid, 
							vector<vector<bool>> &resGrid, int pos, Metric_units & munit)
{
	Mat_<int>	res(img.rows, img.cols);
	Mat_<int>	metrics(img.rows, img.cols);
	
  gridGt2(res, resGrid, pos);
	
	for (int i = 0; i < res.rows; ++i){	
		for (int j = 0; j < res.cols; ++j){
      if (res(i, j) && img(i, j))
        metrics(i, j) = 0;
      else if (res(i, j) == 0 && img(i, j) == 0)
        metrics(i, j) = 1;
      else if (res(i, j)      && img(i, j) == 0)
        metrics(i, j) = 2;
      else if (res(i, j) == 0 && img(i, j)     )
        metrics(i, j) = 3;
    }
	}
  for (int i = 0; i < res.rows; ++i){
    for (int j = 0; j < res.cols; ++j){
      if (metrics(i, j) == 0){
        int p1, 
            p2;
        for (int k = -1; k < 1; ++k){
          for (int l = -1; l <= 1; ++l)
          {
            p1 = i + k;
            p2 = j + l;
            if (p1 >= 0 && p1 < res.rows &&
              p2 >= 0 && p2 < res.cols){
              if (metrics(p1, p2)>1)
                metrics(p1, p2) = -1;
            }
          }
        }
      }
    }
  }
  for (int i = 0; i < res.rows; ++i){
    for (int j = 0; j < res.cols; ++j){
      if (metrics(i, j) >= 0)
      {
        ++munit[metrics(i, j)];
      }
    }
  }
	
}
/*
int p1, 
            p2;
        for (int k = -1; k < 1; ++k){
          for (int l = -1; l <= 1; ++l)
          {
            p1 = i + k;
            p2 = j + l;
            if (p1 >= 0 && p1 < res.rows &&
                p2 >= 0 && p2 < res.cols){

            }
           }
        }
       */
////////////////////////////////////////////////////////////////////////////////
//type of pointer to validate type
typedef void (*FunValType) (Mat_<int> &, vector<cutil_grig_point> &, vector<vector<bool> > &, int, Metric_units &);

//directory: where is the gt
//token: witch type of file is the input
//out_file: 
//validation type: 
//[0] --> true possitive rate vs false positive rate
//[1] --> precision recall                  
void CrowdAnomalies::GTValidation(vector<vector<bool> > & rpta){
	string	directory,
			    token,
			    out_file;
	short	  rows, 
			    cols,
			    validationType;
	vector<FunValType>  validationFunctions{ Validate_FrmLvl_UCSD, 
											Validate_PixelLvl_Train};
	//-------------------------------------------------------------------

	cutil_file_cont				    file_list;
	vector<cutil_grig_point>	grid;	
	Mat							          img0;
	Metric_units				      munit;
	//-------------------------------------------------------------------
	//load info..........................................................

	_fs["main_gtvalidation_dir"]		  >> directory;
	_fs["main_gtvalidation_token"]		>> token;
	_fs["main_gtvalidation_out_file"]	>> out_file;
	_fs["main_gtvalidation_type"]		  >> validationType;
  _fs["main_gtvalidation_rows"]		  >> rows;
  _fs["main_gtvalidation_cols"]		  >> cols;
	

	list_files_all(file_list, directory.c_str(), token.c_str());
	sort(file_list.begin(), file_list.end());
	img0 =  imread(file_list.front());
	
	if (_scale > 0)
		resize(img0, img0, Size(), _scale, _scale, INTER_CUBIC);

	if (!rows)rows = img0.rows;
  if (!cols)cols = img0.cols;

	grid = grid_generator(rows, cols,
		_main_cuboid_width, _main_cuboid_height,
		_main_cuboid_over_width, _main_cuboid_over_height);
	//...................................................................

	string	ant = cutil_antecessor(out_file, 1);
	cutil_create_new_dir_all(ant);
	
	ofstream outFrame	(out_file.c_str(), ios::app);

	//-------------------------------------------------------------------

	int step = _main_frame_interval * _main_frame_range;
	
	for (size_t i = step-1, pos = 0; (i < file_list.size()) && 
							         (pos < rpta[0].size()); i+= step, ++pos)
	{
		//cout << i << " " << pos << endl;
		Mat img  = imread(file_list[i], CV_BGR2GRAY);
		if (_scale > 0)	resize(img, img, Size(), _scale, _scale, INTER_CUBIC);
		Mat_<int> gt	= img;
		validationFunctions[validationType] ( gt, grid, rpta, pos, munit );
		
	}
  //---------------------------------------------------------------------------
  //computing the metrics
	double	FPR = supp_FalsePositiveRate(munit),
			    TPR = supp_Recall(munit),
          PPV = supp_Precision(munit);
  

  switch (validationType)
  {
  case 0:
  {
    outFrame << FPR	<< " ";
    outFrame << TPR << endl;
    outFrame.close();
    break;
  }
  case 1:
  {
    outFrame << PPV	<< " ";
	  outFrame << TPR << endl;
	  outFrame.close();
  }
  default:
    break;
  }
	//----------------------------------------------------------------------------
}

void CrowdAnomalies::GTValidation_Ranges(vector<vector<bool> > & rpta) {
  string	file,
          out_file;
  short   validation_type;
  int     num_frames,
          test_frame_ini;
  vector< Segment<int> >  gt;

  //-------------------------------------------------------------------
  Metric_units  munit;

  //-------------------------------------------------------------------
  //load info..........................................................
  _fs["main_gtvalidation_ranges_file"]            >> file;
  _fs["main_gtvalidation_ranges_out_file"]        >> out_file;
  _fs["main_gtvalidation_ranges_validation_type"] >> validation_type;
  _fs["main_gtvalidation_ranges_num_frames"]      >> num_frames;
  _fs["main_gtvalidation_ranges_test_frame_ini"]  >> test_frame_ini;

  string	ant = cutil_antecessor(out_file, 1);
  cutil_create_new_dir_all(ant);
  
  ofstream outFrame(out_file.c_str(), ios::app);
  //-------------------------------------------------------------------
  //loading ground truth 

  ifstream  gtfile(file);
  string    line;
  while (!gtfile.eof()) {
    getline(gtfile,line);
    auto strs = cutil_split(line);
    if (static_cast<int>(strs.size()) > 1) {
      Segment<int> sg( stoi(strs[0]), stoi(strs[1]) );
      gt.push_back(sg);
    }
  }
  
  int step = _main_frame_interval * _main_frame_range;

  for (size_t i = test_frame_ini + step - 1, pos = 0; 
      pos < rpta[0].size(); 
      i += step, ++pos){
    cout << i << " " << pos << endl;
    
    bool	gt_anomaly = datastr_findSegmentVec<int>(gt, static_cast<int>(i)),
          res_anomaly = false,
          both = false;
    for (size_t j = 0; j < rpta.size(); ++j){
      if (!rpta[j][pos])
        res_anomaly = true;
      if (!rpta[j][pos] && gt_anomaly)
        both = true;
    }

    int mtype;

    if (!gt_anomaly && !res_anomaly)	mtype = 1;

    if (!gt_anomaly &&  res_anomaly)	mtype = 2;

    if (gt_anomaly  && !res_anomaly)	mtype = 3;

    if (gt_anomaly  && res_anomaly)  mtype = 0;

    ++munit[mtype];

  }
  gtfile.close();
  //---------------------------------------------------------------------------
  //computing the metrics
  double	FPR = supp_FalsePositiveRate(munit),
          TPR = supp_Recall(munit),
          PPV = supp_Precision(munit);


  switch (validation_type)
  {
  case 0:
  {
    outFrame << FPR << " ";
    outFrame << TPR << endl;
    outFrame.close();
    break;
  }
  case 1:
  {
    outFrame << PPV << " ";
    outFrame << TPR << endl;
    outFrame.close();
  }
  default:
    break;
  }
  //----------------------------------------------------------------------------
  /**/
}





//in this part we present the code for the inline testa
//in the execute function the vcorresponding value is 
//main_execute_op = 4
void CrowdAnomalies::Test_Inline()
{

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//feat extraction online extracts from video or images source the features
//
void CrowdAnomalies::Feat_Extract_Online()
{
	string	dir_out,
				  token_out;
	short		rows = 0,
				  cols,
				  sourceType;
	vector<cutil_grig_point> grid;
	//-------------------------------------------------------------
	//load info....................................................
	_fs["main_feat_extract_source_type"]	>> sourceType;
	_fs["main_feat_extract_output"]			  >> dir_out;
	_fs["main_feat_extract_token_out"]		>> token_out;
  _fs["main_feat_extract_rows"]		      >> rows;
  _fs["main_feat_extract_cols"]		      >> cols;
	cutil_create_new_dir_all(dir_out);
	//.............................................................
	//sourcetype = 1 for videos
	if (sourceType == 1)
	{
		int	posini,
				posfin;
    
    Mat img;
		
		string	vidFile;

		//loading from mainfile
    _fs["main_feat_extract_video_pos_ini"]	>> posini;
		_fs["main_feat_extract_video_pos_fin"]	>> posfin;
		_fs["main_feat_extract_video_file"]		  >> vidFile;
		//.........................................................

		OFdataType		image_vector;
		OFvecParMat		of_out;
		OpticalFlowBase	*oflow	= new OpticalFlowOCV;
		
		//.........................................................
		//video characteristics 
		int nframes;
		MyVideoCapture cap(vidFile);
    cap >> img;
    if (_scale > 0)
			  resize(img, img, Size(), _scale, _scale, INTER_CUBIC);
    if(!rows)rows = img.cols;
    if(!cols)cols = img.rows;
    nframes = cap.get(CV_CAP_PROP_FRAME_COUNT);

		grid = grid_generator(cols, rows,
				_main_cuboid_width,		 _main_cuboid_height,
				_main_cuboid_over_width, _main_cuboid_over_height);
		//.........................................................

		OFBasedDescriptorBase * descrip = selectChildDes(_main_descriptor_type, _mainfile);
		Trait_OM::DesInData		input;
		Trait_OM::DesOutData	vecOutput(grid.size());	
		input.second = grid;

		for (int i = posini, range =1; i < posfin && i < nframes; i += _main_frame_interval, ++range)
		{
			cout << "Frame: " << i << endl;
			cap.set(CV_CAP_PROP_POS_FRAMES, i);
			cap >> img;
      if (_scale > 0)
			  resize(img, img, Size(), _scale, _scale, INTER_CUBIC);
			image_vector.push_back(img.clone());
			if (range % _main_frame_range == 0)
			{
				oflow->compute(image_vector, of_out);
				input.first = of_out;

				descrip->Describe(&input, &vecOutput);

				image_vector.clear();
				of_out.clear();
			}
		}
		string path = dir_out + "/" + cutil_LastName(vidFile) + token_out;
		supp_vectorMat2YML< Mat_<float> >(vecOutput, path, string("cuboid"));
	}

}


////////////////////////////////////////////////////////////////////////////////
//this function receives the matrix file to compute the thr distance for train 
//matrix 

//op = 5
//computethrvaluefortrain_file      train file
//computethrvaluefortrain_out_file  outputfile
void CrowdAnomalies::ComputeThrValueForTrain()
{
  string  trainFile,
          out_file;

  int     cuboidnumber;

  float   amount;

  _fs["computethrvaluefortrain_file"]     >> trainFile;
  _fs["computethrvaluefortrain_out_file"] >> out_file;
  _fs["computethrvaluefortrain_amount"]   >> amount;
  //....................................................................
  FileStorage trainfs(trainFile, FileStorage::READ);
  FileStorage outfs(out_file, FileStorage::WRITE);
	trainfs["CuboidNumber"] >> cuboidnumber;
	
  //adding threads...............................................
	
  Mat_<float> train,
              thrOut(1, cuboidnumber);

	for (auto i = 0; i < cuboidnumber; ++i){
		stringstream keyphrase;
		keyphrase << "cuboid" << i;
    cout << keyphrase.str() << endl;
		trainfs[keyphrase.str()]  >> train;
    thrOut(0, i) = supp_computeMeanDistanceTrain(train, amount);
	}
  outfs << "Thrs" << thrOut;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void CrowdAnomalies::Feat_Extract_OFCM(){
  string		directory,
				    dir_out,
				    token,
				    token_out;
	short		  rows,
				    cols,
            nBinsMagnitude,
            nBinsAngle,
            distanceMagnitude,
            distanceAngle,
            cuboidLength,
            maxMagnitude,
            logQuantization,
            movementFilter,
            temporalScales;
	cutil_file_cont				    file_list;
	//vector<cutil_grig_point>	grid;
  vector<int>               vect;
	vector<Mat>		            in;
  vector<Cube>              cuboids;
  Mat                       output;
	//-------------------------------------------------------------
	//load info....................................................
	_fs["main_feat_extract_ofcm_dir"] >> directory;
	_fs["main_feat_extract_ofcm_output"] >> dir_out;
	_fs["main_feat_extract_ofcm_token_of"]   >> token;
	_fs["main_feat_extract_ofcm_token_out"]  >> token_out;

  _fs["nBinsMagnitude"] >> nBinsMagnitude;
  _fs["nBinsAngle"] >> nBinsAngle;
  _fs["distanceMagnitude"] >> distanceMagnitude;
  _fs["distanceAngle"] >> distanceAngle;
  _fs["cuboidLength"] >> cuboidLength;
  _fs["maxMagnitude"] >> maxMagnitude;
  _fs["logQuantization"] >> logQuantization;
  _fs["movementFilter"] >> movementFilter;
  _fs["temporalScales"] >> temporalScales;
  vect.push_back(temporalScales);
  //.............................................................
  list_files_all(file_list, directory.c_str(), token.c_str());
  in.resize(file_list.size());
	cutil_create_new_dir_all(dir_out);
	//introducing the thread
  size_t i = 0;
	for (auto & filename : file_list)
	{
		cout << filename << endl;
		Mat		img = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
		if (_scale > 0)
			resize(img, img, Size(), _scale, _scale, INTER_CUBIC);
    in[i++] = img;
	}
  rows = in[0].rows;
  cols = in[0].cols;

  /////////////////////// Create cube vector (dense sampling) ////////////////////////////
  short sampleX = _main_cuboid_width,
        sampleY = _main_cuboid_height,
        sampleL = _main_frame_range,
        strideX = _main_cuboid_over_width,
        strideY = _main_cuboid_over_height,
        strideL = _main_frame_range;
        
  for (int x = 0; x <= static_cast<int>(0 + rows - sampleX); x += strideX){
    for (int y = 0; y <= static_cast<int>(0 + cols - sampleY); y += strideY){
        for (int t = 0; t <= static_cast<int>(0 + in.size() - sampleL); t += strideL){ //video.size() instead of videoLength since we used a frameStep different from 1
          cuboids.push_back(Cube(x, y, t, sampleX, sampleY, sampleL));
      }
    }
  }
  ////////////////////////////////////////////////////////////////////////////////////////

  DescriptorTemporal * desc = new OFCM(nBinsMagnitude, nBinsAngle, distanceMagnitude, distanceAngle, cuboidLength, maxMagnitude, logQuantization, static_cast<bool>(movementFilter), vect);
  desc->setData(in);
  desc->extract(cuboids, output);
  
  vector<Mat_<float>> vecout;
  int observed = in.size() / sampleL;

  for (int i = 0; i < output.rows; i+=observed){
    Mat_<float> cuboid_descriptor(observed,output.cols);
    for (int j = i, x =0; j < i + observed; ++j,++x){
      for (int k = 0; k < output.cols; ++k){
        cuboid_descriptor(x, k) = output.at<float>(j,k);
      }
    }

    vecout.push_back(cuboid_descriptor);
  }
  
  string path = dir_out + "/" + token_out;
	supp_vectorMat2YML<Mat_<float>>(vecout, path, string("cuboid"));
		
	cout << " Des-OK\n";

  delete desc;

  //FileStorage fs(dir_out, FileStorage::WRITE);
  
}





/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
////////////////SUPPORT FUNCTIONS////////////////////////////////////////
//simple repettive code for principal functions
void CrowdAnomalies::CommonLoadInfo(cutil_file_cont & file_list, string & directory,
string key, const char * token, vector<cutil_grig_point> & grid, short &rows, short  &cols){
	Mat			img;
	list_files_all(file_list, directory.c_str(), token);
	if(key=="")
		img =  imread(file_list.front());
	else{
		FileStorage imgfs(file_list.front(), FileStorage::READ);
		imgfs[key] >> img;
	}
	rows = img.rows;
	cols = img.cols;
	grid = grid_generator(rows, cols,
		_main_cuboid_width, _main_cuboid_height,
		_main_cuboid_over_width, _main_cuboid_over_height);
}

//-----------------------------------------------------------------------------
//funtion  that determine patterns using simple distance
//receives matrix patterns
static void determinePatterns(Mat & train, Mat & test, vector<bool> & out, float thr, int type)
{
	out.clear();
	out.resize(test.rows);
	
  switch (type)
	{
		case 1:{
			supp_SimpleDistance(train, test, out, thr);
			break;
		}
		case 2:{
			supp_mahalanobisDistanceFunction(train, test, out, thr);
			break;
		}
		case 3:{
			supp_ComputeDistaceSamples_Train_Figtree(train, test, out, thr);
			break;
		}
    case 4:{
      supp_meanThrBasedDistance(train, test, out, thr);  
      break;
    }
    case 5:{
      supp_SimpleDistance_cometoguether(train, test, out, thr);
      break;
    }
	}
	
	//
}
////////////////////////////////////////////////////////////////////////////////
//==========================================================================
//draw rectangles
void ShowAnomaly(cv::Mat & frm, int pos, std::vector<std::vector<bool> > & an, 
				 std::vector<cutil_grig_point> & grid)
{
	for (std::size_t i = 0; i < an.size(); ++i)
	{
    if (!an[i][pos])
    {
      cv::Scalar color = cv::Scalar(0, 0, 255);

      for (int x = grid[i].xi; x <= grid[i].xf; ++x){
        for (int y = grid[i].yi; y <= grid[i].yf; ++y){
          Vec3b color = frm.at<Vec3b>(Point(y, x));
          color.val[1] = 0;
          frm.at<Vec3b>(Point(y, x)) = color;
        }
      }

		  cv::rectangle(	frm,
					cv::Point(grid[i].yi, grid[i].xi),
					cv::Point(grid[i].yf, grid[i].xf), 
					color );
      
    }
		/*cv::Scalar color = an[i][pos] ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
		cv::rectangle(	frm,
					cv::Point(grid[i].yi, grid[i].xi),
					cv::Point(grid[i].yf, grid[i].xf), 
					color );*/
	}
}
////////////////////////////////////////////////////////////////////////////////
//Drwaing anomalies function 
//Req: param : vector vector rptas (anommaly or not)
void CrowdAnomalies::Graphix( vector<vector<bool> > &rpta )
{	
	string	file_extension,
			file_extension_out,
			directory_out,
			token_out;
	short	rows,
			cols,
			source_type;
	vector<cutil_grig_point>	grid;
	//....................................................................
	//loading generals

	_fs["support_graphix_out_dir"]		>> directory_out;
	_fs["support_graphix_out_token"]	>> token_out;
	_fs["support_graphix_out_ext"]		>> file_extension_out;
	_fs["support_graphix_source_type"]	>> source_type;
  _fs["support_graphix_rows"]		  >> rows;
  _fs["support_graphix_cols"]		  >> cols;

	//--------------------------------------------------------------------

	cutil_create_new_dir_all(directory_out);
	//=====================================================================

	if (source_type == 0){

		string						directory;
		cutil_file_cont				file_list;
		
		_fs["support_graphix_ext"] >> file_extension;
		_fs["support_graphix_dir"] >> directory;

		cutil_create_new_dir_all(directory_out);

		Mat			img0;
		list_files_all(file_list, directory.c_str(), file_extension.c_str());
    sort(file_list.begin(), file_list.end());
		img0 = imread(file_list.front());
		if (_scale > 0)
			resize(img0, img0, Size(), _scale, _scale, INTER_CUBIC);
		rows = img0.rows;
		cols = img0.cols;
		grid = grid_generator(rows, cols,
			_main_cuboid_width, _main_cuboid_height,
			_main_cuboid_over_width, _main_cuboid_over_height);

		int step = _main_frame_interval * _main_frame_range;
		for (size_t i = step - 2, pos = 0; i < file_list.size() &&
			pos < rpta[0].size(); i += step, ++pos)
		{
			stringstream strout;
			strout << directory_out << "/" << token_out << pos << "." << file_extension_out;
			Mat img = imread(file_list[i]);
			if (_scale > 0)
				resize(img, img, Size(), _scale, _scale, INTER_CUBIC);
			ShowAnomaly(img, pos, rpta, grid);
			cout << "write: " << strout.str() << endl;
			imwrite(strout.str(), img);
		}
	}
  else if (source_type == 1)
  {

    string	file;

    int		  nframes,
      ini,
      fin;

    Mat     img;

    _fs["support_graphix_video_file"] >> file;
    _fs["support_graphix_video_ini"] >> ini;
    _fs["support_graphix_video_fin"] >> fin;

    //.........................................................................
    MyVideoCapture cap(file);
    cap >> img;
    if (_scale > 0)
      resize(img, img, Size(), _scale, _scale, INTER_CUBIC);
    //rows = img.cols;
    //cols = img.rows;

    if(!rows)rows = img.cols;
    if(!cols)cols = img.rows;



    nframes = cap.get(CV_CAP_PROP_FRAME_COUNT);

    //.........................................................................
    grid = grid_generator(cols, rows,
      _main_cuboid_width, _main_cuboid_height,
      _main_cuboid_over_width, _main_cuboid_over_height);

    //.........................................................................
    for (int i = ini, pos = 0; i < fin && i < nframes && pos < rpta[0].size(); i += (_main_frame_interval*_main_frame_range), ++pos)
		{
			stringstream strout;
			strout << directory_out << "/" << token_out << pos << "." << file_extension_out;

			cout << "Frame: " << i << endl;
			Mat img;
			cap.set(CV_CAP_PROP_POS_FRAMES, i);
			cap >> img;
			if (_scale > 0)	resize(img, img, Size(), _scale, _scale, INTER_CUBIC);

			ShowAnomaly(img, pos, rpta, grid);
			cout << "write: " << strout.str() << endl;
			imwrite(strout.str(), img);
		}

	}
}

////////////////////////////////////////////////////////////////////////////////
//
bool CrowdAnomalies::CommonLoadInfo ( DirectoryNode *root, string key, const char * token,
	vector<cutil_grig_point> & grid, short &rows, short  &cols)
{
	Mat	img;
	queue<DirectoryNode *>  que;
	for (que.push(root); !que.empty();){
		auto current = que.front();
		que.pop();
		if (current->_listFile.size()){
			if (key == "")
				img = imread(current->_listFile.front());
			else{
				FileStorage imgfs(current->_listFile.front(), FileStorage::READ);
				imgfs[key] >> img;
			}
			rows = img.rows;
			cols = img.cols;
			grid = grid_generator(rows, cols,
				_main_cuboid_width, _main_cuboid_height,
				_main_cuboid_over_width, _main_cuboid_over_height);
			return true;
		}
		for (auto & currentSon : current->_sons)
			que.push(currentSon);
	}
	return false;

}
////////////////////////////////////////////////////////////////////////////////
//Feat extraction OM creates feature vector using the orientation magnitude.....
//descriptor....................................................................
//inputs........................................................................
//directory with of
//direcotry output
//token of oftical flow files
//token out
void CrowdAnomalies::Feat_Extract_OM(){
	string		directory,
				dir_out,
				token,
				token_out;
	short		rows,
				cols;
	cutil_file_cont				file_list;
	vector<cutil_grig_point>	grid;
	vector<Mat_<float> >		info_out;
	//-------------------------------------------------------------
	//load info....................................................
	_fs["main_feat_extract_dir_of"] >> directory;
	_fs["main_feat_extract_output"] >> dir_out;
	_fs["main_feat_extract_token_of"] >> token;
	_fs["main_feat_extract_token_out"] >> token_out;

	cutil_create_new_dir_all(dir_out);
	DirectoryNode	root(directory);
	list_files_all_tree(&root, token.c_str());
	queue<DirectoryNode *> nodelist;

	assert(CommonLoadInfo(&root, "angle", token.c_str(), grid, rows, cols));

	//..........................................................................
	Mat		mainOut;
	for (nodelist.push(&root); !nodelist.empty();)
	{
		auto current = nodelist.front();
		nodelist.pop();

		for (auto & currentSon : current->_sons)
			nodelist.push(currentSon);

		if (current->_listFile.size()){
			DescribeSeq(*current, grid, mainOut, dir_out, token_out);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
//this function describes precomputed of images extracted from video////////////
void CrowdAnomalies::DescribeSeq	( DirectoryNode & current, vector<cutil_grig_point> & grid, 
									  Mat & mainOut, string & outDir,string & outToken){
	
	OFBasedDescriptorBase * descrip = selectChildDes(_main_descriptor_type, _mainfile);
	Trait_OM::DesOutData	vecOutput(grid.size());
	
	size_t			step = _main_frame_range - 1;
	Trait_OM::DesvecParMat	temporalset(step);
	Trait_OM::DesInData		input;
	input.second = grid;
	
	for (size_t i = 0; i < current._listFile.size(); i += step+1)
	{
		cout << i << endl;
		for (int j = 0; j < step; ++j)
		{
			FileStorage imgfs(current._listFile[i + j], FileStorage::READ);
			imgfs["angle"]		>> temporalset[j].first;
			imgfs["magnitude"]	>> temporalset[j].second;
		}
		input.first = temporalset;
		descrip->Describe(&input, &vecOutput);
	}
	string path = outDir + "/" + cutil_LastName(current._label) + outToken;
	supp_vectorMat2YML< Mat_<float> >(vecOutput, path, string("cuboid"));
		
	cout << " Des-OK\n";
	delete descrip;/**/
}
////////////////////////////////////////////////////////////////////////////////
//Feat extraction OM creates feature vector using the orientation magnitude.....
//descriptor....................................................................
//inputs........................................................................
//directory with of
//direcotry output
//token of oftical flow files
//token out
void CrowdAnomalies::Feat_Extract_Gabor(){
	cout << "feat_gabor" << endl;
	string		directory_of,
				directory_gabor,
				dir_out,
				token_of,
				token_gabor,
				token_out;
	short		rows,
				cols,
				nscales;
	
	vector<cutil_grig_point>	grid;
	vector<Mat_<float> >		info_out;
	//-------------------------------------------------------------
	//load info....................................................
	_fs["main_feat_extract_dir_of"]			>> directory_of;
	_fs["main_feat_extract_dir_gabor"]	>> directory_gabor;
	_fs["main_feat_extract_output"]			>> dir_out;
	_fs["main_feat_extract_token_of"]		>> token_of;
	_fs["main_feat_extract_token_gabor"]>> token_gabor;
	_fs["main_feat_extract_token_out"]	>> token_out;
	_fs["descriptor_gaborNumBin"]			  >> nscales;

	cutil_create_new_dir_all(dir_out);
	DirectoryNode	root_of(directory_of);
	list_files_all_tree(&root_of, token_of.c_str());
	DirectoryNode	root_gabor(directory_gabor);
	list_files_all_tree(&root_gabor, token_gabor.c_str());
	

	assert(CommonLoadInfo(&root_of, "angle", token_of.c_str(), grid, rows, cols));
	OFBasedDescriptorBase * descrip = selectChildDes(_main_descriptor_type, _mainfile);
	Trait_Gabor::DesOutData Out(grid.size());
	int step = _main_frame_range - 1;
	
	//..........................................................................
	
	for (size_t i = 0; (i < root_of._listFile.size()) &&
		(i < root_gabor._listFile.size()); i += step + 1)
	{
		Trait_Gabor::DesInData	In;
		In.second = grid;
		In.first.resize(step);
		cout << i << endl;

		for (auto p_i = 0; p_i < step; ++p_i)
		{
			FileStorage imgfs(root_of._listFile[i + p_i], FileStorage::READ);
			Mat			angle, magnitude;
			imgfs["angle"]		>> angle;
			In.first [p_i].push_back(angle);
			imgfs["magnitude"]	>> magnitude;
			In.first [p_i].push_back(magnitude);

			imgfs.open(root_gabor._listFile[i + p_i],    FileStorage::READ);
			for (int sc = 0; sc < nscales; ++sc)
			{
				Mat scaleA;
				stringstream ss;
				ss << "scale" << sc;
				imgfs[ss.str()]	>> scaleA;
				In.first[p_i].push_back(scaleA);
			}
		}
		descrip->Describe(&In, &Out);
	}
	
	string path = dir_out + "/" + cutil_LastName(root_of._label) + token_out;
	supp_vectorMat2YML< Mat_<float> >(Out, path, string("cuboid"));
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CrowdAnomalies::RepairPeds2()
{
  string  directory,
          directory_out,
          token;

  _fs["main_repair_directory"]	    >> directory;
	_fs["main_repair_directory_out"]	>> directory_out;
  _fs["main_repair_token"]	        >> token;

  cutil_file_cont list;
  list_files_all(list, directory.c_str(), token.c_str());
  std::sort(list.begin(), list.end(), cmpStrNum);
  int nfiles = list.size();
  for (size_t i = 0; i < nfiles; ++i){
    string ant = cutil_antecessor(list[i], 1);
    stringstream sstr;
    //sstr << directory_out << "\\" << insert_numbers(i,nfiles);
    sstr << "rename " << cutil_invert(list[i]) << " " << insert_numbers(i+1, nfiles)<<token;
    system(sstr.str().c_str());
  }

}





void threadCrowd(string file)
{
	CrowdAnomalies cr(file);
	cr.Execute();
}

void threadedTask(string file)
{
	ifstream	infile(file);
	if (!infile.is_open())return;
	string		line;
	vector<thread> vect(num_threads);
	for (int i = 1; getline(infile, line); ++i)// 
	{
		vect[i] = thread(threadCrowd, line);
		if (i % num_threads == 0)
			for (auto & th : vect)
				th.join();
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//loading images into a vector
void loadImages2Vec(std::string &src, int type, std::string &file_ext,
	float scale, int step, std::vector< cv::Mat > & image_vector)
{
	image_vector.clear();
	switch (type)
	{
	case 1:
	{
		  cutil_file_cont		file_list;
		  list_files_all(file_list, src.c_str(), file_ext.c_str());
		  for (size_t i = 0; i < file_list.size(); ++i)
		  {
			  Mat	img = imread(file_list[i]);
			  if (scale > 0)
				  resize(img, img, Size(), scale, scale, INTER_CUBIC);
			  image_vector.push_back(img);
		  }
	}
	case 2:
	{
		  MyVideoCapture	cap(src);
		  if (cap.isOpened())
		  {
			  cv::Mat img;
			  for (short i = 0; cap.increment(step, img); i+=step){
				  if (scale > 0)
					  resize(img, img, Size(), scale, scale, INTER_CUBIC);
				  image_vector.push_back(img.clone());
			  }
		  }
	}
  
	default:
	{}
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
