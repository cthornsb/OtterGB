#ifndef SOUND_HPP
#define SOUND_HPP

#include <memory>

#include "SoundManager.hpp"
#include "SystemComponent.hpp"
#include "SystemTimer.hpp"

#include "SquareWave.hpp"
#include "WaveTable.hpp"
#include "ShiftRegister.hpp"

#include "SystemRegisters.hpp"

enum class Channels {
	CH1, // Square 1
	CH2, // Square 2
	CH3, // Wave
	CH4  // Noise
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
	
	void handleTriggerEnable(const int& ch);

	virtual void rollOver();
};

#endif
