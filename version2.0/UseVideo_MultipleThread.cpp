#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp> 
#include <opencv2/imgproc.hpp>

#include "conio.h"
#include "Region.h"
#include "RegionProps.h"
#include "UseMode.h"
#include "ThreadPool.h"

static const int BinaryThresh = 170;
static const int AreaThresh = 50;
static const int AngleThresh = 100;
static const int DeltaAxisThresh = 100;
static const double eps = 1e-6;

static std::mutex mtx;
static std::condition_variable cond;
static std::condition_variable condLoop;

static bool descendSort(std::vector<cv::Point>& a, std::vector<cv::Point>& b)
{
	return a.size() > b.size();
}

static void ShowLoop(cv::Mat& _frame, cv::Mat& _ImgClosed, cv::Mat& _BgModel, bool& _EndFlag)
{
	cv::namedWindow("video", 0);
	cv::namedWindow("bgmodel", 0);
	cv::namedWindow("closed", 0);

	while (!_EndFlag)
	{
		std::unique_lock<std::mutex> lock(mtx);
		condLoop.wait(lock);
		if (_frame.empty())
			break;
		cv::imshow("video", _frame);
		cv::imshow("closed", _ImgClosed);
		cv::imshow("bgmodel", _BgModel);
		cv::waitKey(1);
	}
}

