
#include <iostream>
#include <math.h>

#include "Support.hpp"
#include "SystemGBC.hpp"
#include "Sound.hpp"

bool LengthCounter::onClockUpdate(){
	if(!bEnabled)
		return false;
	if(nCounter == 1){ // Disable the channel
		nCounter = 0;
		return true;
	}
	nCounter--;
	return false;
}

void LengthCounter::onTriggerEvent(){
	if(!nCounter)
		nCounter = nMaximum;
}

bool VolumeEnvelope::onClockUpdate(){
	if(!nPeriod)
		return false;
	if(bAdd){ // Add (louder)
		if(nVolume + 1 <= 15)
			nVolume++;
	}
	else{ // Subtract (quieter)
		if(nVolume > 0)
			nVolume--;
	}
}

void VolumeEnvelope::onTriggerEvent(){
	//Volume envelope timer is reloaded with period.
	//Channel volume is reloaded from NRx2.

	nCounter = nPeriod;
	//nVolume = ;
}
		
bool FrequencySweep::onClockUpdate(){
	if(!bEnabled)
		return false;
	nTimer = 0; // Refill?
	if(nPeriod || nShift)
		bEnabled = true;
	else
		bEnabled = false;
	if(!nShift)
		return false;
	// Compute new frequency (x)
	// f(x) = 131072 / (2048 - x)
	// x = 2048 - (131072 / f(x))
	nShadowFrequency = 2048 - (unsigned short)(131072.f / *frequency);
	unsigned short newFrequency = nShadowFrequency >> nShift;
	if(bNegate)
		newFrequency = ~newFrequency;
	newFrequency += nShadowFrequency;
	if(newFrequency > 2047){ // Overflow check
		bEnabled = false;
		return false;
	}
	nShadowFrequency = newFrequency;
	// Write new frequency to registers NR13-14
	*frequency = 131072.f / (2048 - nShadowFrequency);
	// Calculate frequency and check overflow again immediately, without writing back
	
}

void FrequencySweep::onTriggerEvent(){
	//Frequency timer is reloaded with period.
	nCounter = nPeriod;
}

SoundProcessor::SoundProcessor() : 
	SystemComponent("APU"), 
	ComponentTimer(2048), // 512 Hz sequencer
	audio(new SoundManager()),
	ch1FrequencySweep(),
	ch1SoundLengthData(64),
	ch1VolumeEnvelope(),
	ch1WavePatternDuty(0),
	ch1Frequency(0.f),
	ch1ToSO1(false),
	ch1ToSO2(false),
	ch2SoundLengthData(64),
	ch2VolumeEnvelope(),
	ch2WavePatternDuty(0),
	ch2Frequency(0.f),
	ch2ToSO1(false),
	ch2ToSO2(false),
	ch3SoundLengthData(256),
	ch3OutputLevel(0),
	ch3Frequency(0.f),
	ch3ToSO1(false),
	ch3ToSO2(false),
	ch4SoundLengthData(64),
	ch4VolumeEnvelope(),
	ch4ShiftClockFreq(0),
	ch4DividingRation(0),
	ch4DirectionEnv(false),
	ch4CounterStepWidth(false),
	ch4ToSO1(false),
	ch4ToSO2(false),
	outputLevelSO1(0),
	outputLevelSO2(0),
	wavePatternRAM(),
	masterSoundEnable(false),
	sequencerTicks(0)
{ 
	//audio->init();
	ch1FrequencySweep.setFrequency(&ch1Frequency);
	disableTimer();
}

bool SoundProcessor::checkRegister(const unsigned short &reg){
	// Check if the APU is powered down.
	// If the APU is powered down, only the APU control register and
	//  wave RAM may be written to.
	if(!masterSoundEnable && (reg < 0xFF30 || reg > 0xFF3F) && reg != 0xFF26) 
		return false;
	return true;
}

