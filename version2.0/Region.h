#pragma once

#include <opencv2/opencv.hpp>
#include <iostream>

class Region
{
public:
	Region() {};
	inline double Area() const
	{
		return area;
	}
	inline void setArea(const double& _area)
	{
		area = _area;
	}
	inline cv::RotatedRect Ellipse() const
	{
		return ellipse;
	}
	inline void setEllipse(const cv::RotatedRect& _ellipse)
	{
		ellipse = _ellipse;
	}
	inline double Orientation() const
	{
		return orientation;
	}
	inline void setOrientation(const double& _orientation)
	{
		orientation = _orientation;
	}
	inline double MinorAxis() const
	{
		return minoraxis_length;
	}
	inline void setMinorAxis(const double& minor_axis)
	{
		minoraxis_length = minor_axis;
	}
	inline double MajorAxis() const
	{
		return majoraxis_length;
	}
	inline void setMajorAxis(const double& major_axis)
	{
		majoraxis_length = major_axis;
	}
	inline void setDeltaAxis()
	{
		delta_length = majoraxis_length - minoraxis_length;
	}
	inline double DeltaAxis() const
	{
		return delta_length;
	}
	/*inline void setFilledArea(const double &_filledArea)
	{
		filledArea = _filledArea;
	}
	inline double FilledArea() const
	{
		return filledArea;
	}
	inline void setFilledImage(const cv::Mat &_filledImage)
	{
		filledImage = _filledImage;
	}
	inline cv::Mat FilledImage() const
	{
		return filledImage;
	}*/
private:
	double area;
	double perimeter;
	//cv::Moments moments;
	//cv::Point centroid;
	//cv::Rect boundingBox;
	//double aspect_ratio, equi_diameter, extent;
	//std::vector< cv::Point> convex_hull;
	//double convex_area, solidity;
	//cv::Point center;
	double majoraxis_length, minoraxis_length;
	double orientation;
	//double eccentricity;
	cv::Mat filledImage;
	//cv::Mat pixelList;
	//double filledArea;
	//cv::Mat convexImage;
	cv::RotatedRect ellipse;
	double delta_length;
	//std::vector<cv::Point> approx;
	//double maxval, minval;
	//cv::Point maxloc, minloc;
	//cv::Scalar meanval;
	//std::vector<cv::Point> extrema;

};

// https://github.com/apennisi/regionprops

