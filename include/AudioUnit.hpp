#ifndef AUDIO_UNIT_HPP
#define AUDIO_UNIT_HPP

#include "UnitTimer.hpp"
#include "LengthCounter.hpp"

class Register;

class AudioUnit : public UnitTimer {
public:
	/** Default constructor (maximum audio length of 64)
	  */
	AudioUnit() :
		UnitTimer(),
		bDisableThisChannel(false),
		bEnableThisChannel(false),
		bOutputToSO1(false),
		bOutputToSO2(false),
		length(64)
	{ 
	}

	/** Maximum audio length constructor
	  */
	AudioUnit(const unsigned short& maxLength, const unsigned int& master=8) :
		UnitTimer(master),
		bDisableThisChannel(false),
		bEnableThisChannel(false),
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
	bool pollDisable();
	
	/** Poll this channel's enable flag, indicating that the channel should be enabled by the master controller
	  * If the flag is set, it will be reset.
	  * @return True if the unit should be enabled and return false otherwise
	  */
	bool pollEnable();
	
	/**
	  */
	bool powerOn(const Register* nrx4, const unsigned int& nSequencerTicks);

	/** Return a sample from the current state of the audio waveform
	  */
	virtual unsigned char sample(){
		return 0;
	}
	
	/** Handle frame sequencer clocks
	  * Called by master sound countroller at a rate of 512 Hz
	  * @param sequencerTicks The number of 512 Hz ticks since the last rollover
	  */
	virtual void clockSequencer(const unsigned int&) { }

	/** Handle timer trigger events whenever register NRx4 is written to
	  */	
	virtual void trigger() { 
		length.trigger();
	}

	/** Enable the length counter
	  */
	virtual void enableLengthCounter(){
		length.enable();
	}
	
	/** Disable the length counter
	  */
	virtual void disableLengthCounter(){
		length.disable();
	}
	
protected:
	bool bDisableThisChannel; ///< Channel will be disabled immediately
	
	bool bEnableThisChannel; ///< Channel will be enabled immediately

	bool bOutputToSO1; ///< Output audio to SO1 terminal

	bool bOutputToSO2; ///< Output audio to SO2 terminal

	LengthCounter length; ///< Sound length counter
	
	/** Enable this channel
	  */
	virtual void userEnable() { }
	
	/** Disable this channel
	  */
	virtual void userDisable() { }
	
	/** Method called from powerOn() when the channel was triggered and will be enabled
	  */
	virtual void channelWillBeEnabled() { }
};

#endif