bool SoundProcessor::writeRegister(const unsigned short &reg, const unsigned char &val){
	unsigned short freq16;
	bool channelEnabled;
	switch(reg){
		case 0xFF10: // NR10 ([TONE] Channel 1 sweep register)
			ch1FrequencySweep.setPeriod(rNR10->getBits(4,6));
			ch1FrequencySweep.setNegate(rNR10->getBit(3));
			ch1FrequencySweep.setBitShift(rNR10->getBits(0,2));
			break;
		case 0xFF11: // NR11 ([TONE] Channel 1 sound length / wave pattern duty)
			ch1WavePatternDuty = rNR11->getBits(6,7); // Wave pattern duty (see below)
			ch1SoundLengthData.setLength(rNR11->getBits(0,5));
			break;
		case 0xFF12: // NR12 ([TONE] Channel 1 volume envelope)
			ch1VolumeEnvelope.setVolume(rNR12->getBits(4,7));
			ch1VolumeEnvelope.setAddMode(rNR12->getBit(3));
			ch1VolumeEnvelope.setPeriod(rNR12->getBits(0,2));
			if(!ch1VolumeEnvelope.getVolume()) // Disable channel with zero volume
				rNR52->resetBit(0); // Ch 1 OFF
			break;
		case 0xFF13: // NR13 ([TONE] Channel 1 frequency low)
			freq16 = (rNR14->getBits(0,2) << 8) + rNR13->getValue();
			ch1Frequency = 131072.0f/(2048-freq16); // Hz
			break;
		case 0xFF14: // NR14 ([TONE] Channel 1 frequency high)
			freq16 = (rNR14->getBits(0,2) << 8) + rNR13->getValue();
			ch1Frequency = 131072.0f/(2048-freq16); // Hz
			if(rNR14->getBit(6)){
				ch1SoundLengthData.enable();
				rNR52->setBit(0);
			}
			else{
				ch1SoundLengthData.disable();
			}
			if(rNR14->getBit(7)){
				ch1SoundLengthData.onTriggerEvent();
				ch1VolumeEnvelope.onTriggerEvent();
				ch1FrequencySweep.onTriggerEvent();
			}
			break;
		case 0xFF15: // Not used
			break;
		case 0xFF16: // NR21 ([TONE] Channel 2 sound length / wave pattern duty)
			ch2WavePatternDuty = rNR21->getBits(6,7); // Wave pattern duty (see below)
			ch2SoundLengthData.setLength(rNR21->getBits(0,5));
			break;
		case 0xFF17: // NR22 ([TONE] Channel 2 volume envelope
			ch2VolumeEnvelope.setVolume(rNR22->getBits(4,7));
			ch2VolumeEnvelope.setAddMode(rNR22->getBit(3));
			ch2VolumeEnvelope.setPeriod(rNR22->getBits(0,2));
			if(!ch2VolumeEnvelope.getVolume()) // Disable channel with zero volume
				rNR52->resetBit(1); // Ch 2 OFF
			break;
		case 0xFF18: // NR23 ([TONE] Channel 2 frequency low)
			freq16 = (rNR24->getBits(0,2) << 8) + rNR23->getValue();
			ch2Frequency = 131072.0f/(2048-freq16); // Hz
			break;
		case 0xFF19: // NR24 ([TONE] Channel 2 frequency high)
			freq16 = (rNR24->getBits(0,2) << 8) + rNR23->getValue();
			ch2Frequency = 131072.0f/(2048-freq16); // Hz
			if(rNR24->getBit(6)){
				ch2SoundLengthData.enable();
				rNR52->setBit(1);
			}
			else{
				ch2SoundLengthData.disable();
			}
			if(rNR24->getBit(7)){
				ch2SoundLengthData.onTriggerEvent();
				ch2VolumeEnvelope.onTriggerEvent();
			}
			break;
		case 0xFF1A: // NR30 ([TONE] Channel 3 sound on/off)
			if(!rNR30->getBit(7))
				rNR52->resetBit(2); // Ch 3 OFF
			break;
		case 0xFF1B: // NR31 ([WAVE] Channel 3 sound length)
			ch3SoundLengthData.setLength(rNR31->getValue());
			break;
		case 0xFF1C: // NR32 ([WAVE] Channel 3 select output level)
			ch3OutputLevel = rNR32->getBits(5,6); // Select output level (see below)
			break;
		case 0xFF1D: // NR33 ([WAVE] Channel 3 frequency low)
			freq16 = (rNR34->getBits(0,2) << 8) + rNR33->getValue();
			ch3Frequency = 65536.0f/(2048-freq16); // Hz
			break;
		case 0xFF1E: // NR34 ([WAVE] Channel 3 frequency high)
			freq16 = (rNR34->getBits(0,2) << 8) + rNR33->getValue();
			ch3Frequency = 65536.0f/(2048-freq16); // Hz
			if(rNR34->getBit(6)){
				ch3SoundLengthData.enable();
				rNR52->setBit(2);
			}
			else{
				ch3SoundLengthData.disable();
			}
			if(rNR34->getBit(7)){
				ch3SoundLengthData.onTriggerEvent();
			}
			break;		
		case 0xFF1F: // Not used
			break;
		case 0xFF20: // NR41 ([NOISE] Channel 4 sound length)
			ch4SoundLengthData.setLength(rNR41->getBits(0,5));
			break;
		case 0xFF21: // NR42 ([NOISE] Channel 4 volume envelope)
			ch4VolumeEnvelope.setVolume(rNR42->getBits(4,7));
			ch4VolumeEnvelope.setAddMode(rNR42->getBit(3));
			ch4VolumeEnvelope.setPeriod(rNR42->getBits(0,2));
			if(!ch4VolumeEnvelope.getVolume()) // Disable channel with zero volume
				rNR52->resetBit(3); // Ch 4 OFF
			break;
		case 0xFF22: // NR43 ([NOISE] Channel 4 polynomial counter)
			ch4ShiftClockFreq   = rNR43->getBits(4,7); // Shift clock frequency (s)
			ch4CounterStepWidth = rNR43->getBit (3); // Counter step/width [0: 15-bits, 1: 7-bits]
			ch4DividingRation   = rNR43->getBits(0,2); // Dividing ratio of frequency (r)
			break;
		case 0xFF23: // NR44 ([NOISE] Channel 4 counter / consecutive, initial)
			if(rNR44->getBit(6)){
				ch4SoundLengthData.enable();
				rNR52->setBit(3);
			}
			else{
				ch4SoundLengthData.disable();
			}
			if(rNR44->getBit(7)){
				ch4SoundLengthData.onTriggerEvent();
				ch4VolumeEnvelope.onTriggerEvent();
			}
			break;
		case 0xFF24: // NR50 (Channel control / ON-OFF / volume)
			// S01 and S02 terminals not emulated
			outputLevelSO2 = rNR50->getBits(4,6); // Output level (0-7)
			outputLevelSO1 = rNR50->getBits(0,2); // Output level (0-7)
			break;
		case 0xFF25: // NR51 (Select sound output)
			ch4ToSO2 = rNR51->getBit(7);
			ch3ToSO2 = rNR51->getBit(6);
			ch2ToSO2 = rNR51->getBit(5);
			ch1ToSO2 = rNR51->getBit(4);
			ch4ToSO1 = rNR51->getBit(3);
			ch3ToSO1 = rNR51->getBit(2);
			ch2ToSO1 = rNR51->getBit(1);
			ch1ToSO1 = rNR51->getBit(0);
			break;
		case 0xFF26: // NR52 (Sound ON-OFF)
			masterSoundEnable = rNR52->getBit(7);
			if(masterSoundEnable){ // Power on the frame sequencer
				//if (!audio->isRunning())
				//	audio->start(); // Start audio interface
				this->reset();
				enableTimer();
			}
			else{
				//if (audio->isRunning())
				//	audio->stop(); // Stop audio interface
				rNR52->resetBit(3); // Ch 4 OFF
				rNR52->resetBit(2); // Ch 3 OFF
				rNR52->resetBit(1); // Ch 2 OFF
				rNR52->resetBit(0); // Ch 1 OFF
				for(unsigned char i = 0x10; i < 0x30; i++){ // Clear all sound registers (except NR52 and wave RAM)
					if(i == 0x26) continue; // Skip NR52
					sys->clearRegister(i);
				}
				disableTimer();
			}
			break;
		default:
			if (reg >= 0xFF27 && reg <= 0xFF2F) { // Not used
			}
			else if (reg >= 0xFF30 && reg <= 0xFF3F) { // [Wave] pattern RAM
				// Contains 32 4-bit samples played back upper 4 bits first
				wavePatternRAM[reg - 0xFF30] = rWAVE[reg - 0xFF30]->getValue();
			}
			else {
				return false;
			}
			break;
	}
	return true;
}

