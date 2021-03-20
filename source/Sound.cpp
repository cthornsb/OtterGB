#include "Support.hpp"
#include "SystemGBC.hpp"
#include "SoundManager.hpp"
#include "Sound.hpp"
#include "FrequencySweep.hpp"
#include "MidiFile.hpp"

/////////////////////////////////////////////////////////////////////
// class SoundProcessor
/////////////////////////////////////////////////////////////////////

SoundProcessor::SoundProcessor() : 
	SystemComponent("APU", 0x20555041), // "APU "
	ComponentTimer(2048), // 512 Hz sequencer
	bMasterSoundEnable(false),
	bRecordMidi(false),
	audio(&SoundManager::getInstance()),
	mixer(audio->getAudioMixer()),	
	ch1(new FrequencySweep()),
	ch2(),
	ch3(wavePatternRAM),
	ch4(),
	wavePatternRAM(),
	nSequencerTicks(0),
	nMidiClockTicks(0),
	midiFile()
{ 
}

bool SoundProcessor::checkRegister(const unsigned short &reg){
	// Check if the APU is powered down.
	// If the APU is powered down, only the APU control register and
	//  wave RAM may be written to.
	if(!bMasterSoundEnable && (reg < 0xFF30 || reg > 0xFF3F) && reg != 0xFF26) 
		return false;
	return true;
}

