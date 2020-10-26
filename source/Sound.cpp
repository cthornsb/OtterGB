
#include <iostream>

#include "SystemRegisters.hpp"
#include "Support.hpp"
#include "Sound.hpp"

bool FrameSequencer::onClockUpdate(){
	if(nCyclesSinceLastTick % 2 == 0) // Length Counter (256 Hz)
		triggerLengthCounter();
	if(nCyclesSinceLastTick % 4 == 2) // Sweep Timer (128 Hz)
		triggerSweepTimer();
	if(nCyclesSinceLastTick % 8 == 7) // Volume Envelope (64 Hz)
		triggerVolumeEnvelope();
	if(++nCyclesSinceLastTick >= timerPeriod){
		nCyclesSinceLastTick = 0;
		return true;
	}
	return false;
}

// ~44.1 kHz
SoundProcessor::SoundProcessor() : SystemComponent(), ComponentTimer(95) { 
}

bool SoundProcessor::writeRegister(const unsigned short &reg, const unsigned char &val){
	switch(reg){
		case 0xFF10: // NR10 ([TONE] Channel 1 sweep register)
			(*rNR10) = val;
			ch1SweepTime   = (val & 0x70) >> 4; // Sweep time (see below)
			ch1SweepIncDec = (val & 0x8) != 0;  // [0: Increase, 1: Decrease]
			ch1SweepShift  = (val & 0x7);       // Number of sweep shift (n=0-7)
			/** Sweep Time
				0: OFF (no frequency change)
				1: 1/128 Hz (7.8 ms)
				2: 2/128 Hz (15.6 ms)
				3: 3/128 Hz (23.4 ms)
				4: 4/128 Hz (31.3 ms)
				5: 5/128 Hz (39.1 ms)
				6: 6/128 Hz (46.9 ms)
				7: 7/128 Hz (54.7 ms) 
				Change in frequency (NR13/NR14) at each shift:
					x(t) = x(t-1) +/- x(t-1)/2^n
				**/
			break;
		case 0xFF11: // NR11 ([TONE] Channel 1 sound length / wave pattern duty)
			(*rNR11) = val;
			ch1WavePatternDuty = (val & 0xC0) >> 6; // Wave pattern duty (see below)
			ch1SoundLengthData = (val & 0x3F);      // Length = (64-t1)/256 s (if bit-6 of NR14 set)
			/** Wave pattern duty
				0: 12.5%
				1: 25.0%
				2: 50.0% (normal)
				3: 75.0% **/
			break;
		case 0xFF12: // NR12 ([TONE] Channel 1 volume envelope)
			(*rNR12) = val;
			ch1InitialVolumeEnv = (val & 0xF0) >> 4; // Initial envelope volume (0-0F) (0=No sound)
			ch1DirectionEnv     = (val & 0x8) != 0;  // Envelope direction [0: Increase, 1: Decrease]
			ch1NumberSweepEnv   = (val & 0x7);       // Number of envelope sweep (n=0-7, if 0 then stop)
			// Length of 1 step = n/64 s
			break;
		case 0xFF13: // NR13 ([TONE] Channel 1 frequency low)
			(*rNR13) = val;
			ch1FrequencyLow = val; // Lower 8-bits of 11-bit frequency (f)
			break;
		case 0xFF14: // NR14 ([TONE] Channel 1 frequency high)
			(*rNR14) = val;
			ch1Restart       = (val & 0x80) != 0; // [0: No action, 1: Restart sound]
			ch1CounterSelect = (val & 0x40) != 0; // [0: No action, 1: Stop output when NR11 length expires]
			ch1FrequencyHigh = (val & 0x7); // Upper 3-bits of 11-bit frequency (f)
			// Frequency = 131072/(2048-f) Hz
			break;
		case 0xFF16: // NR21 ([TONE] Channel 2 sound length / wave pattern duty)
			(*rNR21) = val;
			ch2WavePatternDuty = (val & 0xC0) >> 6; // Wave pattern duty (see below)
			ch2SoundLengthData = (val & 0x3F);      // Length = (64-t1)/256 s (if bit-6 of NR24 set)
			/** Wave pattern duty
				0: 12.5%
				1: 25.0%
				2: 50.0% (normal)
				3: 75.0% **/
			break;
		case 0xFF17: // NR22 ([TONE] Channel 2 volume envelope
			(*rNR22) = val;
			ch2InitialVolumeEnv = (val & 0xF0) >> 4; // Initial envelope volume (0-0F) (0=No sound)
			ch2DirectionEnv     = (val & 0x8) != 0;  // Envelope direction [0: Increase, 1: Decrease]
			ch2NumberSweepEnv   = (val & 0x7);       // Number of envelope sweep (n=0-7, if 0 then stop)
			// Length of 1 step = n/64 s
			break;
		case 0xFF18: // NR23 ([TONE] Channel 2 frequency low)
			(*rNR23) = val;
			ch2FrequencyLow = val; // Lower 8-bits of 11-bit frequency (f)
			break;
		case 0xFF19: // NR24 ([TONE] Channel 2 frequency high)
			(*rNR24) = val;
			ch2Restart       = (val & 0x80) != 0; // [0: No action, 1: Restart sound]
			ch2CounterSelect = (val & 0x40) != 0; // [0: No action, 1: Stop output when NR21 length expires]
			ch2FrequencyHigh = (val & 0x7); // Upper 3-bits of 11-bit frequency (f)
			// Frequency = 131072/(2048-f) Hz
			break;
		case 0xFF1A: // NR30 ([TONE] Channel 3 sound on/off)
			(*rNR30) = val;
			ch3Enable = (val & 0x80) != 0; // [0:Stop, 1:Enable]
			break;
		case 0xFF1B: // NR31 ([WAVE] Channel 3 sound length)
			(*rNR31) = val;
			ch3SoundLengthData = val; // t1=0-255
			// Sound Length = (256-t1)/256 s, if bit-6 of NR34 set
			break;
		case 0xFF1C: // NR32 ([WAVE] Channel 3 select output level)
			(*rNR32) = val;
			ch3OutputLevel = (val & 0x60) >> 5; // Select output level (see below)
			/** Possible output levels:
				0: Mute
				1: 100% (Wave pattern RAM used as is)
				2:  50% (Shift wave pattern RAM once to the right)
				3:  25% (Shift wave pattern RAM twice to the right)
			**/
			break;
		case 0xFF1D: // NR33 ([WAVE] Channel 3 frequency low)
			(*rNR33) = val;
			ch3FrequencyLow = val; // Lower 8-bits of 11-bit frequency (f)
			break;
		case 0xFF1E: // NR34 ([WAVE] Channel 3 frequency high)
			(*rNR34) = val;
			ch3Restart       = (val & 0x80) != 0; // [0: No action, 1: Restart sound]
			ch3CounterSelect = (val & 0x40) != 0; // [0: No action, 1: Stop output when NR31 length expires]
			ch3FrequencyHigh = (val & 0x7); // Upper 3-bits of 11-bit frequency (f)
			// Frequency = 131072/(2048-f) Hz
			break;		
		case 0xFF20: // NR41 ([NOISE] Channel 4 sound length)
			(*rNR41) = val;
			ch4SoundLengthData = val; // t1=0-255
			// Sound Length = (256-t1)/256 s, if bit-6 of NR44 set
			break;
		case 0xFF21: // NR42 ([NOISE] Channel 4 volume envelope)
			(*rNR42) = val;
			ch4InitialVolumeEnv = (val & 0xF0) >> 4; // Initial envelope volume (0-0F) (0=No sound)
			ch4DirectionEnv     = (val & 0x8) != 0;  // Envelope direction [0: Increase, 1: Decrease]
			ch4NumberSweepEnv   = (val & 0x7);       // Number of envelope sweep (n=0-7, if 0 then stop)
			// Length of 1 step = n/64 s
			break;
		case 0xFF22: // NR43 ([NOISE] Channel 4 polynomial counter)
			(*rNR43) = val;
			ch4ShiftClockFreq = (val & 0xF0) >> 4; // Shift clock frequency (s)
			ch4CounterStepWidth = (val & 0x8) != 0; // Counter step/width [0: 15-bits, 1: 7-bits]
			ch4DividingRation = (val & 0x7); // Dividing ratio of frequency (r)
			// Frequency = 524288/r/2^(s+1), for r=0, assume r=0.5
		case 0xFF23: // NR44 ([NOISE] Channel 4 counter / consecutive, initial)
			(*rNR44) = val;
			ch4Restart       = (val & 0x80) != 0; // [0: No action, 1: Restart sound]
			ch4CounterSelect = (val & 0x40) != 0; // [0: No action, 1: Stop output when NR41 length expires]
			break;
		case 0xFF24: // NR50 (Channel control / ON-OFF / volume)
			// S01 and S02 terminals not emulated
			(*rNR50) = val;
			outputLevelSO2 = (val & 0x70) >> 4; // Output level (0-7)
			outputLevelSO1 = (val & 0x7);       // Output level (0-7)
			break;
		case 0xFF25: // NR51 (Select sound output)
			(*rNR51) = val;
			ch4ToSO2 = (val & 0x80) != 0;
			ch3ToSO2 = (val & 0x40) != 0;
			ch2ToSO2 = (val & 0x20) != 0;
			ch1ToSO2 = (val & 0x10) != 0;
			ch4ToSO1 = (val & 0x8) != 0;
			ch3ToSO1 = (val & 0x4) != 0;
			ch2ToSO1 = (val & 0x2) != 0;
			ch1ToSO1 = (val & 0x1) != 0;
			break;
		case 0xFF26: // NR52 (Sound ON-OFF)
			(*rNR52) = val;
			masterSoundEnable = (val & 0x80) != 0;
			if(masterSoundEnable) // Power on the frame sequencer
				sequencer.reset();
			// Bits 0-3 are read-only, writing has no effect
			break;
		case 0xFF30 ... 0xFF3F: // [Wave] pattern RAM
			(*rWAVE) = val;
			// Contains 32 4-bit samples played back upper 4 bits first
			wavePatternRAM[reg-0xFF30] = val;
			break;
		default:
			return false;
	}
	return true;
}