bool SoundProcessor::readRegister(const unsigned short &reg, unsigned char &dest){
	switch(reg){
		case 0xFF10: // NR10 ([TONE] Channel 1 sweep register)
			dest |= 0x80;
			break;
		case 0xFF11: // NR11 ([TONE] Channel 1 sound length / wave pattern duty)
			dest |= 0x3F;
			break;
		case 0xFF12: // NR12 ([TONE] Channel 1 volume envelope)
			dest |= 0x00;
			break;
		case 0xFF13: // NR13 ([TONE] Channel 1 frequency low) (technically write only)
			dest |= 0xFF;
			break;
		case 0xFF14: // NR14 ([TONE] Channel 1 frequency high)
			dest |= 0xBF;
			break;
		case 0xFF15: // Not used
			dest |= 0xFF;
			break;
		case 0xFF16: // NR21 ([TONE] Channel 2 sound length / wave pattern duty)
			dest |= 0x3F;
			break;
		case 0xFF17: // NR22 ([TONE] Channel 2 volume envelope
			dest |= 0x00;
			break;
		case 0xFF18: // NR23 ([TONE] Channel 2 frequency low) (technically write only)
			dest |= 0xFF;
			break;
		case 0xFF19: // NR24 ([TONE] Channel 2 frequency high)
			dest |= 0xBF;
			break;
		case 0xFF1A: // NR30 ([TONE] Channel 3 sound on/off)
			dest |= 0x7F;
			break;
		case 0xFF1B: // NR31 ([WAVE] Channel 3 sound length)
			dest |= 0xFF;
			break;
		case 0xFF1C: // NR32 ([WAVE] Channel 3 select output level)
			dest |= 0x9F;
			break;
		case 0xFF1D: // NR33 ([WAVE] Channel 3 frequency low) (technically write only)
			dest |= 0xFF;
			break;
		case 0xFF1E: // NR34 ([WAVE] Channel 3 frequency high)
			dest |= 0xBF;
			break;		
		case 0xFF1F: // Not used
			dest |= 0xFF;
			break;
		case 0xFF20: // NR41 ([NOISE] Channel 4 sound length)
			dest |= 0xFF;
			break;
		case 0xFF21: // NR42 ([NOISE] Channel 4 volume envelope)
			dest |= 0x00;
			break;
		case 0xFF22: // NR43 ([NOISE] Channel 4 polynomial counter)
			dest |= 0x00;
			break;
		case 0xFF23: // NR44 ([NOISE] Channel 4 counter / consecutive, initial)
			dest |= 0xBF;
			break;
		case 0xFF24: // NR50 (Channel control / ON-OFF / volume)
			dest |= 0x00;
			break;
		case 0xFF25: // NR51 (Select sound output)
			dest |= 0x00;
			break;
		case 0xFF26: // NR52 (Sound ON-OFF)
			dest |= 0x70;
			break;
		default:
			if (reg >= 0xFF27 && reg <= 0xFF2F) { // Not used
				dest |= 0xFF;
			}
			else if (reg >= 0xFF30 && reg <= 0xFF3F) { // [Wave] pattern RAM
				dest = wavePatternRAM[reg - 0xFF30];
			}
			else {
				return false;
			}
	}
	return true;
}