int UseVideo_MutipleThread()
{
	RegionProps Props;
	ThreadPool Pool(10);
	std::promise<bool> result_pro;
	result_pro.set_value(false);
	std::future<bool> result = result_pro.get_future();
	std::atomic<bool> UpdateFlag = false;

	cv::Mat BgModel = cv::imread("mask.jpg", 0);
	cv::Mat BgImgs[100]{ cv::Mat(BgModel.size(), CV_8UC1) };
	std::cout << "开始初始化背景模型" << std::endl;
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
	std::vector<cv::Rect2f> tempRRects;
	for (int i = 0; i < BgContours.size(); i++)
	{
		BgRRects[i] = cv::fitEllipse(BgContours[i]).boundingRect2f();
	}
	std::cout << "初始化背景模型成功" << std::endl;

	cv::VideoCapture cap;
	cap.open("avA1900-50gm_6-17-17-3-50_First.mp4");

	cv::Mat frame;
	cv::Mat ImgThresh, ImgClosed;
	cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
	std::vector<std::vector<cv::Point> > allContours;
	std::vector<cv::Vec4i> hierarchy;
	//cv::namedWindow("video", 0);
	//cv::namedWindow("bgmodel", 0);
	//cv::namedWindow("closed", 0);

	// Flags
	int Count = 0;
	int BgCount = 0;
	int SparkRegion = 0;
	bool SaveFlag = false;
	bool BgFlag = false;
	bool StripFlag = false;
	bool InMaskFlag = false;
	bool EndFlag = false;

	Pool.enqueue(ShowLoop, std::ref(frame), std::ref(ImgClosed), std::ref(BgModel), std::ref(EndFlag));
	Pool.enqueue([&] {
		while (true)
		{
			std::unique_lock<std::mutex> lock(mtx);
			//while (!UpdateFlag)
			cond.wait(lock, [&] {return UpdateFlag == true || EndFlag; });
			auto t5 = std::chrono::high_resolution_clock::now();
			UpdateFlag = false;
			if (EndFlag)
				break;
			tempRRects.resize(BgContours.size());
			for (int i = 0; i < tempRRects.size(); i++)
			{
				tempRRects[i] = cv::fitEllipse(BgContours[i]).boundingRect2f();
			}
			BgRRects = tempRRects;
			auto t6 = std::chrono::high_resolution_clock::now();
			double dr_ms3 = std::chrono::duration<double, std::milli>(t6 - t5).count();
			std::cout << "背景Bounding box更新完成，用时：" << dr_ms3 << "ms" << std::endl;
			std::cout << "线程ID : " << std::this_thread::get_id() << std::endl;
		}
	});
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

		//mtx.lock();
		cap.read(frame);
		if (frame.empty())
		{
			//mtx.unlock();
			break;
		}

		auto t1 = std::chrono::high_resolution_clock::now();
		Count++;

		cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
		threshold(frame, ImgThresh, BinaryThresh, 255, 0);
		cv::morphologyEx(ImgThresh, ImgClosed, cv::MORPH_CLOSE, closeKernel, cv::Point(-1, -1), 1);
		cv::findContours(ImgClosed, allContours, hierarchy, 0, 1);
		//mtx.unlock();
		condLoop.notify_one();
		//std::cout << "frame count: " << Count << std::endl;

		SparkRegion = 0;
		SaveFlag = false;
		BgFlag = false;
		if (allContours.empty())
		{
			//std::cout << "No Contours, No Spark" << std::endl;
		}
		else
		{
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
						//std::cout << "InterArea: " << InterArea << "   Area:  " << Props.region.Area() << std::endl;
						if (InterArea / Props.region.Area() < 0.9)
						{
							++SparkRegion;
							RegionAreas.push_back(Props.region.Area());
							//std::cout << "InterRate: " << InterArea / Props.region.Area() << std::endl;
						}
					}
				}
			}
			auto MaxArea = std::max_element(std::begin(RegionAreas), std::end(RegionAreas));
			if (SparkRegion > 0)
			{
				//std::cout << "有火花" << std::endl;
				//std::cout << "最大的火花面积" << *MaxArea << std::endl;
				SaveFlag = true;
				BgFlag = false;
			}
			else
			{
				//std::cout << "no spark" << std::endl;
			}
		}
		auto t2 = std::chrono::high_resolution_clock::now();
		double dr_ms1 = std::chrono::duration<double, std::milli>(t2 - t1).count();
		std::cout << "处理一帧用时:  " << dr_ms1 << " ms " << std::endl;
		//auto t1 = std::chrono::high_resolution_clock::now();
		try
		{
			if (BgFlag)
			{
				BgImgs[BgCount] = ImgClosed;
				std::cout << "BgCount: " << BgCount << std::endl;
				++BgCount;
				if (BgCount == 100)
				{
					BgCount = 0;
					result = Pool.enqueue([&] {
						auto t3 = std::chrono::high_resolution_clock::now();
						std::cout << "开始更新背景模型" << std::endl;
						BgModel = cv::Mat::zeros(ImgClosed.size(), CV_8UC1);
						for (auto& BgImg : BgImgs)
						{
							cv::add(BgImg, BgModel, BgModel, Mask, -1);
						}
						//std::vector<std::vector<cv::Point>> BgContours;
						//std::vector<cv::Vec4i> BgHierarchy;
						cv::findContours(BgModel, BgContours, BgHierarchy, 0, 1);
						//std::vector<std::vector<cv::Point>>::iterator BgIt = BgContours.begin();
						BgIt = BgContours.begin();
						for (; BgIt != BgContours.end();)
						{
							if (BgIt->size() < 5)
							{
								BgIt = BgContours.erase(BgIt);
								continue;
							}
							BgIt++;
						}
						std::cout << "背景模型更新结束" << std::endl;
						auto t4 = std::chrono::high_resolution_clock::now();
						double dr_ms2 = std::chrono::duration<double, std::milli>(t4 - t3).count();
						std::cout << "背景模型用时:  " << dr_ms2 << " ms " << std::endl;
						UpdateFlag = true;
						cond.notify_one();
						return true; });
				}
			}
			else
			{
				BgCount = 0;
			}
			//cv::imshow("video", frame);
			//cv::imshow("closed", ImgClosed);
			//cv::imshow("bgmodel", BgModel);
			//cv::waitKey(1);
		}
		catch (const std::exception& e)
		{
			std::cerr << "An exception occurred." << std::endl
				<< e.what() << std::endl;
		}
		catch (...)
		{
			std::cerr << "Unknown occurred" << std::endl;
		}
	}
	EndFlag = true;
	condLoop.notify_all();
	cond.notify_all();
	return 0;
}