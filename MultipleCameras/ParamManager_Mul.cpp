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
	int ret = MessageBoxA(NULL, " �Ƿ�ı� Gain ��ֵ, ��'ȡ��'�˳����� ", "���������ʼ��", MB_YESNOCANCEL + MB_ICONQUESTION);
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
	int ret = MessageBoxA(NULL, " �Ƿ�ı� GainRaw ��ֵ, ��'ȡ��'�˳����� ", "���������ʼ��", MB_YESNOCANCEL + MB_ICONQUESTION);
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
	int ret = MessageBoxA(NULL, " �Ƿ�ı� ExposureTime ��ֵ, ��'ȡ��'�˳����� ", "���������ʼ��", MB_YESNOCANCEL + MB_ICONQUESTION);
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
	int ret = MessageBoxA(NULL, " �Ƿ�ı� ExposureTimeRaw ��ֵ, ��'ȡ��'�˳����� ", "���������ʼ��", MB_YESNOCANCEL + MB_ICONQUESTION);
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
	int ret = MessageBoxA(NULL, " �Ƿ�ı� FrameRate ��ֵ, ��'ȡ��'�˳����� ", "���������ʼ��", MB_YESNOCANCEL + MB_ICONQUESTION);
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