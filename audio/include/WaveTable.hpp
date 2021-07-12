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
		nBuffer(0)
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
		nBuffer(0)
	{
		nPeriodMultiplier = 2;
		nWavelengthPeriod = 32; // 32 samples per wavelength period
	}
	
	/** Get the current WAVE sample index
	  */
	unsigned char getIndex() const { 
		return nIndex; 
	}
	
	/** Write to WAVE memory at current buffer index
	  */
	void writeBufferIndex(const unsigned char& val){
		data[nIndex / 2] = val;
	}
	
	/** Get the current WAVE sample
	  */
	unsigned char getBuffer() const { 
		return nBuffer; 
	}
	
	/** Reset current WAVE index and buffer the first sample
	  */
	void clearBuffer() {
		nBuffer = data[(nIndex = 0)];
	}
	
	/** Set wave table volume level
	  *  0: Mute (0%)
	  *  1: 100%
	  *  2: 50%
	  *  3: 25%
	  */
	void setVolumeLevel(const unsigned char& nVolume){
		volume.setVolume(nVolume);
	}
	
	/** Return a sample from the current state of the audio waveform
	  */
	unsigned char sample() override ;

	/** Clock the timer
	  * Called by master sound countroller at a rate of 512 Hz
	  * @param sequencerTicks The number of 512 Hz ticks since the last rollover
	  */
	void clockSequencer(const unsigned int& sequencerTicks) override ;

	/** Handle timer trigger events whenever register NRx4 is written to
	  */	
	void trigger(const unsigned int& nTicks) override ;

private:
	unsigned char* data; ///< Pointer to audio sample data
	
	unsigned char nIndex; ///< Wave table index
	
	unsigned char nBuffer; ///< Sample buffer
	
	/** Method called when unit timer clocks over (every 16 system clock ticks)
	  */	
	void rollover() override ;
	
	/** Do not enable length counter, it must be enabled via writing to APU registers
	  */
	void userEnable() override { }
	
	/** Do not disable length counter, as it is still clocked when DAC is disabled
	  */
	void userDisable() override { }
	
	/** Upon powering down, reset the WAVE index and sample buffer
	  */
	void userReset() override ;
};

#endif
