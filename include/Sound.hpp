#ifndef SOUND_HPP
#define SOUND_HPP

#include <queue>

#include "SystemComponent.hpp"
#include "SystemTimer.hpp"

class FrameSequencer : public ComponentTimer {
public:
	FrameSequencer() : ComponentTimer(512) { }
	
	virtual bool onClockUpdate(const unsigned short &nCycles);
		
private:
	std::queue<int> events; ///< Sound event FIFO buffer
	
	void triggerLengthCounter(){ }
	
	void triggerVolumeEnvelope(){ }
		
	void triggerSweepTimer(){ }
};

class SoundProcessor : public SystemComponent, public ComponentTimer {
public:
	SoundProcessor();

	// The sound controller has no associated RAM, so return false to avoid trying to access it.
	virtual bool preWriteAction(){ return false; }

	// The sound controller has no associated RAM, so return false to avoid trying to access it.	
	virtual bool preReadAction(){ return false; }

	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	virtual bool readRegister(const unsigned short &reg, unsigned char &val);

	virtual bool onClockUpdate(const unsigned short &nCycles);

private:
	// Channel 1 registers
	unsigned char ch1SweepTime;
	unsigned char ch1SweepShift;
	unsigned char ch1WavePatternDuty;
	unsigned char ch1SoundLengthData;
	unsigned char ch1InitialVolumeEnv; 
	unsigned char ch1NumberSweepEnv; 
	unsigned char ch1FrequencyLow;
	unsigned char ch1FrequencyHigh;
	bool ch1SweepIncDec;
	bool ch1DirectionEnv;
	bool ch1Restart;
	bool ch1CounterSelect;
	bool ch1ToSO1;
	bool ch1ToSO2;

	// Channel 2 registers
	unsigned char ch2WavePatternDuty;
	unsigned char ch2SoundLengthData;
	unsigned char ch2InitialVolumeEnv; 
	unsigned char ch2NumberSweepEnv; 
	unsigned char ch2FrequencyLow;
	unsigned char ch2FrequencyHigh;
	bool ch2DirectionEnv;
	bool ch2Restart;
	bool ch2CounterSelect;
	bool ch2ToSO1;
	bool ch2ToSO2;
	
	// Channel 3 registers
	unsigned char ch3SoundLengthData;
	unsigned char ch3OutputLevel;
	unsigned char ch3FrequencyLow;
	unsigned char ch3FrequencyHigh;
	bool ch3Enable;
	bool ch3Restart;
	bool ch3CounterSelect;
	bool ch3ToSO1;
	bool ch3ToSO2;

	// Channel 4 registers
	unsigned char ch4SoundLengthData;
	unsigned char ch4InitialVolumeEnv;
	unsigned char ch4NumberSweepEnv;
	unsigned char ch4ShiftClockFreq;
	unsigned char ch4DividingRation;
	bool ch4DirectionEnv;
	bool ch4CounterStepWidth;
	bool ch4Restart;
	bool ch4CounterSelect;
	bool ch4ToSO1;
	bool ch4ToSO2;

	// Master control registers
	unsigned char outputLevelSO1;
	unsigned char outputLevelSO2;
	unsigned char wavePatternRAM[16];
	bool masterSoundEnable;
	
	FrameSequencer sequencer;
};

#endif
