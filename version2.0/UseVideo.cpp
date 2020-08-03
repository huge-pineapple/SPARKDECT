#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp> 
#include <opencv2/imgproc.hpp>

#include "conio.h"
#include "Region.h"
#include "RegionProps.h"
#include "UseMode.h"

static const int BinaryThresh = 170;
static const int AreaThresh = 50;
static const int AngleThresh = 100;
static const int DeltaAxisThresh = 100;
static const double eps = 1e-6;

static bool descendSort(std::vector<cv::Point>& a, std::vector<cv::Point>& b)
{
	return a.size() > b.size();
}

int UseVideo()
{	
	RegionProps Props;
	cv::VideoCapture cap;
	cap.open("avA1900-50gm_6-17-17-3-50_First.mp4");

	std::cout << "开始初始化背景模型" << std::endl;
	cv::Mat BgModel = cv::imread("mask.jpg", 0);
	cv::Mat Mask = cv::Mat::zeros(BgModel.size(), CV_8UC1);
	cv::Rect2f r1(340, 620, 590, 280);
	Mask(r1).setTo(255);
	std::vector<std::vector<cv::Point>> BgContours;
	std::vector<cv::Vec4i> BgHierarchy;
	cv::findContours(BgModel, BgContours, BgHierarchy, 0, 1);
	std::vector<std::vector<cv::Point>>::iterator BgIt = BgContours.begin();
	for (; BgIt != BgContours.end();)
	{
		if (BgIt->size() < 5)
		{
			BgIt = BgContours.erase(BgIt);
			continue;
		}
		BgIt++;
	}
	std::vector<cv::Rect2f> BgRRects(BgContours.size());
	for (int i = 0; i < BgContours.size(); i++)
	{
		BgRRects[i] = cv::fitEllipse(BgContours[i]).boundingRect2f();
	}
	std::cout << "初始化背景模型成功" << std::endl;

	cv::Mat frame;
	cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
	std::vector<std::vector<cv::Point> > allContours;
	std::vector<cv::Vec4i> hierarchy;
	cv::namedWindow("video", 0);
	cv::namedWindow("bgmodel", 0);
	cv::namedWindow("closed", 0);

	// Flags
	int Count = 0;
	int BgCount = 0;
	int SparkRegion = 0;
	bool UpdateFlag = false;
	bool SaveFlag = false;
	bool BgFlag = false;
	bool StripFlag = false;
	bool InMaskFlag = false;
	while (1)
	{
		if (_kbhit())
		{
			char ch = _getch();
			if (ch == 27 || ch == 'q')
			{
				break;//按键为esc或q 时退出
			}
		}
		cap.read(frame);
		if (frame.empty())
		{
			break;
		}
		
		Count++;
		auto t1 = std::chrono::high_resolution_clock::now();
		cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
		cv::Mat ImgThresh, ImgClosed;
		threshold(frame, ImgThresh, BinaryThresh, 255, 0);
		cv::morphologyEx(ImgThresh, ImgClosed, cv::MORPH_CLOSE, closeKernel, cv::Point(-1, -1), 1);
		cv::imshow("video", frame);
		cv::imshow("closed", ImgClosed);

		if (BgCount == 100)
		{
			std::cout << "开始更新背景模型" << std::endl;
			BgModel = cv::Mat::zeros(ImgClosed.size(), CV_8UC1);
			UpdateFlag = true;
		}
		if (UpdateFlag)
		{
			cv::add(ImgClosed, BgModel, BgModel, Mask, -1);
			--BgCount;
			--BgCount;
			if (BgCount == 0)
			{
				std::vector<std::vector<cv::Point>> BgContours;
				std::vector<cv::Vec4i> BgHierarchy;
				cv::findContours(BgModel, BgContours, BgHierarchy, 0, 1);
				std::vector<std::vector<cv::Point>>::iterator BgIt = BgContours.begin();
				for (; BgIt != BgContours.end();)
				{
					if (BgIt->size() < 5)
					{
						BgIt = BgContours.erase(BgIt);
						continue;
					}
					BgIt++;
				}
				BgRRects.reserve(BgContours.size());
				for (int i = 0; i < BgContours.size(); i++)
				{
					BgRRects[i] = cv::fitEllipse(BgContours[i]).boundingRect2f();
				}
				std::cout << "背景模型更新结束" << std::endl;
				UpdateFlag = false;
			}
			continue;
		}

		cv::findContours(ImgClosed, allContours, hierarchy, 0, 1);

		cv::imshow("bgmodel", BgModel);
		cv::waitKey(1);

		std::cout << "frame count: " << Count << std::endl;

		SparkRegion = 0;
		SaveFlag = false;
		if (allContours.empty())
		{
			std::cout << "No Contours, No Spark" << std::endl;
			BgCount = 0;
		}
		else
		{
			BgFlag = false;
			std::sort(allContours.begin(), allContours.end(), descendSort);
			std::vector<double> RegionAreas(0);
			if (allContours[0].size() > 5)
			{
				RegionAreas.reserve(allContours.size());
				for (int i = 0; i < allContours.size(); i++)
				{
					if (allContours[i].size() <= 5)
					{
						break;
					}
					StripFlag = false;
					InMaskFlag = false;
					Props.getContour(allContours[i]);
					Props.compute();
					if (Props.region.Area() < AreaThresh - eps)
					{
						break;
					}
					//std::cout << "Area: " << Props.region.Area() << std::endl;
					//std::cout << "Angle: " << Props.region.Orientation() << std::endl;
					//std::cout << "DeltaAxis: " << Props.region.DeltaAxis() << std::endl;
					if (Props.region.Ellipse().boundingRect2f() <= r1)
					{
						InMaskFlag = true;
					}
					if (Props.JudgeAngle() || Props.region.DeltaAxis() > DeltaAxisThresh - eps)
					{
						StripFlag = true;
					}
					if (StripFlag && InMaskFlag)
					{
						BgFlag = true;
					}
					if (!StripFlag && !InMaskFlag)
					{
						++SparkRegion;
						RegionAreas.push_back(Props.region.Area());
					}
					if (!StripFlag && InMaskFlag)
					{
						float InterArea = 0;
						for (auto& BgRegion : BgRRects)
						{
							if (!(BgRegion & Props.region.Ellipse().boundingRect2f()).empty())
							{
								auto InterRect = BgRegion & Props.region.Ellipse().boundingRect2f();
								InterArea = InterRect.area();
								break;
							}
						}
						std::cout << "InterArea: " << InterArea << "   Area:  " << Props.region.Area() << std::endl;
						if (InterArea / Props.region.Area() < 0.9)
						{
							++SparkRegion;
							RegionAreas.push_back(Props.region.Area());
							std::cout << "InterRate: " << InterArea/Props.region.Area()  << std::endl;
						}
					}
				}
				if (BgFlag)
				{
					++BgCount;
				}
			}
			auto MaxArea = std::max_element(std::begin(RegionAreas), std::end(RegionAreas));
			if (SparkRegion > 0)
			{
				std::cout << "有火花" << std::endl;
				std::cout << "最大的火花面积" << *MaxArea << std::endl;
				BgCount = 0;
				SaveFlag = true;
			}
			else
			{
				std::cout << "no spark" << std::endl;
			}
			auto t2 = std::chrono::high_resolution_clock::now();
			double dr_ms1 = std::chrono::duration<double, std::milli>(t2 - t1).count();
			std::cout << "处理一帧用时:  " << dr_ms1 << " ms " << std::endl;
		}
	}
	return 0;
}