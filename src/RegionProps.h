#pragma once

#include <opencv2/opencv.hpp>
#include <iostream>

#include "Region.h"

class RegionProps
{
public:
	//RegionProps(const std::vector<cv::Point> &_contour);
		//const cv::Mat &_img);

	inline Region getRegion() const
	{
		return region;
	}

	void getContour(std::vector<cv::Point>& _contour);
	void compute();
	cv::RotatedRect ellipse();
	double orientation();
	double majoraxislength();
	double minoraxislength();
	double deltaaxislength();
	//cv::Mat filledimage();
	//double filledarea();
	//std::vector<cv::Point> extrema();

	Region region;
	std::vector<cv::Point> contour;
	cv::Mat img;

};

// https://github.com/apennisi/regionprops

