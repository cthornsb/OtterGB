#ifndef SOUND_HPP
#define SOUND_HPP

#include <memory>

#include "SoundManager.hpp"
#include "SystemComponent.hpp"
#include "SystemTimer.hpp"

#include "SystemRegisters.hpp"

class LengthCounter{
public:
	LengthCounter() :
		bEnabled(false),
		nMaximum(64),
		nCounter(0)
	{
	}
	
	LengthCounter(const unsigned short& maxLength) :
		bEnabled(false),
		nMaximum(maxLength),
		nCounter(0)
	{
	}
	
	unsigned char getLength() const { 
		return nCounter; 
	}
	
	bool isEnabled() const { 
		return (bEnabled && nCounter != 0); 
	}
	
	void setLength(const unsigned char& length){
		nCounter = nMaximum - length;
	}
	
	void enable() { 
		bEnabled = true; 
	}
	
	void disable() { 
		bEnabled = false; 
	}
	
	bool onClockUpdate();
	
	void onTriggerEvent();
	
private:
	bool bEnabled;

	unsigned short nMaximum;

	unsigned short nCounter;
};

class VolumeEnvelope{
public:
	VolumeEnvelope() :
		bAdd(true),
		nPeriod(0),
		nVolume(0),
		nCounter(0)
	{ 
	}
	
	unsigned char getPeriod() const { return nPeriod; }
	
	float getVolume() const { return (nVolume / 15.f); }

	void setPeriod(const unsigned char& period) { nPeriod = period; }
	
	void setVolume(const unsigned char& volume) { nVolume = volume; }
	
	void setAddMode(const bool& add) { bAdd = add; }

	bool onClockUpdate();
	
	void onTriggerEvent();

private:
	bool bAdd;

	unsigned char nPeriod;

	unsigned char nVolume;
	
	unsigned char nCounter;
};

class FrequencySweep{
public:
	FrequencySweep() :
		bEnabled(false),
		bNegate(false),
		nTimer(0),
		nPeriod(0),
		nShift(0),
		nCounter(0),
		nShadowFrequency(0),
		frequency(0x0)
	{
	}
	
	bool isEnabled() const { return bEnabled; }

	void setNegate(const bool& negate) { bNegate = negate; }

	void setPeriod(const unsigned char& period) { nPeriod = period; }
	
	void setBitShift(const unsigned char& shift) { nShift = shift; }
			
	void setFrequency(float* freq) { frequency = freq; }
			
	bool onClockUpdate();
	
	void onTriggerEvent();

private:
	bool bEnabled;
	
	bool bNegate;
	
	unsigned char nTimer; // Sweep timer
	
	unsigned char nPeriod; // Sweep period
	
	unsigned char nShift; // Sweep shift
	
	unsigned char nCounter;
	
	unsigned short nShadowFrequency;
	
	float* frequency;
};

class SoundProcessor : public SystemComponent, public ComponentTimer {
public:
	SoundProcessor();

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

	// Channel 1 registers (square)
	FrequencySweep ch1FrequencySweep;
	LengthCounter ch1SoundLengthData;
	VolumeEnvelope ch1VolumeEnvelope;
	unsigned char ch1WavePatternDuty;
	float ch1Frequency; ///< Current 11-bit frequency
	bool ch1ToSO1;
	bool ch1ToSO2;

	// Channel 2 registers (square)
	LengthCounter ch2SoundLengthData;
	VolumeEnvelope ch2VolumeEnvelope;
	unsigned char ch2WavePatternDuty;
	float ch2Frequency; ///< Current 11-bit frequency
	bool ch2ToSO1;
	bool ch2ToSO2;
	
	// Channel 3 registers (wave)
	LengthCounter ch3SoundLengthData;
	unsigned char ch3OutputLevel;
	float ch3Frequency; ///< Current 11-bit frequency
	bool ch3ToSO1;
	bool ch3ToSO2;

	// Channel 4 registers (noise)
	LengthCounter ch4SoundLengthData;
	VolumeEnvelope ch4VolumeEnvelope;
	unsigned char ch4ShiftClockFreq;
	unsigned char ch4DividingRation;
	bool ch4DirectionEnv;
	bool ch4CounterStepWidth;
	bool ch4ToSO1;
	bool ch4ToSO2;

	// Master control registers
	unsigned char outputLevelSO1;
	unsigned char outputLevelSO2;
	unsigned char wavePatternRAM[16];
	bool masterSoundEnable;
	
	unsigned int sequencerTicks;

	void triggerLengthCounter();
	
	void triggerVolumeEnvelope();
		
	void triggerSweepTimer();
	
	virtual void rollOver();
};

#endif
