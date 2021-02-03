#ifndef SOUND_HPP
#define SOUND_HPP

#include <memory>

#include "SoundManager.hpp"
#include "SystemComponent.hpp"
#include "SystemTimer.hpp"

#include "SystemRegisters.hpp"

enum class Channels {
	CH1, // Square 1
	CH2, // Square 2
	CH3, // Wave
	CH4  // Noise
};

class UnitTimer{
public:
	/** Default constructor
	  */
	UnitTimer() :
		bEnabled(false),
		nPeriod(0),
		nCounter(0),
		nFrequency(0)
	{
	}

	/** Constructor specifying the timer period
	  */
	UnitTimer(const unsigned short& period) :
		bEnabled(false),
		nPeriod(period),
		nCounter(0),
		nFrequency(0)
	{
	}
	
	/** Get the current period of the timer (computed from the channel's 11-bit frequency)
	  */
	unsigned short getPeriod() const { 
		return nPeriod; 
	}
	
	/** Get the reciprocal of the 11-bit frequency of the timer
	  */
	unsigned short getFrequency() const {
		return (2048 - nPeriod);
	}

	/** Get the actual frequency (in Hz)
	  */
	virtual float getRealFrequency() const {
		return (131072.f / (2048 - nFrequency)); // in Hz
	}

	/** Return true if the timer is enabled, and return false otherwise
	  */
	bool isEnabled() const {
		return bEnabled;
	}

	/** Set the period of the timer (in 512 Hz sequencer ticks)
	  */
	void setPeriod(const unsigned short& period) {
		nPeriod = period;
	}

	/** Set the frequency and period of the timer
	  */
	void setFrequency(const unsigned short& freq){
		nFrequency = (freq & 0x7ff);
		nPeriod = 2048 - nFrequency;
	}
	
	/** Set the frequency and period of the timer using two input bytes
	  */
	void setFrequency(const unsigned char& lowByte, const unsigned char& highByte){
		nFrequency = ((highByte & 0x7) << 8) + lowByte;
		nPeriod = 2048 - nFrequency;
	}

	/** Enable the timer
	  */
	void enable() { 
		bEnabled = true; 
		userEnable();
	}
	
	/** Disable the timer
	  */
	void disable() { 
		bEnabled = false; 
		userDisable();
	}

	/** Reset the counter to the period of the timer
	  */
	void reset() {
		nCounter = nPeriod;
	}

	/** Clock the timer, returning true if the phase rolled over and returning false otherwise
	  */
	bool clock();
	
	/** Reload the unit timer with its period
	  */
	void reload();

protected:
	bool bEnabled; ///< Timer enabled flag
	
	unsigned short nPeriod; ///< Period of timer
	
	unsigned short nCounter; ///< Current timer value
	
	unsigned short nFrequency; ///< 11-bit frequency (0 to 2047)
	
	/** Additional operations performed whenever enable() is called
	  */
	virtual void userEnable() { }
	
	/** Additional operations performed whenever disable() is called
	  */
	virtual void userDisable() { }
};

class LengthCounter{
public:
	/** Default constructor (maximum length of 64 ticks)
	  */
	LengthCounter() :
		bEnabled(false),
		bRefilled(false),
		nMaximum(64),
		nCounter(0)
	{
	}
	
	/** Maximum length constructor
	  */
	LengthCounter(const unsigned short& maxLength) :
		bEnabled(false),
		bRefilled(false),
		nMaximum(maxLength),
		nCounter(0)
	{
	}
	
	/** Get the remaining audio length 
	  */
	unsigned short getLength() const { 
		return nCounter; 
	}

	/** Return true if the length counter is enabled
	  */	
	bool isEnabled() const { 
		return bEnabled; 
	}
	
	/** Return true if the length counter was automatically refilled when triggered with a length of zero
	  */
	bool wasRefilled() const {
		return bRefilled;
	}
	
	/** Set the audio length
	  */
	void setLength(const unsigned char& length){
		nCounter = nMaximum - length;
	}
	
	/** Enable length counter
	  */
	void enable() { 
		bEnabled = true; 
	}
	
	/** Disable length counter
	  */
	void disable() { 
		bEnabled = false; 
	}
	
	/** Clock the length counter
	  * @return True if the length counter clocked to zero and return false otherwise
	  */
	bool clock();
	
	/** Trigger the length counter
	  * If the current length is zero, the length is refilled with the maximum
	  */
	void trigger();
	
private:
	bool bEnabled; ///< Length counter enabled flag
	
	bool bRefilled; ///< Flag indicating that length counter was refilled with maximum length on most recent trigger

	unsigned short nMaximum; ///< Maximum audio length counter

	unsigned short nCounter; ///< Current audio length counter
};

