#include "WaveTable.hpp"

unsigned char WaveTable::sample(){
	unsigned char retval;
	// Retrieve a 4-bit sample
	if(nIndex % 2 == 0) // High nibble
		retval = (nBuffer & 0xf0) >> 4;
	else // Low nibble
		retval = nBuffer & 0xf;
	switch(nVolume){
	case 0: // Mute
		break;
	case 1: // 100% volume
		return retval;
	case 2: // 50% volume (shift 1 bit)
		return (retval >> 1);
	case 3: // 25% volume (shift 2 bits)
		return (retval >> 2);
	default: // Mute
		break;
	};
	return 0;
}

void WaveTable::clockSequencer(const unsigned int& sequencerTicks){
	// Timer -> Wave -> Length Counter -> Volume -> Mixer
	//if(!bEnabled) // DAC powered down
	//	return; // Do not return because modulators can still be clocked while DAC powered off
	if(sequencerTicks % 2 == 0){ // Clock the length counter (256 Hz)
		if(length.clock()){
			// If length counter rolls over, disable the channel
			bDisableThisChannel = true;
		}
	}
}

void WaveTable::trigger(){
	// Reload the main timer with its phase
	if(!nCounter)
		this->reload(); 
	// Trigger length counter
	length.trigger();
	// Reset position counter but do not sample the first byte due to a quirk of the hardware.
	// The first nibble of the wave table will not be played until it loops
	nIndex = 0; 
}

void WaveTable::rollover(){
	reload(); // Reset period counter
	if(nIndex % 2 == 0) // Get the next two nibbles
		nBuffer = data[nIndex / 2];
	if(++nIndex >= 32) // Increment sample position
		nIndex = 0;
}

void WaveTable::userEnable(){
	length.enable();
}

void WaveTable::userDisable(){
	length.disable();
}

void WaveTable::userReset(){
	//length.reset(); // Length is reset by AudioUnit::reset()
	nIndex = 0;
	nBuffer = 0;
	nVolume = 0; // muted
}