bool SoundProcessor::writeRegister(const unsigned short &reg, const unsigned char &val){
	switch(reg){
		/////////////////////////////////////////////////////////////////////
		// NR10-14 CHANNEL 1 (square w/ sweep)
		/////////////////////////////////////////////////////////////////////
		case 0xFF10: // NR10 ([TONE] Channel 1 sweep register)
			ch1.getFrequencySweep()->setPeriod(rNR10->getBits(4,6));
			ch1.getFrequencySweep()->setBitShift(rNR10->getBits(0,2));
			if(!ch1.getFrequencySweep()->setNegate(rNR10->getBit(3))){
				// Switching from negative to positive disables the channel
				disableChannel(1); // Disable channel and DAC
			}
			break;
		case 0xFF11: // NR11 ([TONE] Channel 1 sound length / wave pattern duty)
			ch1.setWaveDuty(rNR11->getBits(6,7)); // Wave pattern duty (see below)
			ch1.setLength(rNR11->getBits(0,5));
			break;
		case 0xFF12: // NR12 ([TONE] Channel 1 volume envelope)
			ch1.getVolumeEnvelope()->update(rNR12->getValue());
			if(rNR12->getBits(3,7) == 0){ // Disable DAC
				disableChannel(1); // Disable channel and DAC
			}
			else{ // Enable DAC (but do not enable channel)
				ch1.enable(); // Enable DAC
			}
			break;
		case 0xFF13: // NR13 ([TONE] Channel 1 frequency low)
			ch1.setFrequency((rNR14->getBits(0,2) << 8) + rNR13->getValue());
			break;
		case 0xFF14: // NR14 ([TONE] Channel 1 frequency high)
			ch1.setFrequency((rNR14->getBits(0,2) << 8) + rNR13->getValue());
			if(ch1.powerOn(rNR14->getValue(), nSequencerTicks))
				handleTriggerEnable(1);
			break;
		/////////////////////////////////////////////////////////////////////
		// NR20-24 CHANNEL 2 (square)
		/////////////////////////////////////////////////////////////////////
		case 0xFF15: // Not used
			break;
		case 0xFF16: // NR21 ([TONE] Channel 2 sound length / wave pattern duty)
			ch2.setWaveDuty(rNR21->getBits(6,7)); // Wave pattern duty (see below)
			ch2.setLength(rNR21->getBits(0,5));
			break;
		case 0xFF17: // NR22 ([TONE] Channel 2 volume envelope
			ch2.getVolumeEnvelope()->update(rNR22->getValue());
			if(rNR22->getBits(3,7) == 0){ // Disable DAC
				disableChannel(2); // Disable channel and DAC
			}
			else{ // Enable DAC (but do not enable channel)
				ch2.enable(); // Enable DAC
			}
			break;
		case 0xFF18: // NR23 ([TONE] Channel 2 frequency low)
			ch2.setFrequency((rNR24->getBits(0,2) << 8) + rNR23->getValue());
			break;
		case 0xFF19: // NR24 ([TONE] Channel 2 frequency high)
			ch2.setFrequency((rNR24->getBits(0,2) << 8) + rNR23->getValue());
			if(ch2.powerOn(rNR24->getValue(), nSequencerTicks))
				handleTriggerEnable(2);
			break;
		/////////////////////////////////////////////////////////////////////
		// NR30-34 CHANNEL 3 (wave)
		/////////////////////////////////////////////////////////////////////
		case 0xFF1A: // NR30 ([TONE] Channel 3 sound on/off)
			if(!rNR30->getBit(7)){ // Disable channel and power off DAC
				disableChannel(3); // Disable channel and DAC
			}
			else{ // Enable DAC but do not enable channel
				ch3.clearBuffer(); // Reset WAVE index and buffer sample
				ch3.enable();
			}
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
			if(ch3.powerOn(rNR34->getValue(), nSequencerTicks))
				handleTriggerEnable(3);
			break;
		/////////////////////////////////////////////////////////////////////
		// NR40-44 CHANNEL 4 (noise)
		/////////////////////////////////////////////////////////////////////
		case 0xFF1F: // Not used
			break;
		case 0xFF20: // NR41 ([NOISE] Channel 4 sound length)
			ch4.setLength(rNR41->getBits(0,5));
			break;
		case 0xFF21: // NR42 ([NOISE] Channel 4 volume envelope)
			ch4.getVolumeEnvelope()->update(rNR42->getValue());
			if(rNR42->getBits(3,7) == 0){ // Disable DAC
				disableChannel(4); // Disable channel and DAC
			}
			else{ // Enable DAC (but do not enable channel)
				ch4.enable(); // Enable DAC
			}
			break;
		case 0xFF22: // NR43 ([NOISE] Channel 4 polynomial counter)
			ch4.setClockShift(rNR43->getBits(4,7)); // Shift clock frequency (s)
			ch4.setWidthMode(rNR43->getBit(3));     // Counter step/width [0: 15-bits, 1: 7-bits]
			ch4.setDivisor(rNR43->getBits(0,2));    // Dividing ratio of frequency (r)
			break;
		case 0xFF23: // NR44 ([NOISE] Channel 4 counter / consecutive, initial)
			if(ch4.powerOn(rNR44->getValue(), nSequencerTicks))
				handleTriggerEnable(4);
			break;
		/////////////////////////////////////////////////////////////////////
		// NR50-52 MASTER CONTROL
		/////////////////////////////////////////////////////////////////////
		case 0xFF24: // NR50 (Channel control / ON-OFF / volume)
			// Ignore Vin since we do not emulate it
			mixer->setOutputLevels(rNR50->getBits(4,6) / 7.f, rNR50->getBits(0,2) / 7.f); // 3-bit volumes
			break;
		case 0xFF25: // NR51 (Select sound output)
			// Left channel
			mixer->setInputToOutput(3, 0, rNR51->getBit(7)); // ch4
			mixer->setInputToOutput(2, 0, rNR51->getBit(6)); // ch3
			mixer->setInputToOutput(1, 0, rNR51->getBit(5)); // ch2
			mixer->setInputToOutput(0, 0, rNR51->getBit(4)); // ch1
			// Right channel
			mixer->setInputToOutput(3, 1, rNR51->getBit(3)); // ch4
			mixer->setInputToOutput(2, 1, rNR51->getBit(2)); // ch3
			mixer->setInputToOutput(1, 1, rNR51->getBit(1)); // ch2
			mixer->setInputToOutput(0, 1, rNR51->getBit(0)); // ch1
			break;
		case 0xFF26: // NR52 (Sound ON-OFF)
			bMasterSoundEnable = rNR52->getBit(7);
			if(bMasterSoundEnable){ // Power on the frame sequencer
				// Resume audio output
				resume(); 
				// Reset frame sequencer (the 512 Hz clock is always running, even while APU is powered down)
				nSequencerTicks = 0;
			}
			else{ // Power down APU
				powerDown();
			}
			break;
		default:
			if (reg >= 0xFF27 && reg <= 0xFF2F) { // Not used
			}
			else if (reg >= 0xFF30 && reg <= 0xFF3F) { // [Wave] pattern RAM
				// Contains 32 4-bit samples played back upper 4 bits first
				if(isChannelEnabled(3)) // WAVE enabled (write to current WAVE index)
					ch3.writeBufferIndex(val);
				else
					wavePatternRAM[reg - 0xFF30] = val; //rWAVE[reg - 0xFF30]->getValue();
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
				if(isChannelEnabled(3)) // WAVE enabled (read back from current WAVE index)
					dest = ch3.getBuffer();
				else
					dest = wavePatternRAM[reg - 0xFF30];

			}
			else {
				return false;
			}
	}
	return true;
}