class VolumeEnvelope{
public:
	VolumeEnvelope() :
		bEnabled(false),
		bAdd(true),
		nPeriod(0),
		nVolume(0),
		nCounter(0)
	{ 
	}

	bool isEnabled() const { 
		return bEnabled; 
	}
	
	void enable() { 
		bEnabled = true; 
	}
	
	void disable() { 
		bEnabled = false; 
	}
	
	unsigned char getPeriod() const { return nPeriod; }
	
	float getVolume() const { return (nVolume / 15.f); }

	void setPeriod(const unsigned char& period) { nPeriod = period; }
	
	void setVolume(const unsigned char& volume) { nVolume = volume; }
	
	void setAddMode(const bool& add) { bAdd = add; }

	bool clock();
	
	void trigger();

private:
	bool bEnabled;

	bool bAdd;

	unsigned char nPeriod;

	unsigned char nVolume;
	
	unsigned char nCounter;
};

class FrequencySweep{
public:
	FrequencySweep() :
		bEnabled(false),
		bOverflow(false),
		bOverflow2(false),
		bNegate(false),
		nTimer(0),
		nPeriod(0),
		nShift(0),
		nCounter(0),
		nShadowFrequency(0),
		nNewFrequency(0),
		nSequencerTicks(0),
		extTimer(0x0)
	{
	}
	
	bool isEnabled() const { 
		return bEnabled; 
	}
	
	void enable() { 
		nSequencerTicks = 0; // Sync with the sequencer
		bEnabled = true; 
	}
	
	void disable() { 
		bEnabled = false; 
	}
	
	bool overflowed() const { 
		return bOverflow; 
	}
	
	bool overflowed2() const {
		return bOverflow2;
	}
	
	unsigned char getBitShift() const { 
		return nShift; 
	}
	
	unsigned short getNewFrequency() const { 
		return nShadowFrequency; 
	}

	void setNegate(const bool& negate) { 
		bNegate = negate; 
	}

	void setPeriod(const unsigned char& period) { 
		nPeriod = period; 
		if(!nCounter) // Handle going from period = 0 to period != 0 with zero counter
			nCounter = nPeriod;
	}
	
	void setBitShift(const unsigned char& shift) { 
		nShift = shift; 
	}
			
	void setUnitTimer(UnitTimer* timer) { 
		extTimer = timer; 
	}
	
	void updateShadowFrequency() { 
		nShadowFrequency = nNewFrequency; 
	}
	
	bool clock();
	
	void trigger();

	bool compute();

private:
	bool bEnabled;
	
	bool bOverflow;
	
	bool bOverflow2;
	
	bool bNegate;
	
	unsigned char nTimer; // Sweep timer
	
	unsigned char nPeriod; // Sweep period
	
	unsigned char nShift; // Sweep shift
	
	unsigned char nCounter;
	
	unsigned short nShadowFrequency;
	
	unsigned short nNewFrequency;
	
	unsigned int nSequencerTicks;
	
	UnitTimer* extTimer;
};

class AudioUnit : public UnitTimer{
public:
	/** Default constructor (maximum audio length of 64)
	  */
	AudioUnit() :
		nSequencerTicks(0),
		bDisableThisChannel(false),
		bOutputToSO1(false),
		bOutputToSO2(false),
		length(64)
	{ 
	}

	/** Maximum audio length constructor
	  */
	AudioUnit(const unsigned short& maxLength) :
		nSequencerTicks(0),
		bDisableThisChannel(false),
		bOutputToSO1(false),
		bOutputToSO2(false),
		length(maxLength)
	{ 
	}

	/** Get pointer to channel's length counter
	  */
	LengthCounter* getLengthCounter() { 
		return &length; 
	}
	
	/** Get const pointer to channel's length counter
	  */
	const LengthCounter* getConstLengthCounter() const {
		return &length;
	}

	/** Get remaining audio length
	  */
	unsigned short getLength() const {
		return length.getLength();
	}

	/** Set audio length
	  */
	void setLength(const unsigned char& len){
		length.setLength(len);
	}

	/** Enable or disable sending this channel's audio to output terminal 1 (right channel)
	  */
	void sendToSO1(const bool& state=true) {
		bOutputToSO1 = state;
	}

	/** Enable or disable sending this channel's audio to output terminal 2 (left channel)
	  */
	void sendToSO2(const bool& state=true) {
		bOutputToSO2 = state;
	}
	
	/** Poll this channel's disable flag, indicating that the channel should be disabled by the master controller
	  * If the flag is set, it will be reset.
	  * @return True if the unit should be disabled and return false otherwise
	  */
	bool poll(){
		if(bDisableThisChannel){
			bDisableThisChannel = false;
			return true;
		}
		return false;
	}

	/** Return a sample from the current state of the audio waveform
	  */
	virtual unsigned char sample(){
		return 0;
	}
	
