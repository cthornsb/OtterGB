#include "UnitTimer.hpp"

bool UnitTimer::clock(){
	if(!bEnabled || !nCounter)
		return false;
	if(--nCounter == 0){ // Decrement the timer
		rollover();
		return true; // The timer rolled over
	}
	return false;
}

void UnitTimer::reload(){
	nCounter = nPeriod;
}

