#include "Setting.h"

#include <iostream>
#include <string>
#include <vector>
#include <io.h>
//#include <algorithm>

/*bool descendSort(std::vector<cv::Point>& a, std::vector<cv::Point>& b)
{
	return a.size() > b.size();
}*/

bool cmp(const std::pair<std::string, FILETIME>& a, const std::pair<std::string, FILETIME>& b)
{
	int ret = CompareFileTime(&a.second, &b.second);
	if (ret == 1)
	{
		return false;
	}
	return true;
}

void Setting::SelectPath()
{	
	std::cout << "input path for saving picture or video " << std::endl;
	std::cout << "press \"Enter\" to choose default path : " << StoragePath<< std::endl;
	while (true)
	{
		
		std::getline(std::cin, StoragePath);


		const char* c_PATH = StoragePath.c_str();

		if (CreateDirectoryA(c_PATH, NULL))
		{
			std::cout << "Storage path: " << '\n' << StoragePath << std::endl;
			break;
		}
		else if (StoragePath == "")
		{
			StoragePath = "D:";
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
				StoragePath = "";
				continue;
			}
		}
	}
}
void Setting::GetDiskName()
{
	char drive[256], dir[256], fname[256], ext[256];
	errno_t pathError = _splitpath_s(StoragePath.c_str(), drive, dir, fname, ext);
	DiskName = drive;
	std::cout <<"DiskName: "<< DiskName << std::endl;
}
bool Setting::CheckFreeSpace()
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
double Setting::GetFreeGBytesAvailable()
{
	return FreeSpaceGB;
}
bool Setting::SelectDevice(int& DevicesNumber)
{	
	std::cout << "请输入需要使用的相机数量: " << std::endl;
	std::vector<int>::iterator it;
	while (1)
	{
		std::cin >> TotalCamNum;
		if (TotalCamNum > DevicesNumber || TotalCamNum <= 0)
		{
			return false;
		}
		if (TotalCamNum <= DevicesNumber)
		{
			break;
		}
		else
		{
			MessageBoxA(NULL, " 请输入需要使用的相机数量！", "初始化错误", MB_OK + MB_ICONERROR);
			continue;
		}
	}
	while (1)
	{	
		it = CameraNumber.begin();
		int count = 0;
		int num = 0;
		for (int i = 0; i < TotalCamNum; i++)
		{	
			std::cout << "请输入选用第"<< i+1<<"台相机编号" << std::endl;
			std::cin >> num;
			it = std::find(CameraNumber.begin(), CameraNumber.end(), num);
			if (it != CameraNumber.end())
			{
				MessageBoxA(NULL, "没有与该编号对应的相机, 请重新输入！", "初始化错误", MB_OK + MB_ICONERROR);
				break;
			}
			if (num < 0 || num > DevicesNumber)
			{	
				MessageBoxA(NULL, "没有与该编号对应的相机, 请重新输入！", "初始化错误", MB_OK + MB_ICONERROR);
				break;
			}
			CameraNumber[i] = num;
			count++;
		}
		if (count == TotalCamNum)
		{
			break;
		}
	}	
	return true;
}
void Setting::GetDevicesID(const int& index, const Pylon::String_t &ID)
{
	DevicesID[index] = ID;
}
std::string Setting::GetStoragePath()
{
	return StoragePath + "\\";
}
void Setting::WhetherProgContinue()
{
	if (CheckFreeSpace())
	{
		std::cout << "still free space : ";
		std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(3)
			<< FreeSpaceGB;
		std::cout << " GB " << std::endl;
		if (FreeSpaceGB <= 1 + eps)
		{	
			std::cerr << "the disk doesn't have enough space" << std::endl;
			DeleteOldVideo();
			WhetherProgContinue();
		}
	}
	else
	{	
		std::cerr << "Check disk space failed" << std::endl;
	}
}
void Setting::DeleteOldVideo()
{
	std::vector<std::pair<std::string, FILETIME>> VideoVec;
	HANDLE hFind;
	WIN32_FIND_DATAA FindData;

	hFind = FindFirstFileA((GetStoragePath()+"*.mp4").c_str(), &FindData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::cout << "Failed to find first file!" << std::endl;
		return ;
	}
	do
	{
		// 忽略"."和".."两个结果 
		if (strcmp(FindData.cFileName, ".") == 0 || strcmp(FindData.cFileName, "..") == 0)
			continue;
		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)    // 是否是目录 
		{
			std::cout << FindData.cFileName << "\t<dir>\n";
		}
		else
		{
			std::string Str_FileName = FindData.cFileName;
			VideoVec.push_back(std::pair<std::string, FILETIME>(Str_FileName, FindData.ftLastWriteTime));
		}
	} while (FindNextFileA(hFind, &FindData));
	std::sort(VideoVec.begin(), VideoVec.end(), cmp);
	std::vector<std::pair<std::string, FILETIME>>::iterator it = VideoVec.begin();
	for (; it != VideoVec.end(); it++)
	{
		std::cout << it->first << std::endl;
	}
	std::cout << "删除 " << GetStoragePath() + VideoVec[0].first << std::endl;
	DeleteFileA((GetStoragePath() + VideoVec[0].first).c_str());
}
