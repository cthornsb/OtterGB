#include "SquareWave.hpp"
#include "FrequencySweep.hpp"
#include "SystemRegisters.hpp"

SquareWave::SquareWave(FrequencySweep* sweep) :
	AudioUnit(),
	bSweepEnabled(true),
	nWaveform(0),
	volume(),
	frequency(sweep)
{
	frequency->setUnitTimer(this);
	nPeriodMultiplier = 4;
}

void SquareWave::setWaveDuty(const unsigned char& duty){
	switch(duty){
	case 0:
		nWaveform = 0x01;
		break;
	case 1:
		nWaveform = 0x81;
		break;
	case 2:
		nWaveform = 0x87;
		break;
	case 3:
		nWaveform = 0x7e;
		break;
	default:
		break;
	};
}

unsigned char SquareWave::sample(){
	return ((nWaveform & 0x1) == 0x1 ? 0xf : 0x0);
}

void SquareWave::clockSequencer(const unsigned int& sequencerTicks){
	// (Sweep ->) Timer -> Duty -> Length Counter -> Envelope -> Mixer
	//if(!bEnabled) // DAC powered down
	//	return; // Do not return because modulators can still be clocked while DAC powered off
	if(bSweepEnabled && sequencerTicks % 4 == 2){ // Clock the frequency sweep (128 Hz)
		if(frequency->clock()){
			// Frequency sweep rolled over
			if(!frequency->overflowed() && !frequency->overflowed2()){ 
				// Get new timer period and update the channel period and frequency registers
				unsigned short freq = frequency->getNewFrequency();
				this->setFrequency(freq); // P = (2048 - f)
				rNR13->setValue((unsigned char)(freq & 0x00FF));
				rNR14->setBits(0, 2, (unsigned char)((freq & 0x0700) >> 8));				
			}
			else{ // Frequency overflowed, disable channel
				bDisableThisChannel = true;
			}
		}
	}
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

void SquareWave::rollover(){
	// Update square wave duty waveform
	bool lowBit = ((nWaveform & 0x1) == 0x1);
	nWaveform = nWaveform >> 1;
	if(lowBit) // Set high bit
		nWaveform |= 0x80;
	else // Clear high bit
		nWaveform &= 0x7f;
}

void SquareWave::trigger(){
	if(!nCounter)
		this->reload(); // Reload the main timer with its phase
	if(bSweepEnabled)
		frequency->trigger();
	length.trigger();
	volume.trigger();
}

void SquareWave::userEnable(){
	// Do not automatically enable the length counter or frequency sweep,
	// they must be enabled independently by writing to APU registers.
	volume.enable();
}

void SquareWave::userDisable(){
	length.disable();
	volume.disable();
	if(bSweepEnabled)
		frequency->disable();
}

void SquareWave::channelWillBeEnabled(){
	if(bSweepEnabled && frequency->overflowed()){
		bDisableThisChannel = true; // Disable channel
		this->disable(); // Disable DAC
	}
}
	
