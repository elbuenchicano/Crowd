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
		_main_cuboid_over_height;	//cuboids height overlap
	void	Extract_Feat();
	void	Precompute_OF();
	void	Train();
public:
	CrowdAnomalies(string featf);
	~CrowdAnomalies(){}
	void Execute();
};
CrowdAnomalies::CrowdAnomalies(string featf){
	_mainfile = featf;
	_fs = FileStorage(featf, FileStorage::READ); 
	_fs["main_frame_interval"]	>> _main_frame_interval;
	_fs["main_frame_range"]		>> _main_frame_range;
	_fs["main_cuboid_width"]	>> _main_cuboid_width;
	_fs["main_cuboid_height"]	>> _main_cuboid_height;
	_fs["main_cuboid_over_width"]	>> _main_cuboid_over_width;
	_fs["main_cuboid_over_height"]	>> _main_cuboid_over_height;
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
			Train();
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
//Train usin precomputed optical flow images........................
//requeriments in the configuration file............................
//main_train_dir = directory................................
void CrowdAnomalies::Train()
{
	string		directory,
				file_out;
	short		rows, 
				cols,
				descriptorType;
	Mat			img;
	cutil_file_cont		file_list;
	//-------------------------------------------------------------
	//load info....................................................
	_fs["main_train_dir"] >> directory;
	_fs["main_descriptor_type"] >> descriptorType;
	_fs["main_train_file_output"] >> file_out;
	//-------------------------------------------------------------
	list_files_all(file_list, directory.c_str(), "of.yml");
	FileStorage imgfs(file_list.front(), FileStorage::READ);
	imgfs["angle"] >> img;
	rows = img.rows;
	cols = img.cols;
	//-------------------------------------------------------------
	//grid generator may have overlaping
	vector<cutil_grig_point> grid = grid_generator(rows, cols,
												_main_cuboid_width,		 _main_cuboid_height,
												_main_cuboid_over_width, _main_cuboid_over_height);
	//-------------------------------------------------------------
	//-------------------------------------------------------------
	OFBasedDescriptorBase<OFBased_Trait>* descrip = selectChildDes<OFBased_Trait>(descriptorType, _mainfile);
	DesOutData		vecOutput(grid.size());
	for (size_t i = _main_frame_range-1; i < file_list.size(); i += _main_frame_range)
	{
		DesvecParMat	temporalset(_main_frame_range);
		DesInData		input;
		for (int j = 0; j < _main_frame_range; ++j)
		{
			imgfs.open(file_list[i-j], FileStorage::READ);
			imgfs["angle"] >> temporalset[_main_frame_range-j-1].first;
			imgfs["magnitude"] >> temporalset[_main_frame_range-j-1].second;
		}
		input.first	 = temporalset;
		input.second = grid;
		descrip->Describe(input, vecOutput);
	}
	vectorMat2YML< Mat_<float> >(vecOutput, file_out);
	imgfs.release();/**/
	delete descrip;
}

