
#include <iostream>
#include <math.h>

#include "Support.hpp"
#include "SystemGBC.hpp"
#include "Sound.hpp"

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

/////////////////////////////////////////////////////////////////////
// class UnitTimer
/////////////////////////////////////////////////////////////////////

bool UnitTimer::clock(){
	if(!bEnabled || !nCounter)
		return false;
	if(--nCounter == 0){ // Decrement the timer
		reload();
		return true; // The timer rolled over
	}
	return false;
}

void UnitTimer::reload(){
	nCounter = nPeriod;
}

/////////////////////////////////////////////////////////////////////
// class SquareWave
/////////////////////////////////////////////////////////////////////

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

void SquareWave::update(const unsigned int& sequencerTicks){
	// (Sweep ->) Timer -> Duty -> Length Counter -> Envelope -> Mixer
	if(bSweepEnabled && frequency->clock()){ // Clock the frequency sweep (128 Hz)
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
	if(this->clock()){ // Unit timer rolled over
		// Update square wave duty waveform
		bool lowBit = ((nWaveform & 0x1) == 0x1);
		nWaveform = nWaveform >> 1;
		if(lowBit) // Set high bit
			nWaveform |= 0x80;
		else // Clear high bit
			nWaveform &= 0x7f;
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
	//nSequencerTicks++;
}

void SquareWave::trigger(){
	this->reload(); // Reload the main timer with its phase
	if(bSweepEnabled)
		frequency->trigger();
	length.trigger();
	volume.trigger();
}

/////////////////////////////////////////////////////////////////////
// class WaveTable
/////////////////////////////////////////////////////////////////////

unsigned char WaveTable::sample(){
	switch(nVolume){
	case 0: // Mute
		break;
	case 1: // 100% volume
		return nBuffer;
	case 2: // 50% volume (shift 1 bit)
		return (nBuffer >> 1);
	case 3: // 25% volume (shift 2 bits)
		return (nBuffer >> 2);
	default: // Mute
		break;
	};
	return 0;
}

void WaveTable::update(const unsigned int& sequencerTicks){
	// Timer -> Wave -> Length Counter -> Volume -> Mixer
	if(this->clock()){ // Unit timer rolled over
		if(++nIndex >= 32) // Increment sample position
			nIndex = 0;
		// Retrieve a 4-bit sample
		if(nIndex % 2 == 0) // Low nibble
			nBuffer = data[nIndex / 2] & 0xf;
		else // High nibble
			nBuffer = (data[nIndex / 2] & 0xf0) >> 4;
	}
	if(sequencerTicks % 2 == 0){ // Clock the length counter (256 Hz)
		if(length.clock()){
			// If length counter rolls over, disable the channel
			bDisableThisChannel = true;
		}
	}
}

void WaveTable::trigger(){
	this->reload(); // Reload the main timer with its phase
	length.trigger();
	nIndex = 0;
}

/////////////////////////////////////////////////////////////////////
// class ShiftRegister
/////////////////////////////////////////////////////////////////////

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
	return ((reg & 0x1) == 0x1 ? 0x0 : 0xf); // Inverted
}

void ShiftRegister::update(const unsigned int& sequencerTicks){
	// Timer -> LFSR -> Length Counter -> Envelope -> Mixer
	if(this->clock()){ // Unit timer rolled over
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

void ShiftRegister::trigger(){
	this->reload(); // Reload the main timer with its phase
	reg = 0x7fff; // Set all 15 bits to 1
	length.trigger();
	volume.trigger();
}

/////////////////////////////////////////////////////////////////////
// class LengthCounter
/////////////////////////////////////////////////////////////////////

bool LengthCounter::clock(){
	if(!bEnabled || !nCounter)
		return false;
	if(nCounter == 1){ // Disable the channel
		nCounter = 0;
		return true;
	}
	nCounter--;
	return false;
}

void LengthCounter::trigger(){
	if(!nCounter){
		nCounter = nMaximum;
		bRefilled = true;
	}
	else
		bRefilled = false;
}

/////////////////////////////////////////////////////////////////////
// class VolumeEnvelope
/////////////////////////////////////////////////////////////////////

bool VolumeEnvelope::clock(){
	if(!bEnabled || !nPeriod)
		return false;
	if(--nCounter == 0){
		nCounter = nPeriod;
		if(bAdd){ // Add (louder)
			if(nVolume + 1 <= 15)
				nVolume++;
		}
		else{ // Subtract (quieter)
			if(nVolume > 0)
				nVolume--;
		}
		return (nVolume == 0);
	}
	return false;
}

void VolumeEnvelope::trigger(){
	//Volume envelope timer is reloaded with period.
	//Channel volume is reloaded from NRx2.

	nCounter = nPeriod;
	//nVolume = ;
}
		
/////////////////////////////////////////////////////////////////////
// class FrequencySweep
/////////////////////////////////////////////////////////////////////
		
bool FrequencySweep::clock(){
	if(nSequencerTicks++ % 4 != 2)
		return false;
	if(!bEnabled)
		return false;
	if(--nCounter == 0){
		nCounter = (nPeriod != 0 ? nPeriod : 8); // Reset period counter
		if(nPeriod){ // Only compute new frequency with non-zero period
			if(compute()){ // Compute new frequency
				// Update shadow frequency
				if(nShift){ // If new frequency <= 2047 and shift is non-zero, update the frequency
					nShadowFrequency = nNewFrequency;
				}
				if(!compute()){ // Compute new frequency again immediately
					bOverflow2 = true;
				}
			}
			else{ // Frequency overflow, disable channel
				bOverflow = true;
				disable(); // TEMP
			}
		}
		return true; // Frequency sweep clock rolled over
	}
	return false;
}

void FrequencySweep::trigger(){
	nShadowFrequency = extTimer->getFrequency();
	nCounter = (nPeriod ? nPeriod : 8);
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
	// Compute new frequency (x)
	if(!bNegate)
		nNewFrequency = (nShadowFrequency >> nShift) + nShadowFrequency;
	else{
		nNewFrequency = nShadowFrequency + twosComp11bit(nShadowFrequency >> nShift);
		bNegateModeUsed = true;
	}
	return (nNewFrequency <= 2047); // Overflow check
}

/////////////////////////////////////////////////////////////////////
// class SoundProcessor
/////////////////////////////////////////////////////////////////////

SoundProcessor::SoundProcessor() : 
	SystemComponent("APU"), 
	ComponentTimer(2048), // 512 Hz sequencer
	audio(new SoundManager()),
	ch1(new FrequencySweep()),
	ch2(),
	ch3(wavePatternRAM),
	ch4(),
	outputLevelSO1(0),
	outputLevelSO2(0),
	wavePatternRAM(),
	masterSoundEnable(false),
	sequencerTicks(0)
{ 
	//audio->init();
}

bool SoundProcessor::checkRegister(const unsigned short &reg){
	// Check if the APU is powered down.
	// If the APU is powered down, only the APU control register and
	//  wave RAM may be written to.
	if(!masterSoundEnable && (reg < 0xFF30 || reg > 0xFF3F) && reg != 0xFF26) 
		return false;
	return true;
}

bool extraLengthClock(AudioUnit& unit, const unsigned int& nTicks, bool triggered){
	LengthCounter* counter = unit.getLengthCounter();
	if(nTicks % 2 != 0){
		// Extra length clocking may occur when enabled on a cycle
		// which clocks the length counter.
		if(!counter->isEnabled() && counter->getLength() != 0){
			// Length counter is going from disabled to enabled with a non-zero length
			// so we need to clock the counter an extra time.
			counter->enable();
			if(counter->clock() && !triggered){
				// If the extra clock caused the counter to roll over, and bit 7 is clear, disable the channel
				return true;
			}
		}
	}
	return false;
}

bool SoundProcessor::writeRegister(const unsigned short &reg, const unsigned char &val){
	switch(reg){
		case 0xFF10: // NR10 ([TONE] Channel 1 sweep register)
			ch1.getFrequencySweep()->setPeriod(rNR10->getBits(4,6));
			ch1.getFrequencySweep()->setBitShift(rNR10->getBits(0,2));
			if(!ch1.getFrequencySweep()->setNegate(rNR10->getBit(3))){
				// Switching from negative to positive disables the channel
				disableChannel(1);
			}
			break;
		case 0xFF11: // NR11 ([TONE] Channel 1 sound length / wave pattern duty)
			ch1.setWaveDuty(rNR11->getBits(6,7)); // Wave pattern duty (see below)
			ch1.setLength(rNR11->getBits(0,5));
			break;
		case 0xFF12: // NR12 ([TONE] Channel 1 volume envelope)
			ch1.getVolumeEnvelope()->setVolume(rNR12->getBits(4,7));
			ch1.getVolumeEnvelope()->setAddMode(rNR12->getBit(3));
			ch1.getVolumeEnvelope()->setPeriod(rNR12->getBits(0,2));
			if(rNR12->getBits(3,7) == 0){ // Disable DAC
				disableChannel(1);
			}
			break;
		case 0xFF13: // NR13 ([TONE] Channel 1 frequency low)
			ch1.setFrequency((rNR14->getBits(0,2) << 8) + rNR13->getValue());
			break;
		case 0xFF14: // NR14 ([TONE] Channel 1 frequency high)
			ch1.setFrequency((rNR14->getBits(0,2) << 8) + rNR13->getValue());
			if(rNR14->getBit(6)){ // Enable length counter
				enableChannel(1);
				if(extraLengthClock(ch1, sequencerTicks, rNR14->getBit(7)))
					disableChannel(1);
				ch1.enable();
			}
			else{
				ch1.disable();
			}
			if(rNR14->getBit(7)){ // Trigger length counter
				ch1.trigger();
				if((sequencerTicks % 2 != 0) && ch1.getLengthCounter()->isEnabled() && ch1.getLengthCounter()->wasRefilled()){
					ch1.getLengthCounter()->clock();
				}
				if(ch1.getFrequencySweep()->overflowed()){
					disableChannel(1);
				}
			}
			break;
		case 0xFF15: // Not used
			break;
		case 0xFF16: // NR21 ([TONE] Channel 2 sound length / wave pattern duty)
			ch2.setWaveDuty(rNR21->getBits(6,7)); // Wave pattern duty (see below)
			ch2.setLength(rNR21->getBits(0,5));
			break;
		case 0xFF17: // NR22 ([TONE] Channel 2 volume envelope
			ch2.getVolumeEnvelope()->setVolume(rNR22->getBits(4,7));
			ch2.getVolumeEnvelope()->setAddMode(rNR22->getBit(3));
			ch2.getVolumeEnvelope()->setPeriod(rNR22->getBits(0,2));
			if(rNR22->getBits(3,7) == 0){ // Disable DAC
				disableChannel(2);
			}
			break;
		case 0xFF18: // NR23 ([TONE] Channel 2 frequency low)
			ch2.setFrequency((rNR24->getBits(0,2) << 8) + rNR23->getValue());
			break;
		case 0xFF19: // NR24 ([TONE] Channel 2 frequency high)
			ch2.setFrequency((rNR24->getBits(0,2) << 8) + rNR23->getValue());
			if(rNR24->getBit(6)){
				enableChannel(2);
				if(extraLengthClock(ch2, sequencerTicks, rNR24->getBit(7)))
					disableChannel(2);
				ch2.enable();
			}
			else{
				ch2.disable();
			}
			if(rNR24->getBit(7)){
				ch2.trigger();
				if((sequencerTicks % 2 != 0) && ch2.getLengthCounter()->isEnabled() && ch2.getLengthCounter()->wasRefilled()){
					ch2.getLengthCounter()->clock();
				}
			}
			break;
		case 0xFF1A: // NR30 ([TONE] Channel 3 sound on/off)
			if(!rNR30->getBit(7))
				disableChannel(3); // Ch 3 OFF
			break;
		case 0xFF1B: // NR31 ([WAVE] Channel 3 sound length)
			ch3.setLength(rNR31->getValue());
			break;
		case 0xFF1C: // NR32 ([WAVE] Channel 3 select output level)
			ch3.setVolumeLevel(rNR32->getBits(5,6));
			break;
		case 0xFF1D: // NR33 ([WAVE] Channel 3 frequency low)
			ch3.setFrequency((rNR34->getBits(0,2) << 8) + rNR33->getValue());
			break;
		case 0xFF1E: // NR34 ([WAVE] Channel 3 frequency high)
			ch3.setFrequency((rNR34->getBits(0,2) << 8) + rNR33->getValue());
			if(rNR34->getBit(6)){
				enableChannel(3);
				if(extraLengthClock(ch3, sequencerTicks, rNR34->getBit(7)))
					disableChannel(3);
				ch3.enable();
			}
			else{
				ch3.disable();
			}
			if(rNR34->getBit(7)){
				ch3.trigger();
				if((sequencerTicks % 2 != 0) && ch3.getLengthCounter()->isEnabled() && ch3.getLengthCounter()->wasRefilled()){
					ch3.getLengthCounter()->clock();
				}
			}
			break;		
		case 0xFF1F: // Not used
			break;
		case 0xFF20: // NR41 ([NOISE] Channel 4 sound length)
			ch4.setLength(rNR41->getBits(0,5));
			break;
		case 0xFF21: // NR42 ([NOISE] Channel 4 volume envelope)
			ch4.getVolumeEnvelope()->setVolume(rNR42->getBits(4,7));
			ch4.getVolumeEnvelope()->setAddMode(rNR42->getBit(3));
			ch4.getVolumeEnvelope()->setPeriod(rNR42->getBits(0,2));
			if(rNR42->getBits(3,7) == 0){ // Disable DAC
				disableChannel(4);
			}
			break;
		case 0xFF22: // NR43 ([NOISE] Channel 4 polynomial counter)
			ch4.setClockShift(rNR43->getBits(4,7)); // Shift clock frequency (s)
			ch4.setWidthMode(rNR43->getBit(3));     // Counter step/width [0: 15-bits, 1: 7-bits]
			ch4.setDivisor(rNR43->getBits(0,2));    // Dividing ratio of frequency (r)
			break;
		case 0xFF23: // NR44 ([NOISE] Channel 4 counter / consecutive, initial)
			if(rNR44->getBit(6)){
				enableChannel(4);
				if(extraLengthClock(ch4, sequencerTicks, rNR44->getBit(7)))
					disableChannel(4);
				ch4.enable();
			}
			else{
				ch4.disable();
			}
			if(rNR44->getBit(7)){
				ch4.trigger();
				if((sequencerTicks % 2 != 0) && ch4.getLengthCounter()->isEnabled() && ch4.getLengthCounter()->wasRefilled()){
					ch4.getLengthCounter()->clock();
				}
			}
			break;
		case 0xFF24: // NR50 (Channel control / ON-OFF / volume)
			outputLevelSO2 = rNR50->getBits(4,6); // Output level (0-7)
			outputLevelSO1 = rNR50->getBits(0,2); // Output level (0-7)
			break;
		case 0xFF25: // NR51 (Select sound output)
			// Left channel
			ch4.sendToSO2(rNR51->getBit(7));
			ch3.sendToSO2(rNR51->getBit(6));
			ch2.sendToSO2(rNR51->getBit(5));
			ch1.sendToSO2(rNR51->getBit(4));
			// Right channel
			ch4.sendToSO1(rNR51->getBit(3));
			ch3.sendToSO1(rNR51->getBit(2));
			ch2.sendToSO1(rNR51->getBit(1));
			ch1.sendToSO1(rNR51->getBit(0));
			break;
		case 0xFF26: // NR52 (Sound ON-OFF)
			masterSoundEnable = rNR52->getBit(7);
			if(masterSoundEnable){ // Power on the frame sequencer
				//if (!audio->isRunning())
				//	audio->start(); // Start audio interface
				// Reset frame sequencer (the 512 Hz clock is always running, even while APU is powered down)
				sequencerTicks = 0;
			}
			else{
				//if (audio->isRunning())
				//	audio->stop(); // Stop audio interface
				disableChannel(4); // Ch 4 OFF
				disableChannel(3); // Ch 3 OFF
				disableChannel(2); // Ch 2 OFF
				disableChannel(1); // Ch 1 OFF
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

void SoundProcessor::disableChannel(const int& ch){
	if(ch < 1 || ch > 4) // Invalid channel
		return;
	rNR52->resetBit(ch - 1);
}

void SoundProcessor::disableChannel(const Channels& ch){
	switch(ch){
	case Channels::CH1:
		disableChannel(1);
		break;
	case Channels::CH2:
		disableChannel(2);
		break;
	case Channels::CH3:
		disableChannel(3);
		break;
	case Channels::CH4:
		disableChannel(4);
		break;
	default:
		break;	
	}
}

void SoundProcessor::enableChannel(const int& ch){
	if(ch < 1 || ch > 4) // Invalid channel
		return;
	rNR52->setBit(ch - 1);
}

void SoundProcessor::enableChannel(const Channels& ch){
	switch(ch){
	case Channels::CH1:
		enableChannel(1);
		break;
	case Channels::CH2:
		enableChannel(2);
		break;
	case Channels::CH3:
		enableChannel(3);
		break;
	case Channels::CH4:
		enableChannel(4);
		break;
	default:
		break;	
	}
}

void SoundProcessor::rollOver(){
	if(masterSoundEnable){ // Update the 512 Hz frame sequencer.
		ch1.update(sequencerTicks);
		if(ch1.poll())
			disableChannel(1);
		ch2.update(sequencerTicks);
		if(ch2.poll())
			disableChannel(2);
		ch3.update(sequencerTicks);
		if(ch3.poll())
			disableChannel(3);
		ch4.update(sequencerTicks);
		if(ch4.poll())
			disableChannel(4);
		sequencerTicks++;
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
