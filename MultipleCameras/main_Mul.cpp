// Grab_MultipleCameras.cpp
/*
	Note: Before getting started, Basler recommends reading the "Programmer's Guide" topic
	in the pylon C++ API documentation delivered with pylon.
	If you are upgrading to a higher major version of pylon, Basler also
	strongly recommends reading the "Migrating from Previous Versions" topic in the pylon C++ API documentation.

	This sample illustrates how to grab and process images from multiple cameras
	using the CInstantCameraArray class. The CInstantCameraArray class represents
	an array of instant camera objects. It provides almost the same interface
	as the instant camera for grabbing.
	The main purpose of the CInstantCameraArray is to simplify waiting for images and
	camera events of multiple cameras in one thread. This is done by providing a single
	RetrieveResult method for all cameras in the array.
	Alternatively, the grabbing can be started using the internal grab loop threads
	of all cameras in the CInstantCameraArray. The grabbed images can then be processed by one or more
	image event handlers. Please note that this is not shown in this example.
*/

// Include files to use the pylon API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

#include "conio.h"
#include "Region.h"
#include "RegionProps.h"
#include "SPARKDECT_Mul.h"
#include "ParamManager_Mul.h"

#include <chrono>
#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp> 
#include <opencv2/imgproc.hpp>
#include <Windows.h>
#include <thread>

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 10;
static const int64_t BytesThreshold = 50 * 1024 * 1024;
static const int BinaryThreshold = 125;
static const int AreaThreshold = 200;
static const int AngleThreshold = 80;
static const int DeltaAxisThreshold = 100;

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
static const double eps = 1e-6;

bool descendSort(std::vector<cv::Point>& a, std::vector<cv::Point>& b)
{
	return a.size() > b.size();
}

