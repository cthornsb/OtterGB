#ifndef SOUND_HPP
#define SOUND_HPP

#include <memory>

#include "SystemComponent.hpp"
#include "SystemTimer.hpp"

#include "SoundMixer.hpp"
#include "SquareWave.hpp"
#include "WaveTable.hpp"
#include "ShiftRegister.hpp"

#include "SystemRegisters.hpp"

class SoundManager;

class SoundBuffer;

enum class Channels {
	CH1, // Square 1
	CH2, // Square 2
	CH3, // Wave
	CH4  // Noise
};

class SoundProcessor : public SystemComponent, public ComponentTimer {
public:
	/** Default constructor
	  */
	SoundProcessor();

	/** Get pointer to output audio mixer
	  */
	SoundMixer* getMixer(){
		return &mixer;
	}

	/** Return true if the APU is enabled (i.e. if it is powered up) and return false otherwise
	  */
	bool isEnabled() const {
		return masterSoundEnable;
	}
	
	/** Return true if the specified channel output is enabled and return false otherwise
	  * Not to be confused with isDacEnabled(const int&) which returns whether or not the channel DAC is powered
	  */
	bool isChannelEnabled(const int& ch) const ;
	
	/** Return true if the specified channel DAC is powered up and return false otherwise
	  * Not to be confused with isChannelEnabled(const int&) which returns whether or not channel output is enabled
	  */
	bool isDacEnabled(const int& ch) const ;
	
	/** Get the current remaining length (if length counter is active) for an audio channel
	  * Returned length is in 256 Hz frame sequencer ticks (3.9 ms ticks)
	  */
	unsigned short getChannelLength(const int& ch) const ;
	
	/** Get the current period for an audio channel in 4 MHz APU ticks
	  */
	unsigned short getChannelPeriod(const int& ch) const ;
	
	/** Get the current frequency for an audio channel in Hz
	  */
	float getChannelFrequency(const int& ch) const ;
	
	/** Set the audio interface pointer
	  */
	void setAudioInterface(SoundManager* ptr){
		audio = ptr;
	}

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

	/** Pause audio output (if enabled)
	  */
	void pause();
	
	/** Resume audio output (if enabled)
	  */
	void resume();

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
	SoundManager* audio; ///< Main system audio handler

	SquareWave ch1; ///< Channel 1 (square w/ frequency sweep)

	SquareWave ch2; ///< Channel 2 (square)
	
	WaveTable ch3; ///< Channel 3 (wave)
	
	ShiftRegister ch4; ///< Channel 4 (noise)

	SoundMixer mixer; ///< Audio output mixer
	
	SoundBuffer* buffer; ///< Pointer to audio sample buffer (singleton)
	
	unsigned char wavePatternRAM[16];
	
	bool masterSoundEnable;
	
	unsigned int sequencerTicks;
	
	void handleTriggerEnable(const int& ch);

	const AudioUnit* getAudioUnit(const int& ch) const ;

	virtual void rollOver();
};

#endif