bool SoundProcessor::onClockUpdate(){
	if(!bEnabled) // If timer not enabled
		return false;

	if(bMasterSoundEnable){
		// Clock audio units (4 MHz clock)
		for(int i = 0; i < 4; i++){
			if(ch1.clock())
				mixer->setInputSample(0, ch1.sample());
			if(ch2.clock())
				mixer->setInputSample(1, ch2.sample());
			if(ch3.clock())
				mixer->setInputSample(2, ch3.sample());
			if(ch4.clock())
				mixer->setInputSample(3, ch4.sample());
		}
		// Clock 16 kHz mixer
		if(mixer->clock()){ // New sample is pushed onto the sample FIFO buffer
			if(bRecordMidi)
				nMidiClockTicks++;
		}
	}
	
	// Update the 512 Hz frame sequencer.
	if(++nCyclesSinceLastTick >= nPeriod){
		this->reload();
		if(bMasterSoundEnable)
			this->rollover();
		return true;
	}
	
	return false;
}

bool SoundProcessor::isChannelEnabled(const int& ch) const {
	if(ch < 1 || ch > 4)
		return false;
	return (bMasterSoundEnable && rNR52->getBit(ch - 1));
}

bool SoundProcessor::isDacEnabled(const int& ch) const {
	const AudioUnit* unit = getConstAudioUnit(ch);
	if(!unit)
		return false;
	return unit->isEnabled();
}

unsigned short SoundProcessor::getChannelLength(const int& ch) const {
	const AudioUnit* unit = getConstAudioUnit(ch);
	if(!unit)
		return false;
	return unit->getLength();
}

unsigned short SoundProcessor::getChannelPeriod(const int& ch) const {
	const AudioUnit* unit = getConstAudioUnit(ch);
	if(!unit)
		return false;
	return unit->getPeriod();
}

float SoundProcessor::getChannelFrequency(const int& ch) const {
	const AudioUnit* unit = getConstAudioUnit(ch);
	if(!unit)
		return false;
	return unit->getRealFrequency();
}

void SoundProcessor::disableChannel(const int& ch){
	if(ch < 1 || ch > 4) // Invalid channel
		return;
	rNR52->resetBit(ch - 1); // Disable channel audio output
	getAudioUnit(ch)->disable(); // Disable DAC
	if (bRecordMidi) { // Add a note to the output midi file
		// Disable the note, if one is currently playing
		// Note that midi ch 10 (index = 9) is typically used for percussion
		midiFile->release((ch < 4 ? ch - 1 : 9), nMidiClockTicks);
	}
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
	rNR52->setBit(ch - 1); // Enable channel audio output
}

