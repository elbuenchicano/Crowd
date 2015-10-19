#ifndef HDESCRIPTOR2D_H
#define HDESCRIPTOR2D_H
#include "CUtil.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include <type_traits>
#include <tuple>

struct TextureBasedDescriptor
{
	TextureBasedDescriptor(){}
	virtual void setData(std::string file){}
	virtual void Describe(void *, void *) = 0;
};

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//////////////////////////LBP////////////////////////////////////////
cv::Mat_<int> LBP(cv::Mat src_image);
//==================================================================
//trait for MO descriptor
struct Trait_LBP
{
	typedef std::tuple<cv::Point, cv::Point, cv::Mat>	DesInData;
	typedef cv::Mat_<int>								HistoType;
	typedef std::vector<HistoType>						DesOutData;//output data type
};
//==================================================================
//descriptor magnitude orientation  

template <class tr>
struct TextureBasedLBP : public TextureBasedDescriptor
{
	typedef typename  tr::DesInData		DesInData;
	typedef typename  tr::DesOutData	DesOutData;
	typedef typename  tr::HistoType		HistoType;

	//______________________________________________________________
	virtual void Describe(void * invoid, void *outvoid)
	{
		
	}
};


cv::Mat_<int> LBP(cv::Mat src_image)
{
    bool affiche=true;
	//cv::Mat MyImg(src_image.rows, src_image.cols, CV_8UC1);
    cv::Mat	Image;
    cv::Mat_<int> lbp(src_image.rows, src_image.cols);
	cv::Mat_<int> histogram(1, 256);
	histogram = histogram * 0;

	if (src_image.channels() == 3)
        cv::cvtColor(src_image, Image, CV_BGR2GRAY);
 
    unsigned center = 0;
    unsigned center_lbp = 0;
 
    for (int row = 1; row < Image.rows-1; row++)
    {
        for (int col = 1; col < Image.cols-1; col++)
        {
            center = Image.at<uchar>(row, col);
            center_lbp = 0;
 
            if (center <= Image.at<uchar>(row - 1, col - 1))
                center_lbp += 1;
 
            if (center <= Image.at<uchar>(row - 1, col))
                center_lbp += 2;
 
            if (center <= Image.at<uchar>(row - 1, col + 1))
                center_lbp += 4;
 
            if (center <= Image.at<uchar>(row, col - 1))
                center_lbp += 8;
 
            if (center <= Image.at<uchar>(row, col + 1))
                center_lbp += 16;
 
            if (center <= Image.at<uchar>(row + 1, col - 1))
                center_lbp += 32;
 
            if (center <= Image.at<uchar>(row + 1, col))
                center_lbp += 64;
 
            if (center <= Image.at<uchar>(row + 1, col + 1))
                center_lbp += 128;
            lbp(row, col) = center_lbp;
			++histogram(0, center_lbp);
			//MyImg.at<uchar>(row, col) = center_lbp;
        }
    }

/* if(affiche == true)
            {
              cv::imshow("image LBP", MyImg);
              cv::waitKey();
              cv::imshow("grayscale",Image);
              cv::waitKey();
            }
 
            else
            {
              cv::destroyWindow("image LBP");
              cv::destroyWindow("grayscale");
            }
 */
    return histogram;
}

#endif