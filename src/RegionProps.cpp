#include "RegionProps.h"

void RegionProps::compute()
{
	region.setEllipse(ellipse());
	region.setMajorAxis(majoraxislength());
	region.setMinorAxis(minoraxislength());
	region.setOrientation(orientation());
	region.setDeltaAxis();
}
void RegionProps::getContour(std::vector<cv::Point>& _contour)
{
	contour.swap(_contour);
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