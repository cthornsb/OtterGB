#ifndef SQUARE_WAVE_HPP
#define SQUARE_WAVE_HPP

#include <memory>

#include "AudioUnit.hpp"
#include "VolumeEnvelope.hpp"
#include "FrequencySweep.hpp"

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
		nPeriodMultiplier = 4;
	}
	
	/** Frequency sweep constructor
	  */
	SquareWave(FrequencySweep* sweep);

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
	virtual void clockSequencer(const unsigned int& sequencerTicks);
	
	/** Handle timer trigger events whenever register NRx4 is written to
	  */	
	virtual void trigger();

private:
	bool bSweepEnabled; ///< Flag indicating frequency sweep is enabled on this channel

	unsigned char nDuty; ///< Square waveform duty cycle

	unsigned char nWaveform; ///< Current square audio waveform
	
	VolumeEnvelope volume; ///< Channel's volume envelope
	
	std::unique_ptr<FrequencySweep> frequency; ///< Channel's frequency sweep (if enabled, otherwise null)
	
	/** Enable the length counter, volume envelope, and frequency sweep (if in use)
	  */
	virtual void userEnable();
	
	/** Disable the length counter, volume envelope, and frequency sweep (if in use)
	  */
	virtual void userDisable();
	
	/** Upon powering down, reset the volume envelope and sweep divider (if enabled)
	  */
	virtual void userReset();
	
	/** Check if sweep frequency calculation overflowed and disable the channel and DAC if it did,
	  * otherwise do nothing.
	  */
	virtual void channelWillBeEnabled();
	
	/** Method called when unit timer clocks over (every 8 system clock ticks)
	  */	
	virtual void rollover();
};

#endif
