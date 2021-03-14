#include <cmath>

#include "ShiftRegister.hpp"

void ShiftRegister::setClockShift(const unsigned char& shift) { 
	nClockShift = shift; 
}

void ShiftRegister::setDivisor(const unsigned char& divisor) { 
	switch(divisor){
	case 0:
		nDivisor = 8;
		break;
	case 1:
		nDivisor = 16;
		break;
	case 2:
		nDivisor = 32;
		break;
	case 3:
		nDivisor = 48;
		break;
	case 4:
		nDivisor = 64;
		break;
	case 5:
		nDivisor = 80;
		break;
	case 6:
		nDivisor = 96;
		break;
	case 7:
		nDivisor = 112;
		break;
	default:
		break;
	}
}

void ShiftRegister::updatePhase(){
	nPeriod = (nClockShift < 14 ? nDivisor << nClockShift : 0);
}

unsigned char ShiftRegister::sample(){
	return ((reg & 0x1) == 0x1 ? 0x0 : volume()); // Inverted
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
		volume.clock();
	}
}

void ShiftRegister::rollover(){
	// Xor the two lowest bits
	updatePhase();
	reload(); // Reset period counter
	if(((reg & 0x1) == 0x1) ^ ((reg & 0x2) == 0x2)){ // 1
		reg = reg >> 1; // Right shift all bits
		reg |= 0x4000; // Set the high bit (14)
		if(bWidthMode)
			reg |= 0x40; // Also set bit 6
	}
	else{ // 0
		reg = reg >> 1; // Right shift all bits
		reg &= 0xbfff; // Clear the high bit (14)
		if(bWidthMode)
			reg &= 0xffbf; // Also clear bit 6
	}
}

void ShiftRegister::trigger(const unsigned int& nTicks){
	updatePhase();
	this->reload(); // Reload the main timer with its phase
	reg = 0x7fff; // Set all 15 bits to 1
	length.trigger();
	volume.trigger();
	if((nTicks % 8) == 6) // If the next sequencer tick will clock the volume envelope, its timer counter is increased by one.
		volume.addExtraClock();
}

void ShiftRegister::userReset(){
	//length.reset(); // Length is reset by AudioUnit::reset()
	volume.reset();
	bWidthMode = false;
	nClockShift = 0;
	nDivisor = 0;
	reg = 0x7fff;
}