	/** Clock the timer
	  * Called by master sound countroller at a rate of 512 Hz
	  * @param sequencerTicks The number of 512 Hz ticks since the last rollover
	  */
	virtual void update(const unsigned int&) { }

	/** Handle timer trigger events whenever register NRx4 is written to
	  */	
	virtual void trigger() { 
		this->reload(); // Reload the main timer with its phase
		length.trigger();
	}

protected:
	unsigned int nSequencerTicks;

	bool bDisableThisChannel; ///< Channel will be disabled immediately

	bool bOutputToSO1; ///< Output audio to SO1 terminal

	bool bOutputToSO2; ///< Output audio to SO2 terminal

	LengthCounter length; ///< Sound length counter
	
	/** Enable the length counter
	  */
	virtual void userEnable(){
		//nSequencerTicks = 0;
		length.enable();
	}
	
	/** Disable the length counter
	  */
	virtual void userDisable(){
		length.disable();
	}
};

class SquareWave : public AudioUnit {
public:
	/** Default constructor (no frequency sweep)
	  */
	SquareWave() :
		AudioUnit(),
		bSweepEnabled(false),
		nWaveform(0),
		volume(),
		frequency()
	{
	}
	
	/** Frequency sweep constructor
	  */
	SquareWave(FrequencySweep* sweep) :
		AudioUnit(),
		bSweepEnabled(true),
		nWaveform(0),
		volume(),
		frequency(sweep)
	{
		frequency->setUnitTimer(this);
	}	

	/** Get pointer to this channel's volume envelope
	  */
	VolumeEnvelope* getVolumeEnvelope() { 
		return &volume; 
	}

	/** Get pointer to this channel's volume envelope
	  */
	FrequencySweep* getFrequencySweep() { 
		return (bSweepEnabled ? frequency.get() : 0x0); 
	}

	/** Set square wave duty cycle
	  *  0: 12.5%
	  *  1: 25%
	  *  2: 50%
	  *  3: 75%
	  */
	void setWaveDuty(const unsigned char& duty);
	
	/** Return a sample from the current state of the audio waveform
	  */
	virtual unsigned char sample();

	/** Clock the timer
	  * Called by master sound countroller at a rate of 512 Hz
	  * @param sequencerTicks The number of 512 Hz ticks since the last rollover
	  */
	virtual void update(const unsigned int& sequencerTicks);
	
	/** Handle timer trigger events whenever register NRx4 is written to
	  */	
	virtual void trigger();

private:
	bool bOutputToSO1; ///< Output audio to SO1 terminal

	bool bOutputToSO2; ///< Output audio to SO2 terminal

	bool bSweepEnabled; ///< Flag indicating frequency sweep is enabled on this channel

	unsigned char nWaveform; ///< Current square audio waveform
	
	VolumeEnvelope volume; ///< Channel's volume envelope
	
	std::unique_ptr<FrequencySweep> frequency; ///< Channel's frequency sweep (if enabled, otherwise null)
	
	/** Enable the length counter, volume envelope, and frequency sweep (if in use)
	  */
	virtual void userEnable(){
		nSequencerTicks = 0;
		length.enable();
		volume.enable();
		if(bSweepEnabled)
			frequency->enable();
	}
	
	/** Disable the length counter, volume envelope, and frequency sweep (if in use)
	  */
	virtual void userDisable(){
		length.disable();
		volume.disable();
		if(bSweepEnabled)
			frequency->disable();
	}
};

class WaveTable : public AudioUnit {
public:
	/** Default constructor
	  */
	WaveTable() :
		AudioUnit(256),
		data(0x0),
		nIndex(0),
		nBuffer(0),
		nVolume(0)
	{
	}
	
	/** Constructor with pointer to audio sample data
	  */
	WaveTable(unsigned char* ptr) :
		AudioUnit(256),
		data(ptr),
		nIndex(0),
		nBuffer(0),
		nVolume(0)
	{
	}
	
	/** Set wave table volume level
	  *  0: Mute (0%)
	  *  1: 100%
	  *  2: 50%
	  *  3: 25%
	  */
	void setVolumeLevel(const unsigned char& volume){
		nVolume = volume;
	}
	
	/** Get the actual unit frequency (in Hz)
	  */
	virtual float getRealFrequency() const {
		return (65536.f / (2048 - nFrequency)); // in Hz
	}
	
	/** Return a sample from the current state of the audio waveform
	  */
	virtual unsigned char sample();

	/** Clock the timer
	  * Called by master sound countroller at a rate of 512 Hz
	  * @param sequencerTicks The number of 512 Hz ticks since the last rollover
	  */
	virtual void update(const unsigned int& sequencerTicks);
	
