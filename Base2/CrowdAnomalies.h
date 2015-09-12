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
using namespace std;
using namespace cv;

static void determinePatterns(Mat & train, Mat & test, vector<bool> & out, float thr);
//=================================================================
//CrowdAnomalies main class of the project.........................
class CrowdAnomalies
{
	FileStorage _fs;
	string		_mainfile;
	int	_main_frame_interval,	//same 4 test & train * numframes-1
		_main_frame_range,		//4 spatiotemporal range
		_main_cuboid_width,		//cuboids width
		_main_cuboid_height,	//cuboids height
		//if the there is no overlap then thya have the same dim
		//as cubois iwdth and height
		_main_cuboid_over_width,	//cuboids width overlap
		_main_cuboid_over_height,	//cuboids height overlap
		_main_descriptor_type;

	//MAIN FUNCTIONS....................................................
	void	Extract_Info();
	void	Precompute_OF();
	void	Feat_Extract();
	void	Test_Offline();
	void	GTValidation();

	//SUPPORT FUNCTIONS.................................................
	void	CommonLoadInfo (cutil_file_cont &, string &, string, const char *, 
						    vector<cutil_grig_point> &, short &, short &);
	void	Graphix	(vector<vector<bool> > &);
public:
	CrowdAnomalies(string featf);
	~CrowdAnomalies(){}
	void Execute();
};

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
////////////////MAIN    FUNCTIONS////////////////////////////////////////

