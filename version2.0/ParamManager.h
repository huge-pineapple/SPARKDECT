#pragma once

#include <Windows.h>
#include <pylon/PylonIncludes.h>

class ParamManager
{
public:
	/*inline void getWidth(const __int64& _Width)
	{
		Width = _Width;
	}
	inline __int64 getWidthValue() const
	{
		return Width;
	}
	inline void getHeight(const __int64& _Height)
	{
		Height = _Height;
	}
	inline __int64 getHeightValue() const
	{
		return Height;
	}*/
	inline void getExposureTime(const float& _ExposureTime)
	{
		ExposureTime = _ExposureTime;
	}
	inline float getExposureTimeValue() const
	{
		return ExposureTime;
	}
	inline void getMaxET(const float& _MaxET)
	{
		MaxExposureTime = _MaxET;
	}
	inline float getMaxETValue() const
	{
		return MaxExposureTime;
	}
	inline void getMinET(const float& _MinET)
	{
		MinExposureTime = _MinET;
	}
	inline float getMinETValue() const
	{
		return MinExposureTime;
	}
	inline void getETinc(const float& _ETinc)
	{
		ETinc = _ETinc;
	}
	inline float getETincValue() const
	{
		return ETinc;
	}
	inline void getExposureTimeRaw(const __int64& _ETR, const int& index)
	{
		ExposureTimeRaw[index] = _ETR;
	}
	inline __int64 getExposureTimeRawValue(const int &index) const
	{
		return ExposureTimeRaw[index];
	}
	inline void getMaxETR(const __int64& _maxETR)
	{
		MaxExposureTimeRaw = _maxETR;
	}
	inline __int64 getMaxETRValue() const
	{
		return MaxExposureTimeRaw;
	}
	inline void getMinETR(const __int64& _minETR)
	{
		MinExposureTimeRaw = _minETR;
	}
	inline __int64 getMinETRValue() const
	{
		return MinExposureTimeRaw;
	}
	inline void getETRinc(const __int64& _ETRinc)
	{
		ETRinc = _ETRinc;
	}
	inline __int64 getETRincValue() const
	{
		return ETRinc;
	}
	bool WhetherSetGain();
	bool WhetherSetGainRaw();
	//bool WhetherSetGamma();
	bool WhetherSetET();
	bool WhetherSetETR();
	bool WhetherSetFrameRate();
	__int64 increaseETR();
	__int64 decreaseETR();
	__int64 increaseETR_2();
	__int64 decreaseETR_2();
private:
	float Gain = 0;
	__int64 GainRaw = 0;

	float ExposureTime = 0;
	float MaxExposureTime = 0;
	float MinExposureTime = 0;
	float ETinc = 1;

	__int64 ExposureTimeRaw[2] = { 0, 0 };
	__int64 MaxExposureTimeRaw = 0;
	__int64 MinExposureTimeRaw = 0;
	__int64 ETRinc = 1;

	double Gamma = 0;
	__int64 Width = 0;
	__int64 Height = 0;
	float FrameRate = 0;
};

