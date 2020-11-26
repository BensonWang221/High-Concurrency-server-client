/*==================================================================================================
   Date                        Description of Change
18-Nov-2020                利用std::chrono实现高精度计时器

$$HISTORY$$
====================================================================================================*/
#ifndef _CELL_TIME_STAMP_INCLUDED
#define _CELL_TIME_STAMP_INCLUDED

#include <chrono>

using namespace std::chrono;

class CELLTimestamp
{
public:
	CELLTimestamp() : _begin(high_resolution_clock::now()) {}

	void Update()
	{
		_begin = high_resolution_clock::now();
	}

	constexpr double GetElapsedTimeInSecond()
	{
		return GetElapsedTimeInMicroSec() * 0.000001;
	}

	constexpr double GetElapsedTimeInMilliroSec()
	{
		return GetElapsedTimeInMicroSec() * 0.001;
	}

	constexpr long long GetElapsedTimeInMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

private:
	time_point<high_resolution_clock> _begin;
};

#endif