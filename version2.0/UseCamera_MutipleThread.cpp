#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

#include <chrono>
#include <Windows.h>
#include <thread>

#include "conio.h"
#include "Region.h"
#include "RegionProps.h"
#include "Setting.h"
#include "ParamManager.h"
#include "UseMode.h"
#include "ThreadPool.h"

#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp> 
#include <opencv2/imgproc.hpp>

// Namespace for using pylon objects.
using namespace Pylon;

// Number of images to be grabbed.
//static const uint32_t c_countOfImagesToGrab = 10;
static const int64_t BytesThreshold = 10 * 1024 * 1024;
static const int BinaryThresh = 170;
static const int AreaThresh = 50;
//static const int AngleThresh = 80;
static const int DeltaAxisThresh = 100;
static const float IntersectThresh = 0.9;
static const double eps = 1e-6;

// Limits the amount of cameras used for grabbing.
// It is important to manage the available bandwidth when grabbing with multiple cameras.
// This applies, for instance, if two GigE cameras are connected to the same network adapter via a switch.
// To manage the bandwidth, the GevSCPD interpacket delay parameter and the GevSCFTD transmission delay
// parameter can be set for each GigE camera device.
// The "Controlling Packet Transmission Timing with the Interpacket and Frame Transmission Delays on Basler GigE Vision Cameras"
// Application Notes (AW000649xx000)
// provide more information about this topic.
// The bandwidth used by a FireWire camera device can be limited by adjusting the packet size.
static const size_t c_maxCamerasToUse = 2;

static bool ascendSort(std::vector<cv::Point>& a, std::vector<cv::Point>& b)
{
	return a.size() < b.size();
}
static bool descendSort(std::vector<cv::Point>& a, std::vector<cv::Point>& b)
{
	return a.size() > b.size();
}

// clean console
static int clrscr()
{

	HANDLE hndl = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hndl, &csbi);
	DWORD written;
	DWORD N = csbi.dwSize.X * csbi.dwCursorPosition.Y +
		csbi.dwCursorPosition.X + 1;
	COORD curhome = { 0,0 };

	FillConsoleOutputCharacter(hndl, ' ', N, curhome, &written);
	csbi.srWindow.Bottom -= csbi.srWindow.Top;
	csbi.srWindow.Top = 0;
	SetConsoleWindowInfo(hndl, TRUE, &csbi.srWindow);
	SetConsoleCursorPosition(hndl, curhome);

	return 0;
}

static std::mutex mtx;
static std::condition_variable cond;
static std::condition_variable condLoop;
static std::condition_variable condCreate;

int UseCamera_MutipleThread()
{
	// The exit code of the sample application.
	int exitCode = 0;
	bool StopFlag = false;

	ParamManager ParamMgr;
	Setting SetMgr;
	RegionProps Props;
	SYSTEMTIME sys;

	ThreadPool Pool(14);

	std::promise<bool> result_pro;
	result_pro.set_value(false);
	std::future<bool> result = result_pro.get_future();

	if (!SetMgr.CheckFreeSpace())
	{
		std::cout << "Reading disk is failed" << std::endl;
		return 1;
	}
	if (SetMgr.GetFreeGBytesAvailable() < 1 + eps)
	{
		std::cout << "The disk doesn't have enough space" << std::endl;
		return 1;
	}
	std::cout << "The disk has " << SetMgr.GetFreeGBytesAvailable() << " GB available space " << std::endl;
	std::cout << "Camera begins to work now " << std::endl;

	// Before using any pylon methods, the pylon runtime must be initialized. 
	PylonInitialize();

	try
	{
		// Check if CVideoWriter is supported and all DLLs are available.
		if (!CVideoWriter::IsSupported())
		{
			std::cout << "VideoWriter is not supported at the moment. Please install the pylon Supplementary Package for MPEG-4 which is available on the Basler website." << std::endl;
			// Releases all pylon resources. 
			PylonTerminate();
			// Return with error code 1.
			return 1;
		}


		// Get the transport layer factory.
		CTlFactory& tlFactory = CTlFactory::GetInstance();

		// Get all attached devices and exit application if no device is found.
		DeviceInfoList_t devices;
		//ITransportLayer* pTl = tlFactory.CreateTl("BaslerGigE"); // Only find GigE cameras
		//int DevicesNum = pTl->EnumerateDevices(devices);
		int DevicesNum = tlFactory.EnumerateDevices(devices);
		DeviceInfoList_t::const_iterator it;
		int count = 0;
		for (it = devices.begin(); it != devices.end(); ++it)
		{
			std::cout << "相机编号 : " << count << std::endl;
			std::cout << "SerialNumber : " << it->GetSerialNumber() << std::endl;
			std::cout << "UserDefinedName: " << it->GetUserDefinedName() << std::endl;
			std::cout << "ModelName: " << it->GetModelName() << std::endl;
			std::cout << "DeviceVersion: " << it->GetDeviceVersion() << std::endl;
			std::cout << "DeviceFactory: " << it->GetDeviceFactory() << std::endl;
			std::cout << "XMLSource: " << it->GetXMLSource() << std::endl;
			std::cout << "FriendlyName: " << it->GetFriendlyName() << std::endl;
			std::cout << "FullName: " << it->GetFullName() << std::endl;
			std::cout << "VendorName: " << it->GetVendorName() << std::endl;
			std::cout << "DeviceClass: " << it->GetDeviceClass() << std::endl;
			std::cout << "" << std::endl;
			++count;
		}

		if (DevicesNum == 0)
		{
			std::cout << "Cannot find any camera!" << std::endl;
			PylonTerminate();
			return 0;
		}
		if (!SetMgr.SelectDevice(DevicesNum))
		{
			PylonTerminate();
			return 0;
		}
		const size_t CamNum = SetMgr.GetCamNum();
		// Create an array of instant cameras for the found devices and avoid exceeding a maximum number of devices.
		CInstantCameraArray cameras(std::min(CamNum, c_maxCamerasToUse));

		// Create and attach all Pylon Devices.
		for (size_t i = 0; i < cameras.GetSize(); ++i)
		{
			cameras[i].Attach(tlFactory.CreateDevice(devices[SetMgr.CameraNumber[i]]));
			// Print the model name of the camera.
			std::cout << "Using device " << cameras[i].GetDeviceInfo().GetModelName() << std::endl;
			std::cout << "Serial Number " << cameras[i].GetDeviceInfo().GetSerialNumber() << std::endl;
			SetMgr.GetDevicesID(i, cameras[i].GetDeviceInfo().GetSerialNumber());
		}


		clrscr();
		cameras.Open();

		CIntegerParameter* width = new CIntegerParameter[CamNum];
		CIntegerParameter* height = new CIntegerParameter[CamNum];
		CEnumParameter pixelFormat;
		CIntegerParameter* exposuretimeRaw = new CIntegerParameter[CamNum];
		CStringParameter cameraname;
		CStringParameter serialnumber;
		for (size_t i = 0; i < cameras.GetSize(); ++i)
		{
			/*const char Filename[] = "acA1300-60gmNIR_21629490.pfs";
			CFeaturePersistence::Load(Filename, &cameras[i].GetNodeMap(), true);*/

			GenApi::INodeMap& nodemap = cameras[i].GetNodeMap();
			cameraname.Attach(nodemap, "DeviceModelName");
			serialnumber.Attach(nodemap, "DeviceID");
			width[i].Attach(nodemap, "Width");
			height[i].Attach(nodemap, "Height");
			//width.SetValue(1920, IntegerValueCorrection_Nearest);
			//height.SetValue(1080, IntegerValueCorrection_Nearest);
			CFloatParameter framerate(cameras[i].GetNodeMap(), "AcquisitionFrameRateAbs");
			CIntegerParameter packetdelay(cameras[i].GetNodeMap(), "GevSCPD");
			CIntegerParameter packetsize(cameras[i].GetNodeMap(), "GevSCPSPacketSize");
			std::cout << "CameraName       : " << cameraname.GetValue() << std::endl;
			std::cout << "SerialNumber     : " << serialnumber.GetValue() << std::endl;
			std::cout << "Width            : " << width[i].GetValue() << std::endl;
			std::cout << "Height           : " << height[i].GetValue() << std::endl;
			std::cout << "Delay            : " << packetdelay.GetValue() << std::endl;
			std::cout << "PacketSize       : " << packetsize.GetValue() << std::endl;
			std::cout << "FrameRate        : " << framerate.GetValue() << std::endl;

			pixelFormat.Attach(nodemap, "PixelFormat");
			// Remember the current pixel format.
			String_t oldPixelFormat = pixelFormat.GetValue();
			std::cout << "Old PixelFormat  : " << oldPixelFormat << std::endl;

			// Set the pixel format to Mono8 if available.
			if (pixelFormat.CanSetValue("Mono8"))
			{
				pixelFormat.SetValue("Mono8");
				std::cout << "New PixelFormat  : " << pixelFormat.GetValue() << std::endl;
			}

			if (ParamMgr.WhetherSetFrameRate() && framerate.IsWritable())
			{
				std::cout << " value range :  " << framerate.GetMin() << "~" << framerate.GetMax() << std::endl;
				std::cout << " input framerate :  " << std::endl;
				__int64 _framerate = 0;
				std::cin >> _framerate;
				framerate.SetValue(_framerate, IntegerValueCorrection_Nearest);
				std::cout << "Change framerate to  : " << framerate.GetValue() << std::endl;
			}
			if (cameras[i].GetSfncVersion() >= Sfnc_2_0_0)
			{
				// Access the Gain float type node. This node is available for USB camera devices.
				// USB camera devices are compliant to SFNC version 2.0.
				CFloatParameter gain(cameras[i].GetNodeMap(), "Gain");
				CFloatParameter exposuretime(cameras[i].GetNodeMap(), "ExposureTime");
				ParamMgr.getExposureTime(exposuretime.GetValue());
				ParamMgr.getMaxET(exposuretime.GetMax());
				ParamMgr.getMinET(exposuretime.GetMin());
				ParamMgr.getETinc(exposuretime.GetInc());
				//gain.SetValuePercentOfRange(50.0);
				std::cout << "Gain (100%)       : " << gain.GetValue() << " (Min: " << gain.GetMin()
					<< "; Max: " << gain.GetMax() << ")" << std::endl;
				if (ParamMgr.WhetherSetGain() && gain.IsWritable())
				{
					std::cout << " value range :  " << gain.GetMin() << "~" << gain.GetMax() << std::endl;
					std::cout << " input gain :  " << std::endl;
					float _gain = 0;
					std::cin >> _gain;
					gain.SetValue(_gain, FloatValueCorrection_ClipToRange);
					std::cout << "Change gain to  : " << gain.GetValue() << std::endl;
				}
				std::cout << "ExposureTime (100%)       : " << ParamMgr.getExposureTimeValue() << " (Min: " << ParamMgr.getMinETValue() << "; Max: "
					<< ParamMgr.getMaxETValue() << "; Inc: " << ParamMgr.getETincValue() << ")" << std::endl;
				if (ParamMgr.WhetherSetET() && exposuretime.IsWritable())
				{
					std::cout << " value range :  " << exposuretime.GetMin() << "~" << exposuretime.GetMax() << std::endl;
					std::cout << " input exposuretime :  " << std::endl;
					float _exposuretime = 0;
					std::cin >> _exposuretime;
					exposuretime.SetValue(_exposuretime, FloatValueCorrection_ClipToRange);
					ParamMgr.getExposureTime(exposuretime.GetValue());
					std::cout << "Change exposuretime to  : " << ParamMgr.getExposureTimeValue() << std::endl;
				}
			}
			else
			{
				// Access the GainRaw integer type node. This node is available for IIDC 1394 and GigE camera devices.
				CIntegerParameter gainRaw(cameras[i].GetNodeMap(), "GainRaw");
				exposuretimeRaw[i].Attach(nodemap, "ExposureTimeRaw");
				ParamMgr.getExposureTimeRaw(exposuretimeRaw[i].GetValue(), i);
				ParamMgr.getMaxETR(exposuretimeRaw[i].GetMax());
				ParamMgr.getMinETR(exposuretimeRaw[i].GetMin());
				ParamMgr.getETRinc(exposuretimeRaw[i].GetInc());
				//gainRaw.SetValuePercentOfRange(50.0);
				std::cout << "GainRaw (100%)       : " << gainRaw.GetValue() << " (Min: " << gainRaw.GetMin() << "; Max: "
					<< gainRaw.GetMax() << "; Inc: " << gainRaw.GetInc() << ")" << std::endl;
				if (ParamMgr.WhetherSetGainRaw() && gainRaw.IsWritable())
				{
					std::cout << " value range :  " << gainRaw.GetMin() << "~" << gainRaw.GetMax() << std::endl;
					std::cout << " input gainRaw :  " << std::endl;
					_int64 _gainRaw = 0;
					std::cin >> _gainRaw;
					gainRaw.SetValue(_gainRaw, IntegerValueCorrection_Nearest);
					std::cout << "Change gainRaw to  : " << gainRaw.GetValue() << std::endl;
				}
				std::cout << "ExposureTimeRaw (100%)       : " << exposuretimeRaw[i].GetValue() << " (Min: " << ParamMgr.getMinETRValue() << "; Max: "
					<< ParamMgr.getMaxETRValue() << "; Inc: " << ParamMgr.getETRincValue() << ")" << std::endl;
				if (ParamMgr.WhetherSetETR() && exposuretimeRaw[i].IsWritable())
				{
					std::cout << " value range :  " << exposuretimeRaw[i].GetMin() << "~" << exposuretimeRaw[i].GetMax() << std::endl;
					std::cout << " input exposuretimeRaw :  " << std::endl;
					__int64 _exposuretimeRaw = 0;
					std::cin >> _exposuretimeRaw;
					exposuretimeRaw[i].SetValue(_exposuretimeRaw, IntegerValueCorrection_Nearest);
					ParamMgr.getExposureTimeRaw(exposuretimeRaw[i].GetValue(), i);
					std::cout << "Change exposuretimeRaw to  : " << ParamMgr.getExposureTimeRawValue(i) << std::endl;
				}
			}
		}

		// Map the pixelType
		CPixelTypeMapper pixelTypeMapper(&pixelFormat);
		EPixelType pixelType = pixelTypeMapper.GetPylonPixelTypeFromNodeValue(pixelFormat.GetIntValue());

		// Create a video writer object.

		// The frame rate used for playing the video (playback frame rate).
		const int cFramesPerSecond = 25;
		// The quality used for compressing the video.
		const uint32_t cQuality = 100;

		CVideoWriter* VideoWriter = new CVideoWriter[CamNum * 2];
		CVideoWriter** VwPtr = new CVideoWriter * [CamNum * 3];
		for (int i = 0; i < CamNum; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				VideoWriter[i * 2 + j].ThreadCount = 16;
				VideoWriter[i * 2 + j].SetParameter((uint32_t)width[i].GetValue(),
					(uint32_t)height[i].GetValue(),
					pixelType,
					cFramesPerSecond,
					cQuality);
			}
			for (int j = 0; j < 2; j++)
			{
				VwPtr[i * 3 + j] = &VideoWriter[i * 2 + j];
			}
			VwPtr[i * 3 + 2] = nullptr;
		}

		using memfunc_type = void (CVideoWriter::*)(const IImage& image);
		memfunc_type memfunc = &CVideoWriter::Add;

		GetLocalTime(&sys);
		std::ostringstream FirstName;
		FirstName << sys.wMonth << "-" << sys.wDay << "-" << sys.wHour << "-" << sys.wMinute << "-" << sys.wSecond;

		for (int i = 0; i < CamNum; i++)
		{
			VwPtr[i * 3]->Open(SetMgr.GetStoragePath().c_str() + SetMgr.DevicesID[i] + "_" + FirstName.str().c_str() + "_First.mp4");
			if (!VwPtr[i * 3]->IsOpen())
				StopFlag = true;
		}

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		// Starts grabbing for all cameras starting with index 0. The grabbing
		// is started for one camera after the other. That's why the images of all
		// cameras are not taken at the same time.
		// However, a hardware trigger setup can be used to cause all cameras to grab images synchronously.
		// According to their default configuration, the cameras are
		// set up for free-running continuous acquisition.
		auto start1 = std::chrono::high_resolution_clock::now();
		cameras.StartGrabbing(GrabStrategy_LatestImages);
		cameras.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);
		// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
		//cameras.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);
		auto stop1 = std::chrono::high_resolution_clock::now();
		double dr_ms1 = std::chrono::duration<double, std::milli>(stop1 - start1).count();
		std::cout << "RetrieveResult:  " << dr_ms1 << " ms " << std::endl;

		cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
		std::vector <std::vector<CPylonImage>> ImgVec;
		std::vector<CPylonImage>::iterator* VecIt = new std::vector<CPylonImage>::iterator[CamNum];
		CPylonImage* pImg = new CPylonImage[CamNum];
		int* ItCount = new int[CamNum];
		ImgVec.reserve(CamNum);

		for (int i = 0; i < CamNum; i++)
		{
			ImgVec.push_back(
				std::vector<CPylonImage>(10, CPylonImage::Create(PixelType_Mono8, (uint32_t)width[i].GetValue(), (uint32_t)height[i].GetValue()))
			);
			VecIt[i] = ImgVec[i].begin();
			pImg[i] = CPylonImage::Create(PixelType_Mono8, (uint32_t)width[i].GetValue(), (uint32_t)height[i].GetValue());
			ItCount[i] = 0;
		}


		clrscr();
		//cv::namedWindow("tstmorph", 0);
		//cv::namedWindow("bgmodel", 0);

		// Create Init Background Model
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

		int SparkRegion = 0;
		int SaveCount = 0;
		int BgCount = 0;
		bool SaveFlag = false;
		bool BgFlag = false;
		bool StripFlag = false;
		bool InMaskFlag = false;
		std::atomic<bool> UpdateFlag = false;
		std::atomic<bool> OpenFlag = false;

		cv::Mat Img;
		cv::Mat ImgThresh, ImgClosed;
		std::vector< std::vector<cv::Point> > allContours;
		std::vector<cv::Vec4i> hierarchy;

		Pool.enqueue([&] {
			cv::namedWindow("video", 0);
			cv::namedWindow("bgmodel", 0);
			cv::namedWindow("closed", 0);

			while (!StopFlag)
			{
				{
					std::unique_lock<std::mutex> lock(mtx);
					condLoop.wait(lock);
					cv::imshow("video", Img);
					cv::imshow("closed", ImgClosed);
					cv::imshow("bgmodel", BgModel); 
				}
				cv::waitKey(1);
			}
			std::cout << "imshow线程退出" << std::endl;
		});
		Pool.enqueue([&] {
			while (true)
			{
				{
					std::unique_lock<std::mutex> lock(mtx);
					//while (!UpdateFlag)
					cond.wait(lock, [&] {return UpdateFlag == true || StopFlag; });
					auto t5 = std::chrono::high_resolution_clock::now();
					UpdateFlag = false;
					if (StopFlag)
					{
						std::cout << "boundingbox更新线程退出" << std::endl;
						break;
					}
					tempRRects.resize(BgContours.size());
					for (int i = 0; i < tempRRects.size(); i++)
					{
						tempRRects[i] = cv::fitEllipse(BgContours[i]).boundingRect2f();
					}
					BgRRects = tempRRects; 
					auto t6 = std::chrono::high_resolution_clock::now();
					double dr_ms3 = std::chrono::duration<double, std::milli>(t6 - t5).count();
					std::cout << "背景Bounding box更新完成，用时：" << dr_ms3 << "ms" << std::endl;
				}
			}
		});

		bool res = true;
		while (cameras.IsGrabbing())
		{
			auto t1 = std::chrono::high_resolution_clock::now();
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			/*res = cameras.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_Return);
			if (!res)
			{
				continue;
			}*/
			cameras.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);
			if (StopFlag)
			{
				std::cout << "StopFlag is trigged! " << std::endl;
				break;
			}
			if (_kbhit())
			{
				char ch = _getch();
				if (ch == 27 || ch == 'q')
				{
					break;//按键为esc或q 时退出
				}
				if (ch == 'w')
				{
					exposuretimeRaw[0].SetValue(ParamMgr.increaseETR(),
						IntegerValueCorrection_Nearest);
					std::cout << "Exposure Time changes to :      " << exposuretimeRaw[0].GetValue() << " us " << std::endl;
				}
				if (ch == 's')
				{
					exposuretimeRaw[0].SetValue(ParamMgr.decreaseETR(),
						IntegerValueCorrection_Nearest);
					std::cout << "Exposure Time changes to :      " << exposuretimeRaw[0].GetValue() << " us " << std::endl;
				}
				if (ch == 'a' && CamNum == 2)
				{
					exposuretimeRaw[1].SetValue(ParamMgr.increaseETR_2(),
						IntegerValueCorrection_Nearest);
					std::cout << "Exposure Time changes to :      " << exposuretimeRaw[1].GetValue() << " us " << std::endl;
				}
				if (ch == 'd' && CamNum == 2)
				{
					exposuretimeRaw[1].SetValue(ParamMgr.decreaseETR_2(),
						IntegerValueCorrection_Nearest);
					std::cout << "Exposure Time changes to :      " << exposuretimeRaw[1].GetValue() << " us " << std::endl;

				}
			}
			intptr_t cameraContextValue = ptrGrabResult->GetCameraContext();
			if (!ptrGrabResult->GrabSucceeded())
			{
				std::cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << std::endl;
				continue;
			}

