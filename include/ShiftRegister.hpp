#ifndef SHIFT_REGISTER_HPP
#define SHIFT_REGISTER_HPP

#include "AudioUnit.hpp"
#include "VolumeEnvelope.hpp"

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
	virtual void clockSequencer(const unsigned int& sequencerTicks);
	
	/** Handle timer trigger events whenever register NRx4 is written to
	  */	
	virtual void trigger();

private:
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
	
	/** Method called when unit timer clocks over (every N system clock ticks)
	  */	
	virtual void rollover();
};

#endif