bool SoundProcessor::readRegister(const unsigned short &reg, unsigned char &dest){
	switch(reg){
		case 0xFF10: // NR10 ([TONE] Channel 1 sweep register)
			dest = (*rNR10);
			break;
		case 0xFF11: // NR11 ([TONE] Channel 1 sound length / wave pattern duty)
			dest = (*rNR11);
			break;
		case 0xFF12: // NR12 ([TONE] Channel 1 volume envelope)
			dest = (*rNR12);
			break;
		case 0xFF13: // NR13 ([TONE] Channel 1 frequency low) (technically write only)
			dest = (*rNR13);
			break;
		case 0xFF14: // NR14 ([TONE] Channel 1 frequency high)
			dest = (*rNR14);
			break;
		case 0xFF16: // NR21 ([TONE] Channel 2 sound length / wave pattern duty)
			dest = (*rNR21);
			break;
		case 0xFF17: // NR22 ([TONE] Channel 2 volume envelope
			dest = (*rNR22);
			break;
		case 0xFF18: // NR23 ([TONE] Channel 2 frequency low) (technically write only)
			dest = (*rNR23);
			break;
		case 0xFF19: // NR24 ([TONE] Channel 2 frequency high)
			dest = (*rNR24);
			break;
		case 0xFF1A: // NR30 ([TONE] Channel 3 sound on/off)
			dest = (*rNR30);
			break;
		case 0xFF1B: // NR31 ([WAVE] Channel 3 sound length)
			dest = (*rNR31);
			break;
		case 0xFF1C: // NR32 ([WAVE] Channel 3 select output level)
			dest = (*rNR32);
			break;
		case 0xFF1D: // NR33 ([WAVE] Channel 3 frequency low) (technically write only)
			dest = (*rNR33);
			break;
		case 0xFF1E: // NR34 ([WAVE] Channel 3 frequency high)
			dest = (*rNR34);
			break;		
		case 0xFF20: // NR41 ([NOISE] Channel 4 sound length)
			dest = (*rNR41);
			break;
		case 0xFF21: // NR42 ([NOISE] Channel 4 volume envelope)
			dest = (*rNR42);
			break;
		case 0xFF22: // NR43 ([NOISE] Channel 4 polynomial counter)
			dest = (*rNR43);
			break;
		case 0xFF23: // NR44 ([NOISE] Channel 4 counter / consecutive, initial)
			dest = (*rNR44);
			break;
		case 0xFF24: // NR50 (Channel control / ON-OFF / volume)
			dest = (*rNR50);
			break;
		case 0xFF25: // NR51 (Select sound output)
			dest = (*rNR51);
			break;
		case 0xFF26: // NR52 (Sound ON-OFF)
			dest = (*rNR52);
			break;
		case 0xFF30 ... 0xFF3F: // [Wave] pattern RAM
			dest = (*rNR30);
			break;
		default:
			return false;
	}
	return true;
}

bool SoundProcessor::onClockUpdate(){
	sequencer.onClockUpdate();
}
