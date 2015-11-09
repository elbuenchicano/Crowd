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


using namespace std;
using namespace cv;

static const int num_threads = 10;
void determinePatterns(Mat & train, Mat & test, vector<bool> & out, float thr, int type);
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
	double		_scale;

	//MAIN FUNCTIONS....................................................
	void	Precompute_OF();
	void	Precompute_Gabor();
	void	Feat_Extract();
	void	Test_Offline();
	void	GTValidation(vector<vector<bool> > &);

	//SUPPORT FUNCTIONS.................................................
	void	Feat_Extract_OM();
	void	DescribeSeq		( DirectoryNode &, vector<cutil_grig_point> &, Mat &,
							  string &, string & );
	void	Feat_Extract_Gabor();
	
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
////////////////MAIN    FUNCTIONS////////////////////////////////////////

CrowdAnomalies::CrowdAnomalies(string featf){
	_mainfile = featf;
	cout << _mainfile << endl;
	_fs = FileStorage(featf, FileStorage::READ);
	_fs["main_frame_interval"]		>> _main_frame_interval;
	_fs["main_frame_range"]			>> _main_frame_range;
	_fs["main_cuboid_width"]		>> _main_cuboid_width;
	_fs["main_cuboid_height"]		>> _main_cuboid_height;
	_fs["main_cuboid_over_width"]	>> _main_cuboid_over_width;
	_fs["main_cuboid_over_height"]	>> _main_cuboid_over_height;
	_fs["main_descriptor_type"]		>> _main_descriptor_type;
	_fs["main_image_scale"]			>> _scale;
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
void CrowdAnomalies::Precompute_OF()
{
	cout << "Precompute_oF" << endl;
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
	cutil_create_new_dir_all(directory_out);
	//doblar el tamaño
	
	for (size_t i = 0; i < file_list.size(); ++i)
	{
		cout << file_list[i] << endl;
		Mat	img	= imread(file_list[i]);
		if (_scale > 0)
			resize(img, img, Size(), _scale, _scale, INTER_CUBIC);
		image_vector.push_back(img);
	}
	oflow->compute(image_vector, of_out);
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
	int		orientation,
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
	switch (_main_descriptor_type)
	{
	case 1:{
		Feat_Extract_OM();
		break;
	}
	case 2:{
	   Feat_Extract_Gabor();
	   break;
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
	Mat			train,
				test;
	short		cuboidnumber, 
				distancetype;
	float		threshold;
	vector<Mat_<float> >	info_out;
	
	
	//-------------------------------------------------------------
	//load info....................................................
	_fs["main_test_of_test_file"]	>> testFile;
	_fs["main_test_of_train_file"]	>> trainFile;
	_fs["main_test_of_threshold"]	>> threshold;
	_fs["main_test_of_distance_type"]	>> distancetype;
	_fs["main_test_of_graphix_flag"]	>> flagGraphix;
	_fs["main_test_of_gtvalidate_flag"]	>> flagGtval;

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
		trainfs[keyphrase.str()] >> train;
		testfs[keyphrase.str()] >> test;
		determinePatterns(train, test, finaloutvec[i], threshold, distancetype);
		cout << keyphrase.str() << endl;
		//thread increment________________________________________
		/*t[i % num_threads] = thread(determinePatterns, ref(train), ref(test), ref(finaloutvec[i]), threshold, i);
		if ( (i + 1) % num_threads == 0)
			for (auto thr = 0; thr < num_threads; ++thr)
				t[thr].join();
		//________________________________________________________/**/
	}
	if (flagGtval == "true"){
		GTValidation(finaloutvec);
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
////////////////////////////////////////////////////////////////////////////////
//result : 
//0 = tp, 
//1 = tn, 
//2 = fp, 
//3 = fn;
static inline int	Validate_FrmLvl(Mat_<int> &img, vector<cutil_grig_point> &grid, vector<vector<bool>> &resGrid, int pos)
{
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
	/*//if ( gt_anomaly &&  res_anomaly &&  both)	return 0;
	if (!gt_anomaly && !res_anomaly && !both)	return 2;
	if ( gt_anomaly && !both)					return 1;
	if (!gt_anomaly && res_anomaly)				return 3;*/

	if ( !gt_anomaly && !res_anomaly )	return 1;

	if ( !gt_anomaly &&  res_anomaly )	return 2;

	if ( gt_anomaly  && !res_anomaly )	return 3;/**/

	return 0;
}
void CrowdAnomalies::GTValidation(vector<vector<bool> > & rpta){
	string	directory,
			token,
			out_file;
	short	rows, 
			cols;
	//-------------------------------------------------------------------

	cutil_file_cont				file_list;
	vector<cutil_grig_point>	grid;	
	//-------------------------------------------------------------------
	//load info..........................................................

	_fs["main_gtvalidation_dir"]	>> directory;
	_fs["main_gtvalidation_token"]	>> token;
	_fs["main_gtvalidation_out_file"]	>> out_file;
	
	Mat			img0;
	list_files_all(file_list, directory.c_str(), token.c_str());
	img0 =  imread(file_list.front());
	if (_scale > 0)
			resize(img0, img0, Size(), _scale, _scale, INTER_CUBIC);
	rows = img0.rows;
	cols = img0.cols;
	grid = grid_generator(rows, cols,
		_main_cuboid_width, _main_cuboid_height,
		_main_cuboid_over_width, _main_cuboid_over_height);

	string	ant = cutil_antecessor(out_file, 1);
	cutil_create_new_dir_all(ant);
	
	ofstream outFrame	(out_file.c_str(), ios::app);

	//-------------------------------------------------------------------

	int step = _main_frame_interval * _main_frame_range;
	Metric_units munit;
	for (size_t i = step-2, pos = 0; (i < file_list.size()) && 
							         (pos < rpta[0].size()); i+= step, ++pos)
	{
		//cout << i << " " << pos << endl;
		Mat img  = imread(file_list[i], CV_BGR2GRAY);
		if (_scale > 0)
			resize(img, img, Size(), _scale, _scale, INTER_CUBIC);
		Mat_<int> gt = img;
		int mtype	= Validate_FrmLvl(gt, grid, rpta, pos);
		++munit[mtype];
	}
	double	FPR = supp_FalsePositiveRate(munit),
			TPR = supp_Recall(munit);
	outFrame << FPR	<< " ";
	outFrame << TPR << endl;
	outFrame.close();
}
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
		cv::Scalar color = an[i][pos] ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
		cv::rectangle(	frm,
					cv::Point(grid[i].yi, grid[i].xi),
					cv::Point(grid[i].yf, grid[i].xf), 
					color );
	}
}
////////////////////////////////////////////////////////////////////////////////
//Drwaing anomalies function 
//Req: param : vector vector rptas (anommaly or not)
void CrowdAnomalies::Graphix( vector<vector<bool> > &rpta )
{	
	string	directory,
			file_extension,
			file_extension_out,
			directory_out,
			token_out;
	short	rows,
			cols;
	cutil_file_cont				file_list;
	vector<cutil_grig_point>	grid;

	_fs["support_graphix_ext"] >> file_extension;
	_fs["support_graphix_dir"] >> directory;
	_fs["support_graphix_out_dir"] >> directory_out;
	_fs["support_graphix_out_token"] >> token_out;
	_fs["support_graphix_out_ext"] >> file_extension_out;

	//=====================================================================

	cutil_create_new_dir_all(directory_out);

	Mat			img0;
	list_files_all(file_list, directory.c_str(), file_extension.c_str());
	img0 =  imread(file_list.front());
	if (_scale > 0)
			resize(img0, img0, Size(), _scale, _scale, INTER_CUBIC);
	rows = img0.rows;
	cols = img0.cols;
	grid = grid_generator(rows, cols,
		_main_cuboid_width, _main_cuboid_height,
		_main_cuboid_over_width, _main_cuboid_over_height);

	int step = _main_frame_interval * _main_frame_range;
	for (size_t i = step-2, pos = 0; i < file_list.size() && 
							       pos < rpta[0].size(); i+= step, ++pos)
	{
		stringstream strout;
		strout << directory_out << "/" << token_out << pos << "." <<file_extension_out;
		Mat img = imread(file_list[i]);
		if (_scale > 0)
			resize(img, img, Size(), _scale, _scale, INTER_CUBIC);
		ShowAnomaly(img, pos, rpta, grid);
		cout <<"write: "<< strout.str() << endl;
		imwrite(strout.str(), img);
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
	_fs["main_feat_extract_dir_gabor"]		>> directory_gabor;
	_fs["main_feat_extract_output"]			>> dir_out;
	_fs["main_feat_extract_token_of"]		>> token_of;
	_fs["main_feat_extract_token_gabor"]	>> token_gabor;
	_fs["main_feat_extract_token_out"]		>> token_out;
	_fs["descriptor_gaborNumBin"]			>> nscales;

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


/*

void ModEventRecognition::FigtreeDistance()
{
	vector<cv::Mat>	vtrain,
					vtest;
	int				rows,
					cols,
					szeMapSpace = 32;
	cv::Mat			calorMap;
	LoadFeaturesSimpleD(_trainFile, _testFile, vtrain, vtest, rows, cols);
	ComputeRareD(vtrain, vtest, rows, cols, szeMapSpace, ComputeDistaceSamples_Train_Figtree);
}


///Function to compute if feature vector is normal or abnormal
///train :  pattern train matrix
///sample : incoming pattern 
double ComputeDistaceSamples_Train_Figtree(cv::Mat & train, cv::Mat & sample)
{
	double	mindist = 1000;
	if (train.rows == 0)
		return 0;
	int d = sample.cols,
		N = train.rows,
		M = sample.rows,
		W = 1;

	double	h		= 100,
			epsilon = 1e-3;
	vector<double> g;
	
	cv::Mat q = cv::Mat::ones(cv::Size(1, M), CV_64F);
	double *qPtr = (double*)q.ptr();

	FigTree(d, N, M, W, train, h, qPtr, sample, epsilon, g);
	
	return mindist;
}


*/