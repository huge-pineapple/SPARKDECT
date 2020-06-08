#include "ParamManager_Mul.h"

__int64 ParamManager_Mul::increaseETR()
{
	__int64 num = ExposureTimeRaw[0] + ETRinc * 100;
	if (num < MaxExposureTimeRaw && num > MinExposureTimeRaw)
	{
		ExposureTimeRaw[0] += ETRinc * 100;
	}
	return ExposureTimeRaw[0];
}
__int64 ParamManager_Mul::increaseETR_2()
{
	__int64 num = ExposureTimeRaw[1] + ETRinc * 100;
	if (num < MaxExposureTimeRaw && num > MinExposureTimeRaw)
	{
		ExposureTimeRaw[1] += ETRinc * 100;
	}
	return ExposureTimeRaw[1];
}
__int64 ParamManager_Mul::decreaseETR()
{
	__int64 num = ExposureTimeRaw[0] - ETRinc * 100;
	if (num < MaxExposureTimeRaw && num > MinExposureTimeRaw)
	{
		ExposureTimeRaw[0] -= ETRinc * 100;
	}
	return ExposureTimeRaw[0];
}
__int64 ParamManager_Mul::decreaseETR_2()
{
	__int64 num = ExposureTimeRaw[1] - ETRinc * 100;
	if (num < MaxExposureTimeRaw && num > MinExposureTimeRaw)
	{
		ExposureTimeRaw[1] -= ETRinc * 100;
	}
	return ExposureTimeRaw[1];
}
bool ParamManager_Mul::WhetherSetGain()
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
bool ParamManager_Mul::WhetherSetGainRaw()
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
bool ParamManager_Mul::WhetherSetET()
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
bool ParamManager_Mul::WhetherSetETR()
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
bool ParamManager_Mul::WhetherSetFrameRate()
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