#pragma once

#include <iostream>
#include <Windows.h>
#include <pylon/PylonIncludes.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include "Region.h"
#include "RegionProps.h"

class Setting
{
public:
	Setting()
	{	
		_i64FreeBytesToCaller.QuadPart = 0L;
		_i64TotalBytes.QuadPart = 0L;
		_i64FreeBytes.QuadPart = 0L;
		SelectPath();
		GetDiskName();
	};
	void SelectPath();
	void GetDiskName();
	bool CheckFreeSpace();
	double GetFreeGBytesAvailable();
	bool SelectDevice(int& DevicesNumber);
	void WhetherProgContinue();
	std::string GetStoragePath();
	void GetDevicesID(const int& index, const Pylon::String_t& name);
	void DeleteOldVideo();
	//bool SparkJudge(const cv::Mat* _img);
	std::vector<int> CameraNumber{ 2,10000 };
	std::vector<Pylon::String_t> DevicesID = { 10, "" };
	inline int GetCamNum() const
	{
		return TotalCamNum;
	}
private:
	std::string StoragePath = "D:";
	std::string DiskName = "";
	_ULARGE_INTEGER _i64FreeBytesToCaller;
	_ULARGE_INTEGER _i64TotalBytes;
	_ULARGE_INTEGER _i64FreeBytes;
	double FreeSpaceGB = 0;
	int TotalCamNum = 2;
	const double eps = 1e-6;

	//std::vector< std::vector<cv::Point> > allContours;
	//std::vector<cv::Vec4i> hierarchy;

	//RegionProps properties;

};

