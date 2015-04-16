#include "PerfMon.h"
#include <VrLib/Log.h>
namespace vrlib
{
	PerfMon::PerfMon()
	{
		LARGE_INTEGER li;
		if (!QueryPerformanceFrequency(&li))
			logger << "Error with performance counter" << Log::newline;
		frequency = double(li.QuadPart) / 1000.0;
		resetTimer();
	}


	PerfMon::~PerfMon()
	{
	}

	double PerfMon::getTime()
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return double(li.QuadPart - startTime) / frequency;
	}

	void PerfMon::resetTimer()
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		startTime = li.QuadPart;
	}
}