bool SoundProcessor::onClockUpdate(){
	if(!timerEnable) 
		return false;
	
	// Update the 512 Hz frame sequencer.
	if(++nCyclesSinceLastTick >= timerPeriod){
		this->reset();
		this->rollOver();
		return true;
	}
	
	return false;
}

void SoundProcessor::triggerLengthCounter(){ 
	// Handle the length registers (256 Hz)
	if(ch1SoundLengthData.onClockUpdate())
		rNR52->resetBit(0); // Ch 1 OFF
	if(ch2SoundLengthData.onClockUpdate())
		rNR52->resetBit(1); // Ch 2 OFF
	if(ch3SoundLengthData.onClockUpdate())
		rNR52->resetBit(2); // Ch 3 OFF
	if(ch4SoundLengthData.onClockUpdate())
		rNR52->resetBit(3); // Ch 4 OFF
}

void SoundProcessor::triggerVolumeEnvelope(){ 
	// Handle the volume envelope register (64 Hz)
	ch1VolumeEnvelope.onClockUpdate();
	ch2VolumeEnvelope.onClockUpdate();
	ch4VolumeEnvelope.onClockUpdate();
}
	
void SoundProcessor::triggerSweepTimer(){ 
	// Handle the frequency sweep register (128 Hz)
	// x(0) is the initial frequency and x(t-1) is the last frequency
	//x(t) = x(t-1) +/- x(t-1)/2^n
	ch1FrequencySweep.onClockUpdate();
}

