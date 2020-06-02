#pragma once

#include<iostream>
#include<Windows.h>
#include <pylon/PylonIncludes.h>

class sparkDect
{
public:
	sparkDect()
	{
		SelectPath();
		GetDiskName();
		//_i64FreeBytes.QuadPart = 0L;
		//_i64FreeBytesToCaller.QuadPart = 0L;
		//_i64TotalBytes.QuadPart = 0L;
	};
	void SelectPath();
	void GetDiskName();
	bool CheckFreeSpace();
	double GetFreeGBytesAvailable();
	int SelectDevice(int& DevicesNumber);
	bool WhetherProgContinue();
	std::string GetStoragePath();


private:
	std::string StoragePath = "D:\\lay\\c++\\output";
	std::string DiskName = "D:";
	_ULARGE_INTEGER _i64FreeBytesToCaller;
	_ULARGE_INTEGER _i64TotalBytes;
	_ULARGE_INTEGER _i64FreeBytes;
	double FreeSpaceGB = 0;
};