	/** Handle timer trigger events whenever register NRx4 is written to
	  */	
	virtual void trigger();

private:
	bool bOutputToSO1; ///< Output audio to SO1 terminal

	bool bOutputToSO2; ///< Output audio to SO2 terminal

	unsigned char* data; ///< Pointer to audio sample data
	
	unsigned char nIndex; ///< Wave table index
	
	unsigned char nBuffer; ///< Sample buffer

	unsigned char nVolume; ///< Current output volume
};

class ShiftRegister : public AudioUnit {
public:
	/** Default constructor
	  */
	ShiftRegister() :
		AudioUnit(),
		bWidthMode(false),
		nClockShift(0),
		nDivisor(0),
		reg(0x7fff),
		volume()
	{ 
	}
	
	/** Set current width mode
	  * 0: 15-bit shift register (default)
	  * 1: 7-bit shift register
	  * Enabling 7-bit mode results in a more periodic sounding noise output than 15-bit mode
	  */
	void setWidthMode(const bool& mode) { 
		bWidthMode = mode; 
	}
	
	/** Set current clock shift such that noise frequency is equal to F = 524288 / divisor / 2^(shift + 1) Hz
	  */
	void setClockShift(const unsigned char& shift) { 
		nClockShift = shift; 
		updatePhase(); // Update timer phase
	}
	
	/** Set current divisor value such that noise frequency is equal to F = 524288 / divisor / 2^(shift + 1) Hz
	  */
	void setDivisor(const unsigned char& divisor) { 
		nDivisor = divisor; 
		updatePhase(); // Update timer phase
	}

	/** Get pointer to this channel's volume envelope
	  */
	VolumeEnvelope* getVolumeEnvelope() { 
		return &volume; 
	}

	/** Get the actual unit frequency (in Hz)
	  */
	virtual float getRealFrequency() const;

	/** Return a sample from the current state of the audio waveform
	  */
	virtual unsigned char sample();

	/** Clock the timer
	  * Called by master sound countroller at a rate of 512 Hz
	  * @param sequencerTicks The number of 512 Hz ticks since the last rollover
	  */
	virtual void update(const unsigned int& sequencerTicks);
	
	/** Handle timer trigger events whenever register NRx4 is written to
	  */	
	virtual void trigger();

private:
	bool bOutputToSO1; ///< Output audio to SO1 terminal

	bool bOutputToSO2; ///< Output audio to SO2 terminal

	bool bWidthMode; ///< Shift register width mode (0: 15-bit, 1: 7-bit)

	unsigned char nClockShift; ///< Current clock shift value (0 to 15)
	
	unsigned char nDivisor; ///< Current divisor value (0 to 7)

	unsigned short reg; ///< Shift register 15-bit waveform
	
	VolumeEnvelope volume; ///< Channel's volume envelope
	
	/** Update the timer phase after modifying the divsor or shift values
	  */
	void updatePhase();
	
	/** Enable the length counter and volume envelope
	  */
	virtual void userEnable(){
		length.enable();
		volume.enable();
	}
	
	/** Disable the length counter and volume envelope
	  */
	virtual void userDisable(){
		length.disable();
		volume.disable();
	}
};

class SoundProcessor : public SystemComponent, public ComponentTimer {
public:
	SoundProcessor();

	/** Disable audio channel (indexed from 1)
	  */
	void disableChannel(const int& ch);
	
	/** Disable audio channel
	  */
	void disableChannel(const Channels& ch);

	/** Enable audio channel (indexed from 1)
	  */
	void enableChannel(const int& ch);

	/** Enable audio channel
	  */	
	void enableChannel(const Channels& ch);

	// The sound controller has no associated RAM, so return false to avoid trying to access it.
	virtual bool preWriteAction(){ return false; }

	// The sound controller has no associated RAM, so return false to avoid trying to access it.	
	virtual bool preReadAction(){ return false; }

	/** Check that the specified APU register may be written to.
	  * @param reg Register address.
	  * @return True if the APU is powered or the address is the APU control register or in wave RAM.
	  */
	virtual bool checkRegister(const unsigned short &reg);

	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	virtual bool readRegister(const unsigned short &reg, unsigned char &val);

	virtual bool onClockUpdate();
	
	virtual void defineRegisters();

private:
	std::unique_ptr<SoundManager> audio; ///< System audio interface

	// Channel 1 (square w/ sweep)
	SquareWave ch1;

	// Channel 2 (square)
	SquareWave ch2;
	
	// Channel 3 (wave)
	WaveTable ch3;

	// Channel 4 (noise)
	ShiftRegister ch4;

	// Master control registers
	unsigned char outputLevelSO1;
	unsigned char outputLevelSO2;
	unsigned char wavePatternRAM[16];
	
	bool masterSoundEnable;
	
	unsigned int sequencerTicks;

	virtual void rollOver();
};

#endif