void SoundProcessor::rollOver(){
	// Update the 512 Hz frame sequencer.
	if(sequencerTicks % 2 == 0) // Length Counter (256 Hz)
		triggerLengthCounter();
	if(sequencerTicks % 4 == 2) // Sweep Timer (128 Hz)
		triggerSweepTimer();
	if(sequencerTicks % 8 == 7) // Volume Envelope (64 Hz)
		triggerVolumeEnvelope();
	sequencerTicks++;
}

void SoundProcessor::defineRegisters(){
	// Channel 1 registers
	sys->addSystemRegister(this, 0x10, rNR10, "NR10", "33333330");
	sys->addSystemRegister(this, 0x11, rNR11, "NR11", "22222233");
	sys->addSystemRegister(this, 0x12, rNR12, "NR12", "33333333");
	sys->addSystemRegister(this, 0x13, rNR13, "NR13", "22222222");
	sys->addSystemRegister(this, 0x14, rNR14, "NR14", "22200032");

	// Channel 2 registers
	sys->addSystemRegister(this, 0x15, rNR20, "NR20", "00000000"); // Not used
	sys->addSystemRegister(this, 0x16, rNR21, "NR21", "22222233");
	sys->addSystemRegister(this, 0x17, rNR22, "NR22", "33333333");
	sys->addSystemRegister(this, 0x18, rNR23, "NR23", "22222222");
	sys->addSystemRegister(this, 0x19, rNR24, "NR24", "22200032");

	// Channel 3 registers
	sys->addSystemRegister(this, 0x1A, rNR30, "NR30", "00000003");
	sys->addSystemRegister(this, 0x1B, rNR31, "NR31", "33333333");
	sys->addSystemRegister(this, 0x1C, rNR32, "NR32", "00000330");
	sys->addSystemRegister(this, 0x1D, rNR33, "NR33", "22222222");
	sys->addSystemRegister(this, 0x1E, rNR34, "NR34", "22200032");

	// Channel 4 registers
	sys->addSystemRegister(this, 0x1F, rNR40, "NR40", "00000000"); // Not used
	sys->addSystemRegister(this, 0x20, rNR41, "NR41", "33333300");
	sys->addSystemRegister(this, 0x21, rNR42, "NR42", "33333333");
	sys->addSystemRegister(this, 0x22, rNR43, "NR43", "33333333");
	sys->addSystemRegister(this, 0x23, rNR44, "NR44", "00000032");

	// Sound control registers
	sys->addSystemRegister(this, 0x24, rNR50, "NR50", "33333333");
	sys->addSystemRegister(this, 0x25, rNR51, "NR51", "33333333");
	sys->addSystemRegister(this, 0x26, rNR52, "NR52", "11110003");

	// Unused APU registers
	for(unsigned char i = 0x27; i <= 0x2F; i++)
		sys->addDummyRegister(this, i); // Not used

	// Wave RAM
	for(unsigned char i = 0x0; i <= 0xF; i++)
		sys->addSystemRegister(this, i+0x30, rWAVE[i], "WAVE", "33333333");
}
