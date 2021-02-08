#include "FrequencySweep.hpp"

/** Compute the two's complement of an unsigned 11-bit integer
  */
short twosComp11bit(const unsigned short &n){
	short retval = (short)n;
	if(n & 0x400) // > 1023 (negative value)
		retval = (short)(n - 0x800);		
	else // <= 1023 (positive value, so simply negate it)
		retval *= -1;
	return retval;
}

bool FrequencySweep::setNegate(const bool& negate) { 
	if(bNegateModeUsed && !negate){ // Negative to positive, channel will be disabled
		bNegate = negate;
		return false;
	}
	bNegate = negate; 
	bNegateModeUsed = false; // Reset used flag
	return true;
}

void FrequencySweep::setPeriod(const unsigned short& period) {
	nPeriod = period; 
	if(!nCounter) // Handle going from period = 0 to period != 0 with zero counter
		reload(); // Reload the timer period
}

void FrequencySweep::trigger(){
	nShadowFrequency = extTimer->getFrequency();
	reload(); // Reset period counter
	// Reset flags
	bOverflow = false; 
	bOverflow2 = false;
	bNegateModeUsed = false;
	if(nPeriod || nShift)
		bEnabled = true;
	else
		bEnabled = false;
	if(nShift){
		if(!compute()){ // Overflow
			bOverflow = true;
		}
	}
}

bool FrequencySweep::compute(){
	// Compute new frequency (f')
	if(!bNegate)
		nNewFrequency = (nShadowFrequency >> nShift) + nShadowFrequency;
	else{
		nNewFrequency = nShadowFrequency + twosComp11bit(nShadowFrequency >> nShift);
		bNegateModeUsed = true;
	}
	return (nNewFrequency <= 2047); // Overflow check
}

void FrequencySweep::reload(){
	nCounter = (nPeriod != 0 ? nPeriod : 8); // Reset period counter
}

void FrequencySweep::rollover(){
	reload(); // Reset period counter
	if(nPeriod){ // Only compute new frequency with non-zero period
		if(compute()){ // Compute new frequency
			// Update shadow frequency
			if(nShift){ // If new frequency <= 2047 and shift is non-zero, update the frequency
				nShadowFrequency = nNewFrequency;
				extTimer->setFrequency(nShadowFrequency);
			}
			if(!compute()){ // Compute new frequency again immediately
				bOverflow2 = true;
			}
		}
		else{ // Frequency overflow, disable channel
			bOverflow = true;
			disable();
		}
	}
}

