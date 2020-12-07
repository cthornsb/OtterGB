
#include <iostream>
#include <math.h>

#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Sound.hpp"

// 512 Hz sequencer
SoundProcessor::SoundProcessor() : SystemComponent("APU"), ComponentTimer(512) { 
	masterSoundEnable = false;
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
	switch(reg){
		case 0xFF10: // NR10 ([TONE] Channel 1 sweep register)
			ch1SweepTime   = rNR10->getBits(4,6); // Sweep time (see below)
			ch1SweepIncDec = rNR10->getBit (3);   // [0: Increase, 1: Decrease]
			ch1SweepShift  = rNR10->getBits(0,2); // Number of sweep shift (n=0-7)
			break;
		case 0xFF11: // NR11 ([TONE] Channel 1 sound length / wave pattern duty)
			ch1WavePatternDuty = rNR11->getBits(6,7); // Wave pattern duty (see below)
			ch1SoundLengthData = rNR11->getBits(0,5); // Length = (64-t1)/256 s (if bit-6 of NR14 set)
			if(ch1SoundLengthData == 0x0) // Disable the channel
				rNR52->resetBit(0); // Ch 1 OFF
			break;
		case 0xFF12: // NR12 ([TONE] Channel 1 volume envelope)
			ch1InitialVolumeEnv = rNR12->getBits(4,7); // Initial envelope volume (0-0F) (0=No sound)
			ch1DirectionEnv     = rNR12->getBit (3);   // Envelope direction [0: Increase, 1: Decrease]
			ch1NumberSweepEnv   = rNR12->getBits(0,2); // Number of envelope sweep (n=0-7, if 0 then stop)
			break;
		case 0xFF13: // NR13 ([TONE] Channel 1 frequency low)
			freq16 = (rNR14->getBits(0,2) << 8) + rNR13->getValue();
			ch1Frequency0 = 131072.0f/(2048-freq16); // Hz
			break;
		case 0xFF14: // NR14 ([TONE] Channel 1 frequency high)
			ch1Restart       = rNR14->getBit(7); // [0: No action, 1: Restart sound]
			ch1CounterSelect = rNR14->getBit(6); // [0: No action, 1: Stop output when NR11 length expires]
			freq16 = (rNR14->getBits(0,2) << 8) + rNR13->getValue();
			ch1Frequency0 = 131072.0f/(2048-freq16); // Hz
			break;
		case 0xFF15: // Not used
			break;
		case 0xFF16: // NR21 ([TONE] Channel 2 sound length / wave pattern duty)
			ch2WavePatternDuty = rNR21->getBits(6,7); // Wave pattern duty (see below)
			ch2SoundLengthData = rNR21->getBits(0,5); // Length = (64-t1)/256 s (if bit-6 of NR24 set)
			if(ch2SoundLengthData == 0x0) // Disable the channel
				rNR52->resetBit(1); // Ch 2 OFF
			break;
		case 0xFF17: // NR22 ([TONE] Channel 2 volume envelope
			ch2InitialVolumeEnv = rNR22->getBits(4,7); // Initial envelope volume (0-0F) (0=No sound)
			ch2DirectionEnv     = rNR22->getBit (3);   // Envelope direction [0: Increase, 1: Decrease]
			ch2NumberSweepEnv   = rNR22->getBits(0,2); // Number of envelope sweep (n=0-7, if 0 then stop)
			break;
		case 0xFF18: // NR23 ([TONE] Channel 2 frequency low)
			freq16 = (rNR24->getBits(0,2) << 8) + rNR23->getValue();
			ch2Frequency0 = 131072.0f/(2048-freq16); // Hz
			break;
		case 0xFF19: // NR24 ([TONE] Channel 2 frequency high)
			ch2Restart       = rNR24->getBit(7); // [0: No action, 1: Restart sound]
			ch2CounterSelect = rNR24->getBit(6); // [0: No action, 1: Stop output when NR21 length expires]
			freq16 = (rNR24->getBits(0,2) << 8) + rNR23->getValue();
			ch2Frequency0 = 131072.0f/(2048-freq16); // Hz
			break;
		case 0xFF1A: // NR30 ([TONE] Channel 3 sound on/off)
			ch3Enable = rNR30->getBit(7); // [0:Stop, 1:Enable]
			if(ch3Enable)
				rNR30->setBit(2); // Ch 3 ON
			else
				rNR30->resetBit(2); // Ch 3 OFF
			break;
		case 0xFF1B: // NR31 ([WAVE] Channel 3 sound length)
			ch3SoundLengthData = rNR31->getValue(); // t1=0-255
			if(ch3SoundLengthData == 0x0) // Disable the channel
				rNR52->resetBit(2); // Ch 3 OFF
			break;
		case 0xFF1C: // NR32 ([WAVE] Channel 3 select output level)
			ch3OutputLevel = rNR32->getBits(5,6); // Select output level (see below)
			break;
		case 0xFF1D: // NR33 ([WAVE] Channel 3 frequency low)
			freq16 = (rNR34->getBits(0,2) << 8) + rNR33->getValue();
			ch3Frequency0 = 65536.0f/(2048-freq16); // Hz
			break;
		case 0xFF1E: // NR34 ([WAVE] Channel 3 frequency high)
			ch3Restart       = rNR34->getBit(7); // [0: No action, 1: Restart sound]
			ch3CounterSelect = rNR34->getBit(6); // [0: No action, 1: Stop output when NR31 length expires]
			freq16 = (rNR34->getBits(0,2) << 8) + rNR33->getValue();
			ch3Frequency0 = 65536.0f/(2048-freq16); // Hz
			break;		
		case 0xFF1F: // Not used
			break;
		case 0xFF20: // NR41 ([NOISE] Channel 4 sound length)
			ch4SoundLengthData = rNR41->getBits(0,5); // t1=0-63
			if(ch4SoundLengthData == 0x0) // Disable the channel
				rNR52->resetBit(3); // Ch 4 OFF
			break;
		case 0xFF21: // NR42 ([NOISE] Channel 4 volume envelope)
			ch4InitialVolumeEnv = rNR42->getBits(4,7); // Initial envelope volume (0-0F) (0=No sound)
			ch4DirectionEnv     = rNR42->getBit (3);   // Envelope direction [0: Increase, 1: Decrease]
			ch4NumberSweepEnv   = rNR42->getBits(0,2); // Number of envelope sweep (n=0-7, if 0 then stop)
			break;
		case 0xFF22: // NR43 ([NOISE] Channel 4 polynomial counter)
			ch4ShiftClockFreq   = rNR43->getBits(4,7); // Shift clock frequency (s)
			ch4CounterStepWidth = rNR43->getBit (3); // Counter step/width [0: 15-bits, 1: 7-bits]
			ch4DividingRation   = rNR43->getBits(0,2); // Dividing ratio of frequency (r)
			break;
		case 0xFF23: // NR44 ([NOISE] Channel 4 counter / consecutive, initial)
			ch4Restart       = rNR44->getBit(7); // [0: No action, 1: Restart sound]
			ch4CounterSelect = rNR44->getBit(6); // [0: No action, 1: Stop output when NR41 length expires]
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
				this->reset();
			}
			else{
				rNR52->resetBit(3); // Ch 4 OFF
				rNR52->resetBit(2); // Ch 3 OFF
				rNR52->resetBit(1); // Ch 2 OFF
				rNR52->resetBit(0); // Ch 1 OFF
				for(unsigned char i = 0x10; i < 0x30; i++){ // Clear all sound registers (except NR52 and wave RAM)
					if(i == 0x26) continue; // Skip NR52
					sys->clearRegister(i);
				}
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
	// Update the 512 Hz sequencer.
	if(nCyclesSinceLastTick % 2 == 0) // Length Counter (256 Hz)
		triggerLengthCounter();
	if(nCyclesSinceLastTick % 4 == 2) // Sweep Timer (128 Hz)
		triggerSweepTimer();
	if(nCyclesSinceLastTick % 8 == 7) // Volume Envelope (64 Hz)
		triggerVolumeEnvelope();
	if(++nCyclesSinceLastTick >= timerPeriod){
		this->reset();
		return true;
	}
	return false;
}

void SoundProcessor::triggerLengthCounter(){ 
	// Handle the length register (256 Hz)
	if(ch1SoundLengthData){
		//if(!--ch1SoundLengthData)
			//ch1Enabled = false;
	}
}

void SoundProcessor::triggerVolumeEnvelope(){ 
	// Handle the volume envelope register (64 Hz)
	//ch1InitialVolumeEnv = (val & 0xF0) >> 4; // Initial envelope volume (0-0F) (0=No sound)
	//ch1DirectionEnv     = (val & 0x8) != 0;  // Envelope direction [0: Increase, 1: Decrease]
	//ch1NumberSweepEnv   = (val & 0x7);       // Number of envelope sweep (n=0-7, if 0 then stop)
	if(ch1NumberSweepEnv){
		if(!ch1DirectionEnv) // Increase volume
			ch1InitialVolumeEnv++;
		else // Decrease volume
			ch1InitialVolumeEnv--;
		ch1NumberSweepEnv--;
	}
}
	
void SoundProcessor::triggerSweepTimer(){ 
	// Handle the frequency sweep register (128 Hz)
	// x(0) is the initial frequency and x(t-1) is the last frequency
	//x(t) = x(t-1) +/- x(t-1)/2^n
	if(ch1SweepTime){
		if(!ch1SweepIncDec) // Increase frequency
			ch1Frequency = ch1Frequency * (1 + 1/powf(2, ch1SweepShift));
		else // Decrease frequency
			ch1Frequency = ch1Frequency * (1 - 1/powf(2, ch1SweepShift));
		ch1SweepTime--;
	}
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