void SoundProcessor::enableChannel(const Channels& ch){
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

void SoundProcessor::pause(){
	if (audio && audio->isRunning())
		audio->stop(); // Stop audio interface
}

void SoundProcessor::resume(){
	if (audio && !audio->isRunning())
		audio->start(); // Start audio interface
}

void SoundProcessor::startMidiFile(const std::string& filename/*="out.mid"*/){
	if(bRecordMidi) // Midi recording already in progress
		return;
	midiFile.reset(new MidiFile::MidiFileReader(filename, sys->getRomFilename())); // New midi file recorder
	// 16384 audio samples per second
	// 120 metronome ticks per minute by default
	// 24 midi clocks per metronome tick
	//  => 2880 midi clocks per minute
	// So... 983040 audio samples per minute and 2880 midi clock ticks per minute
	//  => 341.33 audio samples per midi clock tick
	midiFile->setClockMultiplier(1 / 341.33f);
	bRecordMidi = true;
}

void SoundProcessor::stopMidiFile(){
	if(!bRecordMidi) // No midi recording in progress
		return;
	midiFile->finalize(nMidiClockTicks); 
	midiFile->write(); // Write midi file to disk
	bRecordMidi = false;
}

void SoundProcessor::powerDown(){
	// Pause audio output
	pause();
	
	// Disable channels
	disableChannel(4); // Ch 4 OFF
	disableChannel(3); // Ch 3 OFF
	disableChannel(2); // Ch 2 OFF
	disableChannel(1); // Ch 1 OFF
	for(unsigned char i = 0x10; i < 0x30; i++){ // Clear all sound registers (except NR52 and wave RAM)
		if(i == 0x26) // Skip NR52
			continue; 
		sys->clearRegister(i); // Set register to zero
	}
	
	// Reset DACs
	ch1.reset();
	ch2.reset();
	ch3.reset();
	ch4.reset();
}

bool SoundProcessor::handleTriggerEnable(const int& ch){
	if(ch < 1 || ch > 4)
		return false;
	AudioUnit* unit = getAudioUnit(ch);
	if(unit->pollEnable())
		enableChannel(ch);
	if(unit->pollDisable())
		disableChannel(ch);
	if(rNR52->getBit(ch - 1)){ // Trigger has enabled channel
		if (bRecordMidi) { // Add a note to the output midi file
			// If a note is currently pressed on this channel, the midi handler will automatically release it.
			// Note that midi ch 10 (index = 9) is typically used for percussion
			midiFile->press((ch < 4 ? ch - 1 : 9), nMidiClockTicks, unit->getRealFrequency());
		}
		return true;
	}
	return false;
}

void SoundProcessor::rollover(){
	// Update the 512 Hz frame sequencer.
	ch1.clockSequencer(nSequencerTicks);
	if(ch1.pollDisable())
		disableChannel(1);
	else if(ch1.frequencyUpdated()){ // Frequency was updated by frequency sweep, update frequency register
		unsigned short freq = ch1.getFrequency();
		rNR13->setValue((unsigned char)(freq & 0x00FF));
		rNR14->setBits(0, 2, (unsigned char)((freq & 0x0700) >> 8));		
	}
	ch2.clockSequencer(nSequencerTicks);
	if(ch2.pollDisable())
		disableChannel(2);
	ch3.clockSequencer(nSequencerTicks);
	if(ch3.pollDisable())
		disableChannel(3);
	ch4.clockSequencer(nSequencerTicks);
	if(ch4.pollDisable())
		disableChannel(4);
	nSequencerTicks++;
}

AudioUnit* SoundProcessor::getAudioUnit(const int& ch){
	switch(ch){
	case 1:
		return &ch1;
	case 2:
		return &ch2;
	case 3:
		return &ch3;
	case 4:
		return &ch4;
	default:
		break;	
	}
	return 0x0;	
}

const AudioUnit* SoundProcessor::getConstAudioUnit(const int& ch) const {
	switch(ch){
	case 1:
		return &ch1;
	case 2:
		return &ch2;
	case 3:
		return &ch3;
	case 4:
		return &ch4;
	default:
		break;	
	}
	return 0x0;
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

void SoundProcessor::userAddSavestateValues(){
	unsigned int sizeUShort = sizeof(unsigned short);
	unsigned int sizeBool = sizeof(bool);
	// ComponentTimer fields
	addSavestateValue(&nCyclesSinceLastTick, sizeUShort);
	addSavestateValue(&nPeriod,  sizeUShort);
	addSavestateValue(&nCounter, sizeUShort);
	addSavestateValue(&bEnabled, sizeBool);
	// SoundProcessor fields
	addSavestateValue(&bMasterSoundEnable, sizeBool);
	addSavestateValue(&bRecordMidi,        sizeBool);
	addSavestateValue(wavePatternRAM,   sizeof(unsigned char) * 16);
	addSavestateValue(&nSequencerTicks, sizeof(unsigned int));
	
	/*SquareWave ch1; ///< Channel 1 (square w/ frequency sweep)
	SquareWave ch2; ///< Channel 2 (square)
	WaveTable ch3; ///< Channel 3 (wave)
	ShiftRegister ch4; ///< Channel 4 (noise)*/
}

void SoundProcessor::onUserReset(){
	stopMidiFile(); // Stop midi file recording (if active)
	powerDown(); // Power off the APU
	reload(); // Reset APU clock
	mixer->reset(); // Reset mixer clock
	nSequencerTicks = 0;
}