int main(int argc, char* argv[])
{
	// The exit code of the sample application.
	int exitCode = 0;
	ParamManager_Mul PARAM_MUL;
	sparkDect_Mul SPARK_MUL;
	RegionProps properties;
	SYSTEMTIME sys;

	if (!SPARK_MUL.CheckFreeSpace())
	{
		std::cout << "Reading disk is failed" << std::endl;
		return 1;
	}
	if (SPARK_MUL.GetFreeGBytesAvailable() < 1 + eps)
	{
		std::cout << "The disk doesn't have enough space" << std::endl;
		return 1;
	}
	std::cout << "The disk has " << SPARK_MUL.GetFreeGBytesAvailable() << " GB available space " << std::endl;
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
		ITransportLayer* pTl = tlFactory.CreateTl("BaslerGigE"); // Only find GigE cameras
		int numDevices = pTl->EnumerateDevices(devices);
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

		if (numDevices == 0)
		{
			std::cout << "Cannot find any camera!" << std::endl;
			return 0;
		}
		SPARK_MUL.SelectDevice(numDevices);

		// Create an array of instant cameras for the found devices and avoid exceeding a maximum number of devices.
		CInstantCameraArray cameras(c_maxCamerasToUse);

		// Create and attach all Pylon Devices.
		for (size_t i = 0; i < cameras.GetSize(); ++i)
		{
			cameras[i].Attach(tlFactory.CreateDevice(devices[SPARK_MUL.CamNum[i]]));
			// Print the model name of the camera.
			cout << "Using device " << cameras[i].GetDeviceInfo().GetModelName() << endl;
		}

		cameras.Open();

		CIntegerParameter width;
		CIntegerParameter height;
		CEnumParameter pixelFormat;
		CIntegerParameter exposuretimeRaw[c_maxCamerasToUse];
		CStringParameter cameraname;
		for (size_t i = 0; i < cameras.GetSize(); ++i)
		{
			/*const char Filename[] = "acA1300-60gmNIR_21629490.pfs";
			CFeaturePersistence::Load(Filename, &cameras[i].GetNodeMap(), true);*/

			GenApi::INodeMap& nodemap = cameras[i].GetNodeMap();

			cameraname.Attach(nodemap, "DeviceModelName");
			SPARK_MUL.GetCamName(i, cameraname.GetValue());
			width.Attach(nodemap, "Width");
			height.Attach(nodemap, "Height");
			width.SetValue(1024, IntegerValueCorrection_Nearest);
			height.SetValue(768, IntegerValueCorrection_Nearest);
			CFloatParameter framerate(cameras[i].GetNodeMap(), "AcquisitionFrameRateAbs");
			CIntegerParameter packetdelay(cameras[i].GetNodeMap(), "GevSCPD");
			CIntegerParameter packetsize(cameras[i].GetNodeMap(), "GevSCPSPacketSize");
			std::cout << "CameraName       : " << cameraname.GetValue() << std::endl;
			std::cout << "Width            : " << width.GetValue() << std::endl;
			std::cout << "Height           : " << height.GetValue() << std::endl;
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

			if (PARAM_MUL.WhetherSetFrameRate() && framerate.IsWritable())
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
				PARAM_MUL.getExposureTime(exposuretime.GetValue());
				PARAM_MUL.getMaxET(exposuretime.GetMax());
				PARAM_MUL.getMinET(exposuretime.GetMin());
				PARAM_MUL.getETinc(exposuretime.GetInc());
				//gain.SetValuePercentOfRange(50.0);
				std::cout << "Gain (100%)       : " << gain.GetValue() << " (Min: " << gain.GetMin()
					<< "; Max: " << gain.GetMax() << ")" << std::endl;
				if (PARAM_MUL.WhetherSetGain() && gain.IsWritable())
				{
					std::cout << " value range :  " << gain.GetMin() << "~" << gain.GetMax() << std::endl;
					std::cout << " input gain :  " << std::endl;
					float _gain = 0;
					std::cin >> _gain;
					gain.SetValue(_gain, FloatValueCorrection_ClipToRange);
					std::cout << "Change gain to  : " << gain.GetValue() << std::endl;
				}
				std::cout << "ExposureTime (100%)       : " << PARAM_MUL.getExposureTimeValue() << " (Min: " << PARAM_MUL.getMinETValue() << "; Max: "
					<< PARAM_MUL.getMaxETValue() << "; Inc: " << PARAM_MUL.getETincValue() << ")" << std::endl;
				if (PARAM_MUL.WhetherSetET() && exposuretime.IsWritable())
				{
					std::cout << " value range :  " << exposuretime.GetMin() << "~" << exposuretime.GetMax() << std::endl;
					std::cout << " input exposuretime :  " << std::endl;
					float _exposuretime = 0;
					std::cin >> _exposuretime;
					exposuretime.SetValue(_exposuretime, FloatValueCorrection_ClipToRange);
					PARAM_MUL.getExposureTime(exposuretime.GetValue());
					std::cout << "Change exposuretime to  : " << PARAM_MUL.getExposureTimeValue() << std::endl;
				}
			}
			else
			{
				// Access the GainRaw integer type node. This node is available for IIDC 1394 and GigE camera devices.
				CIntegerParameter gainRaw(cameras[i].GetNodeMap(), "GainRaw");
				exposuretimeRaw[i].Attach(nodemap, "ExposureTimeRaw");
				PARAM_MUL.getExposureTimeRaw(exposuretimeRaw[i].GetValue(), i);
				PARAM_MUL.getMaxETR(exposuretimeRaw[i].GetMax());
				PARAM_MUL.getMinETR(exposuretimeRaw[i].GetMin());
				PARAM_MUL.getETRinc(exposuretimeRaw[i].GetInc());
				//gainRaw.SetValuePercentOfRange(50.0);
				std::cout << "GainRaw (100%)       : " << gainRaw.GetValue() << " (Min: " << gainRaw.GetMin() << "; Max: "
					<< gainRaw.GetMax() << "; Inc: " << gainRaw.GetInc() << ")" << std::endl;
				if (PARAM_MUL.WhetherSetGainRaw() && gainRaw.IsWritable())
				{
					std::cout << " value range :  " << gainRaw.GetMin() << "~" << gainRaw.GetMax() << std::endl;
					std::cout << " input gainRaw :  " << std::endl;
					_int64 _gainRaw = 0;
					std::cin >> _gainRaw;
					gainRaw.SetValue(_gainRaw, IntegerValueCorrection_Nearest);
					std::cout << "Change gainRaw to  : " << gainRaw.GetValue() << std::endl;
				}
				std::cout << "ExposureTimeRaw (100%)       : " << exposuretimeRaw[i].GetValue() << " (Min: " << PARAM_MUL.getMinETRValue() << "; Max: "
					<< PARAM_MUL.getMaxETRValue() << "; Inc: " << PARAM_MUL.getETRincValue() << ")" << std::endl;
				if (PARAM_MUL.WhetherSetETR() && exposuretimeRaw[i].IsWritable())
				{
					std::cout << " value range :  " << exposuretimeRaw[i].GetMin() << "~" << exposuretimeRaw[i].GetMax() << std::endl;
					std::cout << " input exposuretimeRaw :  " << std::endl;
					__int64 _exposuretimeRaw = 0;
					std::cin >> _exposuretimeRaw;
					exposuretimeRaw[i].SetValue(_exposuretimeRaw, IntegerValueCorrection_Nearest);
					PARAM_MUL.getExposureTimeRaw(exposuretimeRaw[i].GetValue(), i);
					std::cout << "Change exposuretimeRaw to  : " << PARAM_MUL.getExposureTimeRawValue(i) << std::endl;
				}
			}
		}

		// Map the pixelType
		CPixelTypeMapper pixelTypeMapper(&pixelFormat);
		EPixelType pixelType = pixelTypeMapper.GetPylonPixelTypeFromNodeValue(pixelFormat.GetIntValue());

		// Create a video writer object.
		CVideoWriter* videoWriter = new CVideoWriter[c_maxCamerasToUse];

		using memfunc_type = void (CVideoWriter::*)(const IImage& image);
		memfunc_type memfunc = &CVideoWriter::Add;

		// The frame rate used for playing the video (playback frame rate).
		const int cFramesPerSecond = 20;
		// The quality used for compressing the video.
		const uint32_t cQuality = 100;

		GetLocalTime(&sys);
		std::ostringstream FirstName;
		FirstName << sys.wMonth << "-" << sys.wDay << "-" << sys.wHour << "-" << sys.wMinute << "-" << sys.wSecond;

		for (int i = 0; i < c_maxCamerasToUse; i++)
		{
			videoWriter[i].ThreadCount = 16;
			// Set parameters before opening the video writer.
			videoWriter[i].SetParameter(
				(uint32_t)width.GetValue(),
				(uint32_t)height.GetValue(),
				pixelType,
				cFramesPerSecond,
				cQuality);

			videoWriter[i].Open(SPARK_MUL.GetStoragePath().c_str() + SPARK_MUL.CamName[i] + "_" + FirstName.str().c_str() + "_First.mp4");
		}

		// Starts grabbing for all cameras starting with index 0. The grabbing
		// is started for one camera after the other. That's why the images of all
		// cameras are not taken at the same time.
		// However, a hardware trigger setup can be used to cause all cameras to grab images synchronously.
		// According to their default configuration, the cameras are
		// set up for free-running continuous acquisition.
		cameras.StartGrabbing(GrabStrategy_LatestImages);

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		// Grab c_countOfImagesToGrab from the cameras.
		/*for (uint32_t i = 0; i < c_countOfImagesToGrab && cameras.IsGrabbing(); ++i)
		{
			cameras.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

			// When the cameras in the array are created the camera context value
			// is set to the index of the camera in the array.
			// The camera context is a user settable value.
			// This value is attached to each grab result and can be used
			// to determine the camera that produced the grab result.
			intptr_t cameraContextValue = ptrGrabResult->GetCameraContext();

#ifdef PYLON_WIN_BUILD
			// Show the image acquired by each camera in the window related to each camera.
			Pylon::DisplayImage(cameraContextValue, ptrGrabResult);
#endif

			// Print the index and the model name of the camera.
			cout << "Camera " << cameraContextValue << ": " << cameras[cameraContextValue].GetDeviceInfo().GetModelName() << endl;

			// Now, the image data can be processed.
			cout << "GrabSucceeded: " << ptrGrabResult->GrabSucceeded() << endl;
			cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
			cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
			const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
			cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;

		}*/

		cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));

		std::vector <std::vector<CPylonImage>> ImgVec{ c_maxCamerasToUse,
			std::vector<CPylonImage>(10, CPylonImage::Create(PixelType_Mono8, (uint32_t)width.GetValue(), (uint32_t)height.GetValue())) };

		int ItCount[c_maxCamerasToUse]{ 0,0 };
		std::vector<CPylonImage>::iterator VecIt[c_maxCamerasToUse];
		for (int i = 0; i < c_maxCamerasToUse; i++)
		{
			VecIt[i] = ImgVec[i].begin();
		}
		CPylonImage* pImg = new CPylonImage[c_maxCamerasToUse]{ CPylonImage::Create(PixelType_Mono8, (uint32_t)width.GetValue(), (uint32_t)height.GetValue()) };
		std::thread WriteThread[c_maxCamerasToUse];

		int SparkRegion = 0;
		int SaveCount = 0;
		bool SaveFlag = false;
		while (cameras.IsGrabbing())
		{
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			cameras.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);
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
					exposuretimeRaw[0].SetValue(PARAM_MUL.increaseETR(),
						IntegerValueCorrection_Nearest);
				}
				if (ch == 's')
				{
					exposuretimeRaw[0].SetValue(PARAM_MUL.decreaseETR(),
						IntegerValueCorrection_Nearest);
				}
				if (ch == 'a')
				{
					exposuretimeRaw[1].SetValue(PARAM_MUL.increaseETR_2(),
						IntegerValueCorrection_Nearest);
				}
				if (ch == 'd')
				{
					exposuretimeRaw[1].SetValue(PARAM_MUL.decreaseETR_2(),
						IntegerValueCorrection_Nearest);
				}
			}

			intptr_t cameraContextValue = ptrGrabResult->GetCameraContext();
			std::cout << "Camera " << cameraContextValue << ": " << cameras[cameraContextValue].GetDeviceInfo().GetModelName() << std::endl;

			if (ptrGrabResult->GrabSucceeded())
			{
				// Access the image data.
				//std::cout << "SizeX: " << ptrGrabResult->GetWidth() << std::endl;
				//std::cout << "SizeY: " << ptrGrabResult->GetHeight() << std::endl;

				/*const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
				std::cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << std::endl << std::endl;*/
			}
			else
			{
				std::cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << std::endl;
				continue;
			}

#ifdef PYLON_WIN_BUILD
			// Display the grabbed image.
			Pylon::DisplayImage(cameraContextValue, ptrGrabResult);
#endif

			//////////////////proc image
			auto start1 = std::chrono::high_resolution_clock::now();

			cv::Mat Img(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, (uint8_t*)ptrGrabResult->GetBuffer());

			cv::Mat imgThresh;
			cv::threshold(Img, imgThresh, BinaryThreshold, 255, cv::THRESH_BINARY);


			cv::Mat imgClosed;
			cv::morphologyEx(imgThresh, imgClosed, cv::MORPH_CLOSE, closeKernel, cv::Point(-1, -1), 1);


			std::vector< std::vector<cv::Point> > allContours;
			std::vector<cv::Vec4i> hierarchy;
			cv::findContours(imgClosed, allContours, hierarchy, 0, 1);


			SparkRegion = 0;
			SaveFlag = false;
			if (allContours.empty())
			{
				std::cout << "No Contours, No Spark" << std::endl;
				++SaveCount;
			}
			else
			{
				std::sort(allContours.begin(), allContours.end(), descendSort);
				if (allContours[0].size() > 5)
				{
					for (int i = 0; i < allContours.size(); i++)
					{
						//std::cout << "CONTOUR[i] SIZE: " << allContours[i].size() << std::endl;
						properties.getContour(allContours[i]);
						//std::cout << "CONTOUR SIZE: " << properties.contour.size() << std::endl;
						properties.compute();
						if (properties.region.Area() < AreaThreshold - eps)
						{
							//allContours.shrink_to_fit();
							break;
						}
						if (fabs(properties.region.Orientation()) < AngleThreshold - eps || properties.region.DeltaAxis() < DeltaAxisThreshold - eps)
						{
							++SparkRegion;
						}
					}
				}
				if (SparkRegion > 0)
				{
					SaveFlag = true;
				}
				//std::cout << "allContours[0].size() :    " << allContours[0].size() << std::endl;
				std::cout << "No Contours, No Spark" << std::endl;
				++SaveCount;
			}
			if (SaveFlag)
			{
				SaveCount = 0;
			}

			pImg[cameraContextValue].AttachGrabResultBuffer(ptrGrabResult);
			*VecIt[cameraContextValue] = pImg[cameraContextValue];

			try
			{
				/*if (SaveCount > 10000)
				{	
					std::cout << "SaveCount > 2000 " << std::endl;
					continue;
				}*/
				WriteThread[cameraContextValue] = std::thread(memfunc, &videoWriter[cameraContextValue], *VecIt[cameraContextValue]);
				WriteThread[cameraContextValue].detach();
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
			if (BytesThreshold < videoWriter[cameraContextValue].BytesWritten.GetValue())
			{
				std::cout << "The image data size limit has been reached." << std::endl;
				SPARK_MUL.WhetherProgContinue();
				videoWriter[cameraContextValue].Close();
				GetLocalTime(&sys);
				std::ostringstream vName;
				vName << sys.wMonth << "-" << sys.wDay << "-" << sys.wHour << "-" << sys.wMinute << "-" << sys.wSecond;
				videoWriter[cameraContextValue].SetParameter(
					(uint32_t)width.GetValue(),
					(uint32_t)height.GetValue(),
					pixelType,
					cFramesPerSecond,
					cQuality);
				videoWriter[cameraContextValue].Open(SPARK_MUL.GetStoragePath().c_str() + SPARK_MUL.CamName[cameraContextValue] + "_" + vName.str().c_str() + ".mp4");

				//cvVideoCreator = SPARK_MUL.CreateNewVideo();
				if (!videoWriter[cameraContextValue].IsOpen())
				{
					std::cerr << "Could not open the new video file for write\n" << std::endl;
					break;
				}
			}
			// If images are skipped, writing video frames takes too much processing time.
			std::cout << "Images Skipped = " << ptrGrabResult->GetNumberOfSkippedImages() << std::boolalpha
				<< "; Image has been converted = " << !videoWriter[cameraContextValue].CanAddWithoutConversion(ptrGrabResult)
				<< std::endl;

			//auto NumThrds = getNumThreads();
			//cout << "cv线程数量 : " << NumThrds << "个" << endl;

			auto stop1 = std::chrono::high_resolution_clock::now();
			auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start1);
			auto timetaken1 = duration1.count() / 1000;

			std::cout << "Img Proc Time : " << timetaken1 << "ms" << std::endl;
			//std::cout << "连通域个数 : " << numConnectedRegions << "个" << std::endl;
			//std::cout << "contour个数 : " << allContours.size() << "个" << std::endl;
			//std::cout << "SparkRegion : " << SparkRegion << "块" << std::endl;
			//std::cout << "Save count : " << SaveCount << "次" << std::endl;

			//cv::imshow("tstbin", imgThresh);
			//cv::imshow("tstmorph", imgClosed);
			//cv::waitKey(1);

		}


		GetLocalTime(&sys);
		std::ostringstream ptfname;
		ptfname << sys.wMonth << "-" << sys.wDay << "-" << sys.wHour << "-" << sys.wMinute << "-" << sys.wSecond;

		for (size_t i = 0; i < cameras.GetSize(); ++i)
		{
			CFeaturePersistence::Save(SPARK_MUL.GetStoragePath().c_str() + SPARK_MUL.CamName[i] + ptfname.str().c_str() + ".pfs", &cameras[i].GetNodeMap());
		}
		cv::waitKey(100);
		delete[] pImg;
		delete[] videoWriter;
	}
	catch (const GenericException& e)
	{
		// Error handling
		cerr << "An exception occurred." << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
	}

	// Comment the following two lines to disable waiting on exit.
	cerr << endl << "Press enter to exit." << endl;
	while (cin.get() != '\n');

	// Releases all pylon resources. 
	PylonTerminate();

	return exitCode;
}
