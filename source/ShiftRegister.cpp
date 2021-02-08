#include <cmath>

#include "ShiftRegister.hpp"

void ShiftRegister::updatePhase(){
	if(nDivisor != 0)
		nPeriod = std::pow(2, nClockShift + 1) / nDivisor;
	else // For divisor=0, assume divisor=0.5
		nPeriod = std::pow(2, nClockShift + 1) / 2;
}

float ShiftRegister::getRealFrequency() const {
	if(nDivisor == 0) // If divisor=0, use divisor=0.5
		return (1048576.f / std::pow(2.f, nClockShift + 1)); // in Hz
	return (524288.f / nDivisor / std::pow(2.f, nClockShift + 1)); // in Hz
}

unsigned char ShiftRegister::sample(){
	return (((reg & 0x1) == 0x1 ? 0x0 : 0xf) * volume()); // Inverted
}

void ShiftRegister::clockSequencer(const unsigned int& sequencerTicks){
	// Timer -> LFSR -> Length Counter -> Envelope -> Mixer
	//if(!bEnabled) // DAC powered down
	//	return; // Do not return because modulators can still be clocked while DAC powered off
	if(sequencerTicks % 2 == 0){ // Clock the length counter (256 Hz)
		if(length.clock()){
			// If length counter rolls over, disable the channel
			bDisableThisChannel = true;
		}
	}
	if(sequencerTicks % 8 == 7){ // Clock the volume envelope (64 Hz)
		if(volume.clock()){
			// If volume envelope counter rolls over, do nothing 
			// since the volume unit will simply output 0 volume
		}
	}
}

void ShiftRegister::rollover(){
	// Xor the two lowest bits
	reg = reg >> 1; // Right shift all bits
	if((reg & 0x1) ^ ((reg & 0x2) >> 1)){ // 1
		reg |= 0x4000; // Set the high bit (14)
		if(bWidthMode)
			reg |= 0x40; // Also set bit 6
	}
	else{ // 0
		reg &= 0xbfff; // Clear the high bit (14)
		if(bWidthMode)
			reg &= 0xffbf; // Also clear bit 6
	}
}

void ShiftRegister::trigger(){
	if(!nCounter)
		this->reload(); // Reload the main timer with its phase
	reg = 0x7fff; // Set all 15 bits to 1
	length.trigger();
	volume.trigger();
}

void ShiftRegister::userReset(){
	//length.reset(); // Length is reset by AudioUnit::reset()
	volume.reset();
	bWidthMode = false;
	nClockShift = 0;
	nDivisor = 0;
	reg = 0x7fff;
}

