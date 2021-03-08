#ifndef AUDIO_UNIT_HPP
#define AUDIO_UNIT_HPP

#include "UnitTimer.hpp"
#include "LengthCounter.hpp"

class AudioUnit : public UnitTimer {
public:
	/** Default constructor (maximum audio length of 64)
	  */
	AudioUnit() :
		UnitTimer(),
		bDisableThisChannel(false),
		bEnableThisChannel(false),
		length(64)
	{ 
	}

	/** Maximum audio length constructor
	  */
	AudioUnit(const unsigned short& maxLength, const unsigned int& master=8) :
		UnitTimer(master),
		bDisableThisChannel(false),
		bEnableThisChannel(false),
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
	
	/** Trigger the channel DAC (called when register nrx4 is written to)
	  * Enable length counter if nrx4 bit 6 is set and trigger DAC if bit 7 is set
	  * @return True if this channel should now be disabled OR enabled and return false otherwise
	  */
	bool powerOn(const unsigned char& nrx4, const unsigned int& nSequencerTicks);

	/** Return a 4-bit sample from the current state of the audio waveform
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
	
	/** Reset all values and flags
	  */
	virtual void reset();
	
protected:
	bool bDisableThisChannel; ///< Channel will be disabled immediately
	
	bool bEnableThisChannel; ///< Channel will be enabled immediately

	LengthCounter length; ///< Sound length counter
	
	/** Enable this channel
	  */
	virtual void userEnable() { }
	
	/** Disable this channel
	  */
	virtual void userDisable() { }

	/** Reset all values and flags
	  */
	virtual void userReset() { }
	
	/** Method called from powerOn() when the channel was triggered and will be enabled
	  */
	virtual void channelWillBeEnabled() { }
};

#endif