CrowdAnomalies::CrowdAnomalies(string featf){
	_mainfile = featf;
	_fs = FileStorage(featf, FileStorage::READ);
	_fs["main_frame_interval"] >> _main_frame_interval;
	_fs["main_frame_range"] >> _main_frame_range;
	_fs["main_cuboid_width"] >> _main_cuboid_width;
	_fs["main_cuboid_height"] >> _main_cuboid_height;
	_fs["main_cuboid_over_width"] >> _main_cuboid_over_width;
	_fs["main_cuboid_over_height"] >> _main_cuboid_over_height;
	_fs["main_descriptor_type"] >> _main_descriptor_type;
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
			Precompute_OF();
			break;
		}
		case 1:{
			Extract_Info();
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
		case 10:{//for training
			Precompute_OF();
			Extract_Info();
			Feat_Extract();
			break;
		}
		case 11:{//for test offline
			Precompute_OF();
			Feat_Extract();
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
void CrowdAnomalies::Precompute_OF()
{
	string		directory,
				directory_out,
				file_extension;
	OFdataType	image_vector;
	OFvecParMat	of_out;
	cutil_file_cont		file_list;
	OpticalFlowBase		*oflow	= new OpticalFlowOCV;
	//load info....................................................
	_fs["main_precompute_of_dir"] >> directory;
	_fs["main_precompute_of_ext"] >> file_extension;
	_fs["main_precompute_of_out"] >> directory_out;
	//.............................................................
	list_files_all(file_list, directory.c_str(), file_extension.c_str());
	for (size_t i = 1; i <= file_list.size(); i+= _main_frame_interval)
	{
		Mat	img	= imread(file_list[i-1]);
		image_vector.push_back(img);
		if (i % _main_frame_range == 0)
		{
			oflow->compute(image_vector, of_out);
			image_vector.clear();
		}
	}
	for (size_t i = 0; i < of_out.size(); ++i)
	{
		stringstream outfile;
		outfile << directory_out<< "/opticalflow_" << insert_numbers(i+1, of_out.size()) << "of.yml";
		FileStorage fs(outfile.str(), FileStorage::WRITE);
		fs << "angle" << of_out[i].first;
		fs << "magnitude" << of_out[i].second;
		fs.release();
	}
	delete oflow;
}
//==================================================================
//Train usin precomputed the optical flow information...............
//requeriments in the configuration file............................
//main_train_dir = directory........................................
//returns info file in yml format...................................
//main_extract_info: "output file"..................................
void CrowdAnomalies::Extract_Info()
{
	string		directory,
				file_out;
	short		rows,
				cols;
	cutil_file_cont				file_list;
	vector<cutil_grig_point>	grid;
	
	//-------------------------------------------------------------
	//load info....................................................
	FileStorage info(_mainfile, FileStorage::READ);
	info["main_extract_info_dir"] >> directory;
	info["main_extract_infofile"] >> file_out;
	CommonLoadInfo(file_list, directory, "angle", "of.yml", grid, rows, cols);

	//-------------------------------------------------------------
	vector<Mat_<float> > vecMag(file_list.size());
	int	cont = 0;
	for (auto &file : file_list){
		FileStorage imgfs(file, FileStorage::READ);
		Mat	img;
		imgfs["magnitude"] >> img;
		vecMag[cont++] = img;
	}
	//-------------------------------------------------------------
	cont = 0;
	vector<Mat_<float> >		vecOut(grid.size());
	for (auto & cuboid : grid)
	{
		float	sumVal = 0, 
				maxVal = 0;
		size_t	pixels = 0;
		for (auto & magImg : vecMag)
		{
			for (int i = cuboid.xi; i < cuboid.xf; ++i)
			{
				for (int j = cuboid.yi; j < cuboid.yf; ++j)
				{
					//cout << i << " " << j << endl;
					if (magImg(i, j) > maxVal) maxVal = magImg(i, j);
					if (magImg(i, j) > 0.1){
						sumVal += magImg(i, j);
						++pixels;
					}
				}
			}
		}
		Mat_<float> out(1, 2);
		out(0, 0) = maxVal;
		out(0, 1) = sumVal / (pixels?pixels:1);
		vecOut[cont++] = out;
	}
	vectorMat2YML< Mat_<float> >(vecOut, file_out, string("cuboid"));
}

//==================================================================
//Train usin precomputed optical flow images........................
//requeriments in the configuration file............................
//main_train_dir = directory................................
void CrowdAnomalies::Feat_Extract()
{
	string		directory,
				file_out;
	short		rows, 
				cols;
	cutil_file_cont				file_list;
	vector<cutil_grig_point>	grid;
	vector<Mat_<float> >		info_out;
	//-------------------------------------------------------------
	//load info....................................................
	FileStorage info(_mainfile, FileStorage::READ);
	info["main_feat_extract_dir"] >> directory;
	info["main_feat_extract_output"] >> file_out;
	CommonLoadInfo(file_list, directory, "angle", "of.yml", grid, rows, cols);

	//-------------------------------------------------------------
	OFBasedDescriptorBase<OFBased_Trait>* descrip = selectChildDes<OFBased_Trait>(_main_descriptor_type, _mainfile);
	DesOutData		vecOutput(grid.size());
	for (size_t i = _main_frame_range - 1; i < file_list.size(); i += _main_frame_range)
	{
		DesvecParMat	temporalset(_main_frame_range);
		DesInData		input;
		for (int j = 0; j < _main_frame_range; ++j)
		{
			FileStorage imgfs(file_list[i - j], FileStorage::READ);
			imgfs["angle"] >> temporalset[_main_frame_range - j - 1].first;
			imgfs["magnitude"] >> temporalset[_main_frame_range - j - 1].second;
		}
		input.first = temporalset;
		input.second = grid;
		descrip->Describe(input, vecOutput);
	}
	vectorMat2YML< Mat_<float> >(vecOutput, file_out, string("cuboid"));
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
				file_out,
				flag;
	Mat			train,
				test;
	short		cuboidnumber;
	float		threshold;
	vector<Mat_<float> >	info_out;
	
	
	//-------------------------------------------------------------
	//load info....................................................
	FileStorage info(_mainfile, FileStorage::READ);
	info["main_test_of_test_file"] >> testFile;
	info["main_test_of_train_file"] >> trainFile;
	info["main_test_of_output"] >> file_out;
	info["main_test_of_threshold"] >> threshold;
	info["main_test_of_graphix_flag"] >> flag;
	//-------------------------------------------------------------
	FileStorage testfs(testFile, FileStorage::READ);
	FileStorage trainfs(trainFile, FileStorage::READ);

	trainfs["CuboidNumber"] >> cuboidnumber;
	vector<vector<bool> > finaloutvec(cuboidnumber);
	for (auto i = 0; i < cuboidnumber; ++i){
		stringstream keyphrase;
		keyphrase << "cuboid" << i;
		trainfs[keyphrase.str()] >> train;
		testfs[keyphrase.str()] >> test;
		determinePatterns(train, test, finaloutvec[i], threshold);
	}
	if (flag != ""){
		Graphix(finaloutvec);
	}
	//-------------------------------------------------------------
	
	
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
////////////////SUPPORT FUNCTIONS////////////////////////////////////////

//simple repettive code for principal functions
void CrowdAnomalies::CommonLoadInfo(cutil_file_cont & file_list, string & directory,
string key, const char * token, vector<cutil_grig_point> & grid, short &rows, short  &cols)
{
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
//------------------------------------------------------------------------
//compare patterns
//
static double EuclideanDistance(Mat_<float>  & vec1, Mat_<float> & vec2)
{
	double dst = 0;
	for (auto i = 0; i < vec1.cols; ++i)
		dst += (vec2(0, i) - vec1(0, i)) * (vec2(0, i) - vec1(0, i)); 
	return sqrt(dst);
}

static void determinePatterns(Mat & train, Mat & test, vector<bool> & out, float thr)
{
	out.clear();
	out.resize(test.rows);
	for (auto i = 0; i < test.rows; ++i)
	{
		out[i] = false;
		Mat_<float> pattern = test.row(i);
		for (int j = 0; j < train.rows; ++j)
		{
			Mat_<float> trainPat = train.row(j);
			if (EuclideanDistance(trainPat, pattern) < thr){
				out[i] = true;
				break;
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//==========================================================================
//draw rectangles
void ShowAnomaly(cv::Mat & frm, int pos, std::vector<std::vector<bool> > & an, 
				 std::vector<cutil_grig_point> & grid)
{
	for (std::size_t i = 0; i < an.size(); ++i)
	{
		cv::Scalar color = an[i][pos] ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
		cv::rectangle(	frm,
					cv::Point(grid[i].yi, grid[i].xi),
					cv::Point(grid[i].yf, grid[i].xf), 
					color );
	}
}
//
void CrowdAnomalies::Graphix( vector<vector<bool> > &rpta )
{	
	string	directory,
			file_extension,
			directory_out,
			token_out;
	short	rows,
			cols;
	cutil_file_cont				file_list;
	vector<cutil_grig_point>	grid;

	FileStorage fs(_mainfile, FileStorage::READ);
	fs["support_graphix_ext"] >> file_extension;
	fs["support_graphix_dir"] >> directory;
	fs["support_graphix_out_dir"] >> directory_out;
	fs["support_graphix_out_token"] >> token_out;

	//=====================================================================
	CommonLoadInfo(file_list, directory, "", file_extension.c_str(), grid, rows, cols);
	int step = _main_frame_interval * _main_frame_range;
	for (size_t i = step, pos = 0; i < file_list.size() && 
							       pos < rpta[0].size(); i+= step, ++pos)
	{
		stringstream strout;
		strout << directory_out << "/" << token_out << pos << "." <<file_extension;
		Mat img = imread(file_list[i]);
		ShowAnomaly(img, pos, rpta, grid);
		imwrite(strout.str(), img);
	}
}