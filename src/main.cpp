// Utility_GrabVideo.cpp
/*
	Note: Before getting started, Basler recommends reading the "Programmer's Guide" topic
	in the pylon C++ API documentation delivered with pylon.
	If you are upgrading to a higher major version of pylon, Basler also
	strongly recommends reading the "Migrating from Previous Versions" topic in the pylon C++ API documentation.

	This sample illustrates how to create a video file in MP4 format.
*/

// Include files to use the pylon API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

#include <conio.h>
#include <iostream>
#include <chrono>
#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp> 
#include <opencv2/imgproc.hpp>


#include "Region.h"
#include "RegionProps.h"
#include "SPARKDECT.h"
#include "ParamManager.h"

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using GenApi objects.
using namespace GenApi;

// Namespace for using cout.
//using namespace std;

using namespace std::chrono;
using namespace cv;

// The maximum number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 100;
// When this amount of image data has been written, the grabbing is stopped.
static const int64_t c_maxImageDataBytesThreshold = 200 * 1024 * 1024;
const double eps = 1e-6;

bool descendSort(std::vector<cv::Point>& a, std::vector<cv::Point>& b)
{
	return a.size() > b.size();
}

int main(int argc, char* argv[])
{
	// The exit code of the sample application.
	int exitCode = 0;

	// Before using any pylon methods, the pylon runtime must be initialized. 

	RegionProps properties;
	sparkDect SPARK;
	ParamManager PARAM;
	if (!SPARK.CheckFreeSpace())
	{
		std::cout << "Reading disk is failed" << std::endl;
		return 1;
	}
	if (SPARK.GetFreeGBytesAvailable() < 1 + eps)
	{
		std::cout << "The disk doesn't have enough space" << std::endl;
		return 1;
	}
	std::cout << "The disk has " << SPARK.GetFreeGBytesAvailable() << " GB available space " << std::endl;
	std::cout << "Camera begins to work now " << std::endl;
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

		// Create a video writer object.
		CVideoWriter videoWriter;
		CLock videoLock;
		videoWriter.ThreadCount = 16;

		// The frame rate used for playing the video (playback frame rate).
		const int cFramesPerSecond = 20;
		// The quality used for compressing the video.
		const uint32_t cQuality = 100;

		CTlFactory& TlFactory = CTlFactory::GetInstance();
		DeviceInfoList_t devices;
		ITransportLayer* pTl = TlFactory.CreateTl("BaslerGigE"); // 只寻找GigE类相机
		int numDevices = pTl->EnumerateDevices(devices);
		DeviceInfoList_t::const_iterator it;
		//std::vector<Pylon::String_t> allSerialNum;
		int count = 0;
		for (it = devices.begin(); it != devices.end(); ++it)
		{
			std::cout << "相机编号 : " << count << std::endl;
			std::cout << "SerialNumber : " << it->GetSerialNumber() << std::endl;
			//allSerialNum.push_back(it->GetSerialNumber());
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

		IPylonDevice* pDevice;
		if (numDevices == 0)
		{
			std::cout << "Cannot find any camera!" << std::endl;
			return 0;
		}
		/*else if (numDevices == 1)
		{
			pDevice = pTl->CreateFirstDevice();
		}*/
		else
		{
			pDevice = pTl->CreateFirstDevice(devices[SPARK.SelectDevice(numDevices)]);
			//pDevice = pTl->CreateFirstDevice();
		}

		// Open the camera.
		CInstantCamera camera(pDevice);
		camera.Open();

		/*// Create an instant camera object with the first camera device found.
		CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());

		// Print the model name of the camera.
		cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;



		// Get camera device information.

		INodeMap& nodemap = camera.GetNodeMap();
		cout << "Camera Device Information" << endl
			<< "=========================" << endl;
		cout << "Vendor           : "
			<< CStringParameter(nodemap, "DeviceVendorName").GetValue() << endl;
		cout << "Model            : "
			<< CStringParameter(nodemap, "DeviceModelName").GetValue() << endl;
		cout << "Firmware version : "
			<< CStringParameter(nodemap, "DeviceFirmwareVersion").GetValue() << endl << endl;

		// Camera settings.
		cout << "Camera Device Settings" << endl
			<< "======================" << endl;*/

			/*const char Filename[] = "avA1900-50gm_22786435.pfs";
			CFeaturePersistence::Load(Filename, &camera.GetNodeMap(), true);*/

			// Get the required camera settings.
		CIntegerParameter width(camera.GetNodeMap(), "Width");
		CIntegerParameter height(camera.GetNodeMap(), "Height");
		CEnumParameter pixelFormat(camera.GetNodeMap(), "PixelFormat");
		CFloatParameter framerate(camera.GetNodeMap(), "AcquisitionFrameRateAbs");
		CIntegerParameter packetdelay(camera.GetNodeMap(), "GevSCPD");
		CIntegerParameter packetsize(camera.GetNodeMap(), "GevSCPSPacketSize");

		PARAM.getFrameRate(framerate.GetValue());
		width.SetValue(1920, IntegerValueCorrection_Nearest);
		height.SetValue(1080, IntegerValueCorrection_Nearest);
		std::cout << "Width            : " << width.GetValue() << std::endl;
		std::cout << "Height           : " << height.GetValue() << std::endl;
		std::cout << "Delay            : " << packetdelay.GetValue() << std::endl;
		std::cout << "PacketSize       : " << packetsize.GetValue() << std::endl;
		std::cout << "FrameRate        : " << framerate.GetValue() << std::endl;
		if (PARAM.WhetherSetFrameRate() && framerate.IsWritable())
		{
			std::cout << " value range :  " << framerate.GetMin() << "~" << framerate.GetMax() << std::endl;
			std::cout << " input framerate :  " << std::endl;
			__int64 _framerate = 0;
			std::cin >> _framerate;
			framerate.SetValue(_framerate, IntegerValueCorrection_Nearest);
			PARAM.getFrameRate(framerate.GetValue());
			std::cout << "Change framerate to  : " << PARAM.getFrameRateValue() << std::endl;
		}

		CIntegerParameter exposuretimeRaw(camera.GetNodeMap(), "ExposureTimeRaw");

		//width.SetToMaximum();
		//height.SetToMaximum();
		// Optional: Depending on your camera or computer, you may not be able to save
		// a video without losing frames. Therefore, we limit the resolution:
		//width.SetValue( 640, IntegerValueCorrection_Nearest );
		//height.SetValue( 480, IntegerValueCorrection_Nearest );

		// Access the PixelFormat enumeration type node.
		//CEnumParameter pixelFormat(nodemap, "PixelFormat");

		// Remember the current pixel format.
		String_t oldPixelFormat = pixelFormat.GetValue();
		std::cout << "Old PixelFormat  : " << oldPixelFormat << std::endl;

		// Set the pixel format to Mono8 if available.
		if (pixelFormat.CanSetValue("Mono8"))
		{
			pixelFormat.SetValue("Mono8");
			std::cout << "New PixelFormat  : " << pixelFormat.GetValue() << std::endl;
		}
		if (camera.GetSfncVersion() >= Sfnc_2_0_0)
		{
			// Access the Gain float type node. This node is available for USB camera devices.
			// USB camera devices are compliant to SFNC version 2.0.
			CFloatParameter gain(camera.GetNodeMap(), "Gain");
			CFloatParameter exposuretime(camera.GetNodeMap(), "ExposureTime");
			PARAM.getGain(gain.GetValue());
			PARAM.getExposureTime(exposuretime.GetValue());
			PARAM.getMaxET(exposuretime.GetMax());
			PARAM.getMinET(exposuretime.GetMin());
			PARAM.getETinc(exposuretime.GetInc());
			//gain.SetValuePercentOfRange(50.0);
			std::cout << "Gain (100%)       : " << PARAM.getGainValue() << " (Min: " << gain.GetMin()
				<< "; Max: " << gain.GetMax() << ")" << std::endl;
			if (PARAM.WhetherSetGain() && gain.IsWritable())
			{
				std::cout << " value range :  " << gain.GetMin() << "~" << gain.GetMax() << std::endl;
				std::cout << " input gain :  " << std::endl;
				float _gain = 0;
				std::cin >> _gain;
				gain.SetValue(_gain, FloatValueCorrection_ClipToRange);
				PARAM.getGain(gain.GetValue());
				std::cout << "Change gain to  : " << PARAM.getGainValue() << std::endl;
			}
			std::cout << "ExposureTime (100%)       : " << PARAM.getExposureTimeValue() << " (Min: " << PARAM.getMinETValue() << "; Max: "
				<< PARAM.getMaxETValue() << "; Inc: " << PARAM.getETincValue() << ")" << std::endl;
			if (PARAM.WhetherSetET() && exposuretime.IsWritable())
			{
				std::cout << " value range :  " << exposuretime.GetMin() << "~" << exposuretime.GetMax() << std::endl;
				std::cout << " input exposuretime :  " << std::endl;
				float _exposuretime = 0;
				std::cin >> _exposuretime;
				exposuretime.SetValue(_exposuretime, FloatValueCorrection_ClipToRange);
				PARAM.getExposureTime(exposuretime.GetValue());
				std::cout << "Change exposuretime to  : " << PARAM.getExposureTimeValue() << std::endl;
			}
		}
		else
		{
			// Access the GainRaw integer type node. This node is available for IIDC 1394 and GigE camera devices.
			CIntegerParameter gainRaw(camera.GetNodeMap(), "GainRaw");
			CIntegerParameter exposuretimeRaw(camera.GetNodeMap(), "ExposureTimeRaw");
			PARAM.getGainRaw(gainRaw.GetValue());
			PARAM.getExposureTimeRaw(gainRaw.GetValue());
			PARAM.getMaxETR(exposuretimeRaw.GetMax());
			PARAM.getMinETR(exposuretimeRaw.GetMin());
			PARAM.getETRinc(exposuretimeRaw.GetInc());
			//gainRaw.SetValuePercentOfRange(50.0);
			std::cout << "GainRaw (100%)       : " << gainRaw.GetValue() << " (Min: " << gainRaw.GetMin() << "; Max: "
				<< gainRaw.GetMax() << "; Inc: " << gainRaw.GetInc() << ")" << std::endl;
			if (PARAM.WhetherSetGainRaw() && gainRaw.IsWritable())
			{
				std::cout << " value range :  " << gainRaw.GetMin() << "~" << gainRaw.GetMax() << std::endl;
				std::cout << " input gainRaw :  " << std::endl;
				_int64 _gainRaw = 0;
				std::cin >> _gainRaw;
				gainRaw.SetValue(_gainRaw, IntegerValueCorrection_Nearest);
				PARAM.getGainRaw(gainRaw.GetValue());
				std::cout << "Change gainRaw to  : " << PARAM.getGainRawValue() << std::endl;
			}
			std::cout << "ExposureTimeRaw (100%)       : " << exposuretimeRaw.GetValue() << " (Min: " << PARAM.getMinETRValue() << "; Max: "
				<< PARAM.getMaxETRValue() << "; Inc: " << PARAM.getETRincValue() << ")" << std::endl;
			if (PARAM.WhetherSetET() && exposuretimeRaw.IsWritable())
			{
				std::cout << " value range :  " << exposuretimeRaw.GetMin() << "~" << exposuretimeRaw.GetMax() << std::endl;
				std::cout << " input exposuretimeRaw :  " << std::endl;
				__int64 _exposuretimeRaw = 0;
				std::cin >> _exposuretimeRaw;
				exposuretimeRaw.SetValue(_exposuretimeRaw, IntegerValueCorrection_Nearest);
				PARAM.getExposureTimeRaw(exposuretimeRaw.GetValue());
				std::cout << "Change exposuretimeRaw to  : " << PARAM.getExposureTimeRawValue() << std::endl;
			}
		}


		// Map the pixelType
		CPixelTypeMapper pixelTypeMapper(&pixelFormat);
		EPixelType pixelType = pixelTypeMapper.GetPylonPixelTypeFromNodeValue(pixelFormat.GetIntValue());

		// Set parameters before opening the video writer.
		videoWriter.SetParameter(
			(uint32_t)width.GetValue(),
			(uint32_t)height.GetValue(),
			pixelType,
			cFramesPerSecond,
			cQuality);

		// Open the video writer.
		videoWriter.Open((SPARK.GetStoragePath() + "_TestVideo.mp4").c_str());

		// Start the grabbing of c_countOfImagesToGrab images.
		// The camera device is parameterized with a default configuration which
		// sets up free running continuous acquisition.
		//camera.StartGrabbing( c_countOfImagesToGrab, GrabStrategy_LatestImages);
		camera.StartGrabbing(GrabStrategy_LatestImages);

		std::cout << "Please wait. Images are being grabbed." << std::endl;

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		// Camera.StopGrabbing() is called automatically by the RetrieveResult() method
		// when c_countOfImagesToGrab images have been retrieved.

		int sparkRegion = 0;
		int SaveCount = 0;
		cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
		while (camera.IsGrabbing())
		{
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);
			if (_kbhit())
			{
				char ch = _getch();
				std::cout << ch << std::endl;
				if (ch == 27 || ch == 'q')
				{
					break;//按键为esc或q 时退出
				}
				if (ch == 'w')
				{
					exposuretimeRaw.SetValue(PARAM.increaseETR(),
						IntegerValueCorrection_Nearest);
				}
				if (ch == 's')
				{
					exposuretimeRaw.SetValue(PARAM.decreaseETR(),
						IntegerValueCorrection_Nearest);
				}
			}
			if (ptrGrabResult->GrabSucceeded())
			{
				// Access the image data.
				std::cout << "SizeX: " << ptrGrabResult->GetWidth() << std::endl;
				std::cout << "SizeY: " << ptrGrabResult->GetHeight() << std::endl;
				const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
				std::cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << std::endl << std::endl;
			}
			else
			{
				std::cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << std::endl;
				continue;
			}

#ifdef PYLON_WIN_BUILD
			// Display the grabbed image.
			Pylon::DisplayImage(1, ptrGrabResult);
#endif

			//////////////////proc image
			auto start1 = std::chrono::high_resolution_clock::now();

			sparkRegion = 0;
			// If required, the grabbed image is converted to the correct format and is then added to the video file.
			// If the orientation of the image does not mach the orientation required for video compression, the
			// image will be flipped automatically to ImageOrientation_TopDown, unless the input pixel type is Yuv420p.
			try
			{
				/*if (SaveCount <= 10)
				{
					videoWriter.Add(ptrGrabResult);
				}*/
				videoWriter.Add(ptrGrabResult);
				//cv::waitKey(2);
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

			cv::Mat Img(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, (uint8_t*)ptrGrabResult->GetBuffer());

			cv::Mat imgThresh;
			cv::threshold(Img, imgThresh, 125, 255, cv::THRESH_BINARY);


			cv::Mat imgClosed;
			cv::morphologyEx(imgThresh, imgClosed, cv::MORPH_CLOSE, closeKernel, cv::Point(-1, -1), 1);


			std::vector< std::vector<cv::Point> > allContours;
			std::vector<cv::Vec4i> hierarchy;
			cv::findContours(imgClosed, allContours, hierarchy, 0, 1);

			if (allContours.empty())
			{
				std::cout << "没有Contour" << std::endl;
				std::cout << "没有火花" << std::endl;
				++SaveCount;
				continue;
			}
			std::sort(allContours.begin(), allContours.end(), descendSort);
			if (allContours[0].size() <= 5)
			{
				std::cout << "allContours[0].size() :    " << allContours[0].size() << std::endl;
				std::cout << "没有火花" << std::endl;
				++SaveCount;
				continue;
			}

			for (int i = 0; i < allContours.size(); i++)
			{
				std::cout << "CONTOUR[i] SIZE: " << allContours[i].size() << std::endl;
				properties.getContour(allContours[i]);
				//std::cout << "CONTOUR SIZE: " << properties.contour.size() << std::endl;
				properties.compute();
				if (properties.region.Area() < 200 - eps)
				{
					//allContours.shrink_to_fit();
					break;
				}
				if (fabs(properties.region.Orientation()) < 90 - eps || properties.region.DeltaAxis() < 100 - eps)
				{
					++sparkRegion;
				}
			}
			if (sparkRegion > 0)
			{
				SaveCount = 0;
			}
			else
			{
				++SaveCount;
			}


			//cv::Mat labelsMat; //CV_16U
			//cv::Mat statsMat; //CV_32S
			//cv::Mat centerPoints; //CV_64F
			//auto numConnectedRegions = cv::connectedComponentsWithStats(imgClosed, labelsMat,statsMat, centerPoints, 8, 2, cv::CCL_DEFAULT);
			//auto NumThrds = getNumThreads();
			//cout << "cv线程数量 : " << NumThrds << "个" << endl;

			auto stop1 = std::chrono::high_resolution_clock::now();
			auto duration1 = duration_cast<microseconds>(stop1 - start1);
			auto timetaken1 = duration1.count() / 1000;

			std::cout << "Img Proc Time : " << timetaken1 << "ms" << std::endl;
			//std::cout << "连通域个数 : " << numConnectedRegions << "个" << std::endl;
			std::cout << "contour个数 : " << allContours.size() << "个" << std::endl;
			std::cout << "Spark Region : " << sparkRegion << "块" << std::endl;
			std::cout << "Save count : " << SaveCount << "次" << std::endl;

			cv::imshow("tstbin", imgThresh);
			cv::imshow("tstmorph", imgClosed);
			cv::waitKey(1);

			// If images are skipped, writing video frames takes too much processing time.
			std::cout << "Images Skipped = " << ptrGrabResult->GetNumberOfSkippedImages() << std::boolalpha
				<< "; Image has been converted = " << !videoWriter.CanAddWithoutConversion(ptrGrabResult)
				<< std::endl;

			// Check whether the image data size limit has been reached to avoid the video file becoming too large.
			if (c_maxImageDataBytesThreshold < videoWriter.BytesWritten.GetValue())
			{
				std::cout << "The image data size limit has been reached." << std::endl;
				if (!SPARK.WhetherProgContinue())
				{
					std::cerr << "the disk doesn't have enough space" << std::endl;
					break;
				}
				videoWriter.Close();
				SYSTEMTIME sys;
				GetLocalTime(&sys);
				std::ostringstream vName;
				vName << "V" << sys.wMonth << "-" << sys.wDay << "-" << sys.wHour << "-" << sys.wMinute << ".mp4";
				videoWriter.SetParameter(
					(uint32_t)width.GetValue(),
					(uint32_t)height.GetValue(),
					pixelType,
					cFramesPerSecond,
					cQuality);
				videoWriter.Open((SPARK.GetStoragePath() + vName.str()).c_str());

				//cvVideoCreator = SPARK.CreateNewVideo();
				if (!videoWriter.IsOpen())
				{
					std::cerr << "Could not open the new video file for write\n" << std::endl;
					break;
				}
			}
		}
	}
	catch (const GenericException &e)
	{
		// Error handling.
		std::cerr << "An exception occurred." << std::endl
			<< e.GetDescription() << std::endl;
		exitCode = 1;
	}

	// Releases all pylon resources. 
	PylonTerminate();

	// Comment the following two lines to disable waiting on exit.
	std::cerr << std::endl << "Press enter to exit." << std::endl;
	while (std::cin.get() != '\n');



	return exitCode;
}