#ifdef PYLON_WIN_BUILD
			// Display the grabbed image.
			//Pylon::DisplayImage(cameraContextValue, ptrGrabResult);
#endif	
			// Proc image
			//mtx.lock();
			Img = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, (uint8_t*)ptrGrabResult->GetBuffer());

			cv::threshold(Img, ImgThresh, BinaryThresh, 255, cv::THRESH_BINARY);
			cv::morphologyEx(ImgThresh, ImgClosed, cv::MORPH_CLOSE, closeKernel, cv::Point(-1, -1), 1);
			cv::findContours(ImgClosed, allContours, hierarchy, 0, 1);

			//mtx.unlock();
			condLoop.notify_one();

			//cv::imshow("tstmorph", ImgClosed);
			//cv::imshow("bgmodel", BgModel);
			//cv::waitKey(1);

			SparkRegion = 0;
			SaveFlag = false;
			BgFlag = false;
			if (allContours.empty())
			{
				//std::cout << "No Contours, No Spark" << std::endl;
				++SaveCount;
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
								if ((BgRegion & Props.region.Ellipse().boundingRect2f()).empty())
								{
									auto InterRect = BgRegion & Props.region.Ellipse().boundingRect2f();
									InterArea = InterRect.area();
									break;
								}
							}
							//std::cout << "InterArea: " << InterArea << "   Area:  " << Props.region.Area() << std::endl;
							if (InterArea / Props.region.Area() < IntersectThresh)
							{
								++SparkRegion;
								RegionAreas.push_back(Props.region.Area());
								//std::cout << "InterRate: " << InterArea/Props.region.Area()  << std::endl;
							}
						}
					}
				}
				auto MaxArea = std::max_element(std::begin(RegionAreas), std::end(RegionAreas));
				if (SparkRegion > 0)
				{
					std::cout << "有火花，最大的火花面积： " << *MaxArea << std::endl;
					SaveFlag = true;
					BgFlag = false;
				}
				else
				{
					//std::cout << "no spark" << std::endl;
				}
				++SaveCount;
			}
			if (SaveFlag)
			{
				SaveCount = 0;
			}
			try
			{
				if (BgFlag)
				{
					BgImgs[BgCount] = ImgClosed;
					//std::cout << "BgCount: " << BgCount << std::endl;
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
			}
			catch (const std::exception& e)
			{
				std::cerr << "An exception occurred." << std::endl
					<< e.what() << std::endl;
				break;
			}
			catch (...)
			{
				std::cerr << "Unknown occurred" << std::endl;
				break;
			}
			//std::cout << "SaveCount: " << SaveCount << std::endl;

			pImg[cameraContextValue].AttachGrabResultBuffer(ptrGrabResult);
#ifdef PYLON_WIN_BUILD
			// Display the grabbed image.
			//Pylon::DisplayImage(0, pImg[0]);
			//Pylon::DisplayImage(1, pImg[1]);
#endif	
			*VecIt[cameraContextValue] = pImg[cameraContextValue];
			try
			{
				/*if (SaveCount > 10000)
				{
					std::cout << "SaveCount > 400 " << std::endl;
					continue;
				}*/

				Pool.enqueue([&, cameraContextValue] {
					VwPtr[cameraContextValue * 3]->Add(*VecIt[cameraContextValue]);
				});

				++VecIt[cameraContextValue];
				++ItCount[cameraContextValue];
				if (ItCount[cameraContextValue] == 8)
				{
					VecIt[cameraContextValue] = ImgVec[cameraContextValue].begin();
					ItCount[cameraContextValue] = 0;
				}
			}
			catch (const GenericException& e)
			{
				// Error handling.
				std::cerr << "An ---------------VideoWriter---------- exception occurred." << std::endl
					<< e.GetDescription() << std::endl;
				CImagePersistence::Save(ImageFileFormat_Png, "D:\\lay\\c++\\errorImage\\1.png", ptrGrabResult);
				exitCode = 2;
				break;
			}
			if (BytesThreshold < VwPtr[cameraContextValue * 3]->BytesWritten.GetValue() && !OpenFlag)
			{
				std::cout << "The image data size limit has been reached." << std::endl;
				SetMgr.WhetherProgContinue();
				Pool.enqueue([&, cameraContextValue] {
					OpenFlag = true;
					GetLocalTime(&sys);
					std::ostringstream vName;
					vName << sys.wMonth << "-" << sys.wDay << "-" << sys.wHour << "-" << sys.wMinute << "-" << sys.wSecond;
					VwPtr[cameraContextValue * 3 + 1]->Open(SetMgr.GetStoragePath().c_str() + SetMgr.DevicesID[cameraContextValue] + "_" + vName.str().c_str() + ".mp4");
					if (!VwPtr[cameraContextValue * 3 + 1]->IsOpen())
					{
						std::cerr << "Could not open the new video file for write\n" << std::endl;
						StopFlag = true;
					}
					if (StopFlag)
					{
						std::cout << "创建新视频线程退出" << std::endl;
						return;
					}
					VwPtr[cameraContextValue * 3 + 2] = VwPtr[cameraContextValue * 3];
					VwPtr[cameraContextValue * 3] = VwPtr[cameraContextValue * 3 + 1];
					VwPtr[cameraContextValue * 3 + 1] = VwPtr[cameraContextValue * 3 + 2];
					VwPtr[cameraContextValue * 3 + 1]->Close();
					for (int i = 0; i < 3; i++)
					{
						std::cout << "VwPtr[" << i << "]的值:  " << VwPtr[cameraContextValue * 3 + i] << std::endl;
					}
					OpenFlag = false;
				});
			}
			auto t2 = std::chrono::high_resolution_clock::now();
			double dr_ms1 = std::chrono::duration<double, std::milli>(t2 - t1).count();
			std::cout << "相机序列号: " << cameraContextValue << '\t' << "处理一帧用时:  " << dr_ms1 << " ms " << std::endl;
		}
		StopFlag = true;
		condLoop.notify_all();
		cond.notify_all();
		condCreate.notify_all();

		GetLocalTime(&sys);
		std::ostringstream ptfname;
		ptfname << sys.wMonth << "-" << sys.wDay << "-" << sys.wHour << "-" << sys.wMinute << "-" << sys.wSecond;

		for (size_t i = 0; i < cameras.GetSize(); ++i)
		{
			CFeaturePersistence::Save(SetMgr.GetStoragePath().c_str() + SetMgr.DevicesID[i] + ptfname.str().c_str() + ".pfs", &cameras[i].GetNodeMap());
		}
		cv::waitKey(100);
		delete[] pImg;
		delete[] VideoWriter;
		delete[] VwPtr;
		delete[] ItCount;
		delete[] VecIt;
		delete[] width;
		delete[] height;
		delete[] exposuretimeRaw;
	}
	catch (const GenericException& e)
	{
		// Error handling
		std::cerr << "An exception occurred." << std::endl
			<< e.GetDescription() << std::endl;
		exitCode = 1;
	}

	// Comment the following two lines to disable waiting on exit.
	std::cerr << std::endl << "Press enter to exit." << std::endl;
	while (std::cin.get() != '\n');

	// Releases all pylon resources. 
	PylonTerminate();

	return exitCode;
}
