#pragma once

#include <opencv2/core/core.hpp>  

template<typename _Tp> inline bool
operator <= (const cv::Rect_<_Tp>& r1, const cv::Rect_<_Tp>& r2)
{
	return (r1 & r2) == r1;
}

int UseCamera();
int UseVideo();
int UseVideo_MutipleThread();
int UseCamera_MutipleThread();
int test();