#pragma once

#include <iostream>
#include <Windows.h>
#include <pylon/PylonIncludes.h>

class sparkDect_Mul
{
public:
	sparkDect_Mul()
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
	void SelectDevice(int& DevicesNumber);
	bool WhetherProgContinue();
	std::string GetStoragePath();
	void GetCamName(const int& index, const Pylon::String_t& name);
	int CamNum[2]{ 0,0 };
	Pylon::String_t CamName[2] = { "","" };

private:
	std::string StoragePath = "D:\\lay\\c++\\output";
	std::string DiskName = "D:";
	_ULARGE_INTEGER _i64FreeBytesToCaller;
	_ULARGE_INTEGER _i64TotalBytes;
	_ULARGE_INTEGER _i64FreeBytes;
	double FreeSpaceGB = 0;

	const double eps = 1e-6;
};

