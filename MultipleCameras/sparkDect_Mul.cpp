#include "sparkDect_Mul.h"

#include <iostream>
#include <string>
#include <io.h>

void sparkDect_Mul::SelectPath()
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
void sparkDect_Mul::GetDiskName()
{
	char drive[256], dir[256], fname[256], ext[256];
	errno_t pathError = _splitpath_s(StoragePath.c_str(), drive, dir, fname, ext);
	std::string DiskName = drive;
	//std::cout <<"DiskName: "<< DiskName << std::endl;
}
bool sparkDect_Mul::CheckFreeSpace()
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
double sparkDect_Mul::GetFreeGBytesAvailable()
{
	return FreeSpaceGB;
}
void sparkDect_Mul::SelectDevice(int& DevicesNumber)
{
	/*std::cout << "请输入选用相机台数" << std::endl;
	int NeedDeviceNum = 0;
	while (1)
	{
		std::cin >> NeedDeviceNum;
		if (NeedDeviceNum >= 0 && NeedDeviceNum <= DevicesNumber)
		{
			break;
		}
		else
		{
			MessageBoxA(NULL, "现有相机数量不能满足要求, 请重新输入！", "初始化错误", MB_OK + MB_ICONERROR);
			continue;
		}
	}*/
	while (1)
	{
		for (int i = 0; i < 2; i++)
		{
			std::cout << "请输入选用第" << i + 1 << "台相机编号" << std::endl;
			std::cin >> CamNum[i];
		}
		if (CamNum[0] >= 0 && CamNum[0] < DevicesNumber && CamNum[0] >= 0 && CamNum[0] < DevicesNumber)
		{
			break;
		}
		else
		{
			MessageBoxA(NULL, "没有与该编号对应的相机, 请重新输入！", "初始化错误", MB_OK + MB_ICONERROR);
			continue;
		}
	}
}
void sparkDect_Mul::GetCamName(const int& index, const Pylon::String_t &name)
{
	CamName[index] = name;
}
std::string sparkDect_Mul::GetStoragePath()
{
	return StoragePath + "\\";
}
bool sparkDect_Mul::WhetherProgContinue()
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