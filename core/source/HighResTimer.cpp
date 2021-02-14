
#include "HighResTimer.hpp"

HighResTimer::HighResTimer() :
	tInitialization(hrclock::now()),
	tLastStart()
{
}

void HighResTimer::start(){
	tLastStart = hrclock::now();
}

double HighResTimer::stop() const {
	return std::chrono::duration<double>(hrclock::now() - tLastStart).count();
}

double HighResTimer::uptime() const {
	return std::chrono::duration<double>(hrclock::now() - tInitialization).count();
}

void HighResTimer::reset(){
	tInitialization = hrclock::now();
}

