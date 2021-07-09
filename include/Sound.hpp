#ifndef SOUND_HPP
#define SOUND_HPP

#include <fstream>
#include <memory>

#include "SystemComponent.hpp"
#include "SystemTimer.hpp"

#include "SoundMixer.hpp"
#include "SquareWave.hpp"
#include "WaveTable.hpp"
#include "ShiftRegister.hpp"

#include "MidiFile.hpp"
#include "WavFile.hpp"

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

	/** Initialize the audio output interface
	  */
	void initialize(bool audioOutputEnabled, const double& clockSpeed);

	/** Get pointer to output audio mixer
	  */
	SoundMixer* getMixer(){
		return &mixer;
	}

	/** Return true if the APU is enabled (i.e. if it is powered up) and return false otherwise
	  */
	bool isEnabled() const {
		return bMasterSoundEnable;
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
	
	/** Get the current remaining time (if length counter is active) for an audio channel (in milliseconds)
	  */
	float getChannelTime(const int& ch) const {
		return (getChannelLength(ch) * 3.90625f);
	}
	
	/** Get the current period for an audio channel in 4 MHz APU ticks
	  */
	unsigned short getChannelPeriod(const int& ch) const ;
	
	/** Get the current frequency for an audio channel in Hz
	  */
	float getChannelFrequency(const int& ch) const ;
	
	/** Set the mixer sample rate (in Hz). The default sample rate is 32768 Hz.
	  * Note that setting the sample rate after initializing the audio interface will have no effect. 
	  * @param rate Mixer sample rate (in the range 16 to 1,048,576 Hz)
	  */
	void setSampleRate(const float& rate);
	
	/** Set the mixer timer period multiplier to account for non-standard clock speed
	  */
	void setSampleRateMultiplier(const float &mult);
	
	/** Modify mixer timer period for CGB double speed mode (2 MHz)
	  */
	void setDoubleSpeedMode();

	/** Modify mixer timer period for CGB normal speed mode (1 MHz)
	  */
	void setNormalSpeedMode();
	
	/** Disable audio channel output and DAC (indexed from 1)
	  */
	void disableChannel(const int& ch);
	
	/** Disable audio channel output and DAC
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
	
	/** Start recording a midi file
	  */
	void startMidiFile(const std::string& filename="out.mid");
	
	/** Stop recording midi file and write it to disk
	  */
	void stopMidiFile();
	
	/** Return true if midi file recording is in progress and return false otherwise
	  */
	bool midiFileEnabled() const {
		return bRecordMidi;
	}

	/** Start recording a wav file
	  */
	bool startRecording(const std::string& filename = "out.wav");

	/** Stop recording wav file and close its output file
	  */
	bool stopRecording();
	
	/** Return true if a wav recording is currently in progress
	  */
	bool recordingEnabled() const {
		return bRecording;
	}

	/** The sound controller has no associated RAM, so return false to avoid trying to access it
	  */
	bool preWriteAction() override { 
		return false; 
	}

	/** The sound controller has no associated RAM, so return false to avoid trying to access it.
	  */
	bool preReadAction() override { 
		return false; 
	}

	/** Check that the specified APU register may be written to.
	  * @param reg Register address.
	  * @return True if the APU is powered or the address is the APU control register or in wave RAM.
	  */
	bool checkRegister(const unsigned short &reg) override;

	/** Write APU register
	  */
	bool writeRegister(const unsigned short &reg, const unsigned char &val) override;
	
	/** Read APU register
	  */
	bool readRegister(const unsigned short &reg, unsigned char &val) override;

	/** Clock the audio DACs, the mixer, and the frame sequencer
	  */
	bool onClockUpdate() override;
	
	/** Define APU registers
	  */
	void defineRegisters() override;

	/** Read settings from an input user configuration file
	  */
	void readConfigFile(ConfigFile* config) override;

private:
	bool bInitialized; ///< Set if audio interface has been initialized

	bool bMasterSoundEnable; ///< Master sound enabled flag

	bool bRecordMidi; ///< Midi recording in progress

	bool bRecording; ///< Set if wav recording is in progress

	SoundManager* audio; ///< Main system audio handler
	
	SoundMixer mixer; ///< Audio output mixer

	SquareWave ch1; ///< Channel 1 (square w/ frequency sweep)

	SquareWave ch2; ///< Channel 2 (square)
	
	WaveTable ch3; ///< Channel 3 (wave)
	
	ShiftRegister ch4; ///< Channel 4 (noise)

	unsigned char wavePatternRAM[16]; ///< 4-bit WAVE audio samples
	
	unsigned int nSequencerTicks; ///< Frame sequencer tick counter
	
	unsigned int nMidiClockTicks; ///< Midi clock tick counter (if midi recording in progress)

	unsigned short nMixerClockPeriod; ///< The period of the audio output mixer (in 1 MHz sys clock ticks)

	std::unique_ptr<MidiFile::MidiFileRecorder> midiFile; ///< Midi file recorder
	
	std::unique_ptr<WavFile::WavFileRecorder> wavFile; ///< Wav file recorder

	/** Power down APU
	  * Stop audio output stream, disable audio output channels, clear all APU registers, and reset all audio DACs.
	  */
	void powerDown();
	
	/** Handle enabling or disabling channel output when that channel's DAC is triggered (by writting to NRx4)
	  * @param ch Audio channel number (1, 2, 3, or 4)
	  * @return True if the specified channel is now enabled and return false otherwise
	  */
	bool handleTriggerEnable(const int& ch);

	/** Get a pointer to one of the four audio DACs (ch=1 to 4)
	  * @return Pointer to channel ch's audio DAC or null if invalid channel
	  */
	AudioUnit* getAudioUnit(const int& ch);

	/** Get a const pointer to one of the four audio DACs (ch=1 to 4)
	  * @return Pointer to channel ch's audio DAC or null if invalid channel
	  */
	const AudioUnit* getConstAudioUnit(const int& ch) const ;

	/** Clock the frame sequencer, disabling any DACs which clocking has caused to expire
	  */
	void rollover() override;

	/** Add elements to a list of values which will be written to / read from an emulator savestate
	  */
	void userAddSavestateValues() override;
	
	/** Shut down APU and clear all DACs
	  */
	void onUserReset() override;
};

#endif
