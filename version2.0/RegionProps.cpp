#include "RegionProps.h"

void RegionProps::compute()
{
	region.setEllipse(ellipse());
	region.setMajorAxis(majoraxislength());
	region.setMinorAxis(minoraxislength());
	region.setOrientation(orientation());
	region.setDeltaAxis();
	region.setArea(area());
}
void RegionProps::getContour(std::vector<cv::Point>& _contour)
{
	contour.assign(_contour.begin(), _contour.end());
}
cv::RotatedRect RegionProps::ellipse()
{
	return cv::fitEllipse(contour);
}

double RegionProps::majoraxislength()
{
	return cv::max(region.Ellipse().size.width, region.Ellipse().size.height);
}

double RegionProps::minoraxislength()
{
	return cv::min(region.Ellipse().size.width, region.Ellipse().size.height);
}

double RegionProps::orientation()
{
	return region.Ellipse().angle;
}

double RegionProps::deltaaxislength()
{
	return region.DeltaAxis();
}
double RegionProps::area()
{
	return cv::contourArea(contour);
}
bool RegionProps::JudgeAngle()
{
	if (region.Orientation() >= 155 - eps && region.Orientation() <= 180 + eps)
	{
		return true;
	}
	if ((region.Orientation() >= 350 - eps && region.Orientation() <= 360 + eps) || (region.Orientation() >= 0 - eps && region.Orientation() <= 12 + eps))
	{
		return true;
	}
	return false;
}
/*cv::Mat RegionProps::filledimage()
{
	cv::Mat filled = cv::Mat::zeros(img.size(), CV_8UC1);
	cv::drawContours(filled, std::vector< std::vector<cv::Point> >(1, contour), -1, cv::Scalar(255), -1);
	return filled;
}

double RegionProps::filledarea()
{
	return cv::countNonZero(region.FilledImage());
}*/
// https://github.com/apennisi/regionprops