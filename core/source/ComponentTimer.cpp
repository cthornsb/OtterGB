
#include "ComponentTimer.hpp"

bool ComponentTimer::clock(){
	if(!bEnabled || nPeriod == 0)
		return false;
	if(++nCyclesSinceLastTick == nPeriod){
		this->rollover();
		nCounter++;
		return true;
	}
	return false;
}

