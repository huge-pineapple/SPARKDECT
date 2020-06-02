#include "ParamManager.h"
#include <Windows.h>
#include <string>
#include <pylon/PylonIncludes.h>

__int64 ParamManager::increaseETR()
{
	__int64 num = ExposureTimeRaw + ETRinc * 100;
	if (num < MaxExposureTimeRaw && num > MinExposureTimeRaw)
	{
		ExposureTimeRaw += ETRinc * 100;
	}
	return ExposureTimeRaw;
}
__int64 ParamManager::decreaseETR()
{
	__int64 num = ExposureTimeRaw - ETRinc * 100;
	if (num < MaxExposureTimeRaw && num > MinExposureTimeRaw)
	{
		ExposureTimeRaw -= ETRinc * 100;
	}
	return ExposureTimeRaw;
}
bool ParamManager::WhetherSetGain()
{
	int ret = MessageBoxA(NULL, " 是否改变 Gain 的值, 按'取消'退出程序 ", "相机参数初始化", MB_YESNOCANCEL + MB_ICONQUESTION);
	if (ret == 6)
	{
		return true;
	}
	else if (ret == 7)
	{
		return false;
	}
	else
	{
		Pylon::PylonTerminate();
		exit(1);
	}
}
bool ParamManager::WhetherSetGainRaw()
{
	int ret = MessageBoxA(NULL, " 是否改变 GainRaw 的值, 按'取消'退出程序 ", "相机参数初始化", MB_YESNOCANCEL + MB_ICONQUESTION);
	if (ret == 6)
	{
		return true;
	}
	else if (ret == 7)
	{
		return false;
	}
	else
	{
		Pylon::PylonTerminate();
		exit(1);
	}
}
/*bool ParamManager::WhetherSetGamma()
{
	char flag = 'n';
	std::cout << "whether change the value of Gamma or not ? (y/n) " << std::endl;
	while (true)
	{
		std::cin >> flag;
		if (flag == 'n' || flag == 'N')
		{
			return false;
		}
		else if (flag == 'y' || flag == 'Y')
		{
			return true;
		}
		else
		{
			std::cout << "whether change the value of Gamma or not ? (y/n) " << std::endl;
			continue;
		}
	}
}*/
bool ParamManager::WhetherSetET()
{
	int ret = MessageBoxA(NULL, " 是否改变 ExposureTime 的值, 按'取消'退出程序 ", "相机参数初始化", MB_YESNOCANCEL + MB_ICONQUESTION);
	if (ret == 6)
	{
		return true;
	}
	else if (ret == 7)
	{
		return false;
	}
	else
	{
		Pylon::PylonTerminate();
		exit(1);
	}
}
bool ParamManager::WhetherSetETR()
{
	int ret = MessageBoxA(NULL, " 是否改变 ExposureTimeRaw 的值, 按'取消'退出程序 ", "相机参数初始化", MB_YESNOCANCEL + MB_ICONQUESTION);
	if (ret == 6)
	{
		return true;
	}
	else if (ret == 7)
	{
		return false;
	}
	else
	{
		Pylon::PylonTerminate();
		exit(1);
	}
}
bool ParamManager::WhetherSetFrameRate()
{
	int ret = MessageBoxA(NULL, " 是否改变 FrameRate 的值, 按'取消'退出程序 ", "相机参数初始化", MB_YESNOCANCEL + MB_ICONQUESTION);
	if (ret == 6)
	{
		return true;
	}
	else if (ret == 7)
	{
		return false;
	}
	else
	{
		Pylon::PylonTerminate();
		exit(1);
	}
}