#include "AudioUnit.hpp"

bool AudioUnit::pollDisable(){
	if(bDisableThisChannel){
		bDisableThisChannel = false;
		return true;
	}
	return false;
}

bool AudioUnit::pollEnable(){
	if(bEnableThisChannel){
		bEnableThisChannel = false;
		return true;
	}
	return false;
}

bool AudioUnit::powerOn(const unsigned char& nrx4, const unsigned int& nSequencerTicks){
	bool bLengthEnable = ((nrx4 & 0x40) == 0x40); // bit 6
	bool bTrigger = ((nrx4 & 0x80) == 0x80); // bit 7
	if(bLengthEnable){ // Enable length counter
		if(length.extraClock(nSequencerTicks, bTrigger)){ // Extra clock rolled over the length counter
			bDisableThisChannel = true; // Disable channel
			this->disable(); // Disable DAC
		}
		length.enable();
	}
	else{
		length.disable();
	}
	if(bTrigger){ // Trigger
		this->trigger();
		if(bLengthEnable && length.isEnabled() && length.getLength()){
			// Triggering with a length counter of non-zero length.
			// Ensure that the DAC is powered up.
			this->enable();
		}
		if(bEnabled){ // Only enable if DAC powered on
			bEnableThisChannel = true;
			length.extraClock(nSequencerTicks); // Extra clock upon triggering
			this->channelWillBeEnabled(); // Channel is going to be enabled, do whatever else needs to be done beforehand
		}
	}
	return (bDisableThisChannel || bEnableThisChannel);
}

void AudioUnit::reset(){
	// Timer values & flags
	bEnabled = false;
	nPeriod = 0;
	nCounter = 0;
	nCyclesSinceLastClock = 0;
	// DAC values & flags
	bDisableThisChannel = false;
	bEnableThisChannel = false;
	length.reset();
	// Inherited classes
	this->userReset();
}

