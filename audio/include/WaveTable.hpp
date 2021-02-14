#ifndef WAVE_TABLE_HPP
#define WAVE_TABLE_HPP

#include "AudioUnit.hpp"

class WaveTable : public AudioUnit {
public:
	/** Default constructor
	  */
	WaveTable() :
		AudioUnit(256, 16),
		data(0x0),
		nIndex(0),
		nBuffer(0),
		nVolume(0)
	{
		nPeriodMultiplier = 2;
		nWavelengthPeriod = 32; // 32 samples per wavelength period
	}
	
	/** Constructor with pointer to audio sample data
	  */
	WaveTable(unsigned char* ptr) :
		AudioUnit(256, 16),
		data(ptr),
		nIndex(0),
		nBuffer(0),
		nVolume(0)
	{
		nPeriodMultiplier = 2;
		nWavelengthPeriod = 32; // 32 samples per wavelength period
	}
	
	/** Get the current wave sample index
	  */
	unsigned char getIndex() const { 
		return nIndex; 
	}
	
	/** Get the current wave sample
	  */
	unsigned char getBuffer() const { 
		return nBuffer; 
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
	unsigned char* data; ///< Pointer to audio sample data
	
	unsigned char nIndex; ///< Wave table index
	
	unsigned char nBuffer; ///< Sample buffer

	unsigned char nVolume; ///< Current output volume
	
	/** Method called when unit timer clocks over (every 16 system clock ticks)
	  */	
	virtual void rollover();
	
	/** Enable length counter
	  */
	virtual void userEnable();
	
	/** Disable length counter
	  */
	virtual void userDisable();
	
	/** Upon powering down, reset the WAVE index and sample buffer
	  */
	virtual void userReset();
};

#endif
