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

static void determinePatterns(Mat & train, Mat & test, vector<bool> & out);
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

	//SUPPORT FUNCTIONS.................................................
	void	CommonLoadInfo(cutil_file_cont &, string &, char *, 
						   vector<cutil_grig_point> &, short &, short &);
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
	for (auto & fil : file_list)
	{
		Mat	img	= imread(fil);
		image_vector.push_back(img);
	}
	oflow->compute(image_vector, of_out);
	for (size_t i = 0; i < of_out.size(); i += _main_frame_interval)
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
	CommonLoadInfo(file_list, directory, "of.yml", grid, rows, cols);

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
	CommonLoadInfo(file_list, directory, "of.yml", grid, rows, cols);

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
				file_out;
	Mat			train,
				test;
	short		cuboidnumber;
	vector<Mat_<float> >	info_out;
	
	//-------------------------------------------------------------
	//load info....................................................
	FileStorage info(_mainfile, FileStorage::READ);
	info["main_test_of_test_file"] >> testFile;
	info["main_test_of_train_file"] >> trainFile;
	info["main_test_of_output"] >> file_out;
	
	//-------------------------------------------------------------
	FileStorage testfs(testFile, FileStorage::READ);
	FileStorage trainfs(trainFile, FileStorage::READ);

	trainfs["CuboidNumber"] >> cuboidnumber;
	for (auto i = 0; i < cuboidnumber; ++i){
		stringstream keyphrase;
		keyphrase << "cuboid" << i;
		trainfs[keyphrase.str()] >> train;
		testfs[keyphrase.str()] >> test;
		
	}

	//-------------------------------------------------------------
	
	
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
////////////////SUPPORT FUNCTIONS////////////////////////////////////////

//simple repettive code for principal functions
void CrowdAnomalies::CommonLoadInfo(cutil_file_cont & file_list, string & directory,
		   char * token, vector<cutil_grig_point> & grid, short &rows, short  &cols)
{
	Mat			img;
	list_files_all(file_list, directory.c_str(), token);
	FileStorage imgfs(file_list.front(), FileStorage::READ);
	imgfs["angle"] >> img;
	rows = img.rows;
	cols = img.cols;
	grid = grid_generator(rows, cols,
		_main_cuboid_width, _main_cuboid_height,
		_main_cuboid_over_width, _main_cuboid_over_height);
}
//------------------------------------------------------------------------
//compare patterns

static bool comparePatterns(Mat & train, Mat & test)
{
	for (auto i = 0; i < train.cols; ++i)
	{
		
	}
}

static void determinePatterns(Mat & train, Mat & test, vector<bool> & out)
{
	for (auto i = 0; i < test.rows; ++i)
	{

	}
}
