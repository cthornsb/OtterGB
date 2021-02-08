#include "LengthCounter.hpp"

bool LengthCounter::extraClock(const unsigned int& nTicks, bool bWillBeTriggered){
	if(nTicks % 2 != 0){
		// Extra length clocking may occur when enabled on a cycle
		// which clocks the length counter.
		if(!bEnabled && nCounter != 0){
			// Length counter is going from disabled to enabled with a non-zero length
			// so we need to clock the counter an extra time.
			this->enable();
			if(this->clock() && !bWillBeTriggered){
				// If the extra clock caused the counter to roll over, and bit 7 is clear, disable the channel
				return true;
			}
		}
	}
	return false;
}

bool LengthCounter::extraClock(const unsigned int& nTicks){
	if((nTicks % 2 != 0) && bEnabled && bRefilled){
		this->clock();
		return true;
	}
	return false;
}

void LengthCounter::trigger(){
	if(!nCounter){
		reload(); // Reload length counter with maximum length
		bRefilled = true;
	}
	else
		bRefilled = false;
}

void LengthCounter::reset(){
	// Do not set period to zero because it is used to compute the length
	bEnabled = false;
	bRefilled = false;
	nCounter = 0;
	nCyclesSinceLastClock = 0;
}

