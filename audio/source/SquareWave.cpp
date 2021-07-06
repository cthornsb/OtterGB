#include "SquareWave.hpp"
#include "FrequencySweep.hpp"

SquareWave::SquareWave(FrequencySweep* sweep) :
	AudioUnit(),
	bSweepEnabled(true),
	bFrequencyUpdated(false),
	nDuty(0),
	nDutyStep(1),
	nWaveform(0xf0), // 50% duty
	volume(),
	frequency(sweep)
{
	frequency->setUnitTimer(this);
	nPeriodMultiplier = 4;
}

void SquareWave::setWaveDuty(const unsigned char& duty){
	switch(duty){
	case 0: // 12.5%
		nWaveform = 0x80;
		break;
	case 1: // 25%
		nWaveform = 0xc0;
		break;
	case 2: // 50%
		nWaveform = 0xf0;
		break;
	case 3: // 75%
		nWaveform = 0xfc;
		break;
	default: // Invalid duty cycle
		return;
	};
	nDuty = duty;
}

unsigned char SquareWave::sample(){
	return ((nWaveform & nDutyStep) == nDutyStep ? volume() : 0x0);
}

void SquareWave::clockSequencer(const unsigned int& sequencerTicks){
	// (Sweep ->) Timer -> Duty -> Length Counter -> Envelope -> Mixer
	//if(!bEnabled) // DAC powered down
	//	return; // Do not return because modulators can still be clocked while DAC powered off
	bFrequencyUpdated = false;
	if(bSweepEnabled && sequencerTicks % 4 == 2){ // Clock the frequency sweep (128 Hz)
		if(frequency->clock()){
			// Frequency sweep rolled over
			if(!frequency->overflowed() && !frequency->overflowed2()){ 
				// Get new timer period and update the channel period and frequency registers
				this->setFrequency(frequency->getNewFrequency()); // P = (2048 - f)
				bFrequencyUpdated = true;		
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
		volume.clock();
	}
}

void SquareWave::rollover(){
	// Update square wave duty waveform
	reload(); // Reset period counter
	if (nDutyStep < 0x80)
		nDutyStep = nDutyStep << 1;
	else // Reset to bit 0
		nDutyStep = 0x1;
}

void SquareWave::trigger(const unsigned int& nTicks){
	this->reload(); // Reload the main timer with its phase
	if(bSweepEnabled)
		frequency->trigger();
	length.trigger();
	volume.trigger();
}

void SquareWave::userEnable(){
	volume.enable();
}

void SquareWave::userDisable(){
	volume.disable();
}

void SquareWave::userReset(){
	//length.reset(); // Length is reset by AudioUnit::reset()
	volume.reset();
	if(bSweepEnabled)
		frequency->reset();	
	nDutyStep = 1; // Pulse duty step index is only reset when APU is powered off
	setWaveDuty(nDuty); // Reset duty waveform to starting position
}

void SquareWave::channelWillBeEnabled(){
	if(bSweepEnabled && frequency->overflowed()){
		bDisableThisChannel = true; // Disable channel
		this->disable(); // Disable DAC
	}
}
	
