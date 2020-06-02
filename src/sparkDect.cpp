#include "SPARKDECT.h"

//#include <pylon/PylonIncludes.h>

#include <iostream>
//#include <Windows.h>
#include <string>
#include <io.h>

const double eps = 1e-6;

void sparkDect::SelectPath()
{
	while (true)
	{
		std::cout << "input path for saving picture or video " << std::endl;
		std::cout << "press \"Enter\" to choose default path : " << StoragePath << std::endl;
		std::getline(std::cin, StoragePath);


		const char* c_PATH = StoragePath.c_str();

		if (CreateDirectoryA(c_PATH, NULL))
		{
			std::cout << "Storage path: " << '\n' << StoragePath << std::endl;
			break;
		}
		else if (StoragePath == "")
		{
			StoragePath = "D:\\lay\\c++\\output";
			std::cout << "storage path: " << '\n' << StoragePath << std::endl;
			break;
		}
		else if (!CreateDirectoryA(c_PATH, NULL) && _access(c_PATH, 00) == 0)
		{
			std::cout << "Storage path: " << '\n' << StoragePath << std::endl;
			break;
		}
		else
		{
			int ret = MessageBoxA(NULL, "存储路径错误，请重新输入或退出程序",
				"存储路径",
				MB_RETRYCANCEL + MB_ICONEXCLAMATION);
			if (ret == 2)
			{
				//Pylon::PylonTerminate();
				exit(1);
			}
			else
			{
				continue;
			}
		}
	}
}
void sparkDect::GetDiskName()
{
	char drive[256], dir[256], fname[256], ext[256];
	errno_t pathError = _splitpath_s(StoragePath.c_str(), drive, dir, fname, ext);
	std::string DiskName = drive;
	//std::cout <<"DiskName: "<< DiskName << std::endl;
}
bool sparkDect::CheckFreeSpace()
{
	if (!GetDiskFreeSpaceExA(
		DiskName.c_str(),                  // directory name
		&_i64FreeBytesToCaller,        // bytes available to caller
		&_i64TotalBytes,         // bytes on disk
		&_i64FreeBytes))   // free bytes on disk
	{
		return false;
	}
	FreeSpaceGB = (double)((_i64FreeBytesToCaller.QuadPart) / 1024.0 / 1024.0 / 1024.0);
	return true;
}
double sparkDect::GetFreeGBytesAvailable()
{
	return FreeSpaceGB;
}
int sparkDect::SelectDevice(int& DevicesNumber)
{
	std::cout << "请输入选用相机编号" << std::endl;
	/*std::string num;
	std::vector<Pylon::String_t>::const_iterator itfind = serialNumbers.begin();
	int deviceNum = 0;
	while (true)
	{
		std::cin >> num;
		if ((itfind = std::find(std::begin(serialNumbers),
			std::end(serialNumbers), num)) != serialNumbers.end())
		{
			deviceNum = std::distance(std::begin(serialNumbers), itfind);
			break;
		}
		else
		{
			std::cout << "没有与该序列号对应的相机" << std::endl;
			continue;
		}
	}*/
	int DeviceNum = 0;
	while (1)
	{
		std::cin >> DeviceNum;
		if (DeviceNum >= 0 && DeviceNum < DevicesNumber)
		{
			break;
		}
		else
		{
			std::cout << "没有与该数字对应的相机" << std::endl;
			continue;
		}
	}
	return DeviceNum;
}
bool sparkDect::WhetherProgContinue(void)
{
	if (CheckFreeSpace())
	{
		std::cout << "still free space : ";
		std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(3)
			<< FreeSpaceGB;
		std::cout << " GB " << std::endl;
		if (FreeSpaceGB <= 1.00 + eps)
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;

}
std::string sparkDect::GetStoragePath()
{
	return StoragePath + "\\";
}
