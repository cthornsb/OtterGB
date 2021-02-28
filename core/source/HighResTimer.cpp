
#include "HighResTimer.hpp"

HighResTimer::HighResTimer() :
	dTotalTime(0),
	nStops(0),
	tInitialization(hrclock::now()),
	tLastStart()
{
}

void HighResTimer::start(){
	tLastStart = hrclock::now();
}

double HighResTimer::stop(){
	nStops++;
	double retval = std::chrono::duration<double>(hrclock::now() - tLastStart).count();
	dTotalTime += retval;
	return retval;
}

double HighResTimer::uptime() const {
	return std::chrono::duration<double>(hrclock::now() - tInitialization).count();
}

void HighResTimer::reset(){
	dTotalTime = 0;
	nStops = 0;
}

