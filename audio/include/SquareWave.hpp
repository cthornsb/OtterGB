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
		bFrequencyUpdated(false),
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

	bool frequencyUpdated() const {
		return bFrequencyUpdated;
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
	bool bSweepEnabled; ///< Flag indicating frequency sweep is enabled on this channel

	bool bFrequencyUpdated; ///< Flag indicating that the channel frequency was updated by the frequency sweep

	unsigned char nDuty; ///< Square waveform duty cycle

	unsigned char nWaveform; ///< Current square audio waveform
	
	VolumeEnvelope volume; ///< Channel's volume envelope
	
	std::unique_ptr<FrequencySweep> frequency; ///< Channel's frequency sweep (if enabled, otherwise null)

	/** Enable volume envelope
	  * Do not automatically enable the length counter or frequency sweep as
	  * they must be enabled independently by writing to APU registers.
	  */
	void userEnable() override ;
	
	/** Disable volume envelope.
	  * Do not disable length counter or frequency sweep as they may still be clocked
	  * while the DAC is disabled.
	  */
	void userDisable() override ;
	
	/** Upon powering down, reset the volume envelope and sweep divider (if enabled)
	  */
	void userReset() override ;
	
	/** Check if sweep frequency calculation overflowed and disable the channel and DAC if it did,
	  * otherwise do nothing.
	  */
	void channelWillBeEnabled() override ;
	
	/** Method called when unit timer clocks over (every 8 system clock ticks)
	  */	
	void rollover() override ;
};

#endif
