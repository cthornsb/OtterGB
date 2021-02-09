#ifndef SOUND_MIXER_HPP
#define SOUND_MIXER_HPP

#include "UnitTimer.hpp"

constexpr unsigned short MIXER_CLOCK_PERIOD = 64; // 16.384 kHz clock

class AudioUnit;

class SoundMixer : public UnitTimer {
public:
	/** Default constructor
	  */
	SoundMixer() :
		UnitTimer(MIXER_CLOCK_PERIOD),
		bModified(false),
		bStereoOutput(true),
		fMasterVolume(1.f),
		fOffsetDC(0.f),
		fOutputVolume{1.f, 1.f},
		fOutputSamples{0.f, 0.f},
		fInputVolume{1.f, 1.f, 1.f, 1.f},
		fInputSamples{0.f, 0.f, 0.f, 0.f},
		bSendInputToOutput{{0, 0, 0, 0}, {0, 0, 0, 0}}
	{ 
	}

	/** Get the current output channel sample
	  */
	float operator [] (const unsigned int& ch){
		return fOutputSamples[ch];
	}

	/** Get a pointer to the input sample buffer
	  */
	float* getSampleBuffer(){
		return fInputSamples;
	}

	/** Get the current master output volume
	  */
	float getVolume() const {
		return fMasterVolume;
	}
	
	/** Return true if audio output is muted and return false otherwise
	  */
	bool isMuted() const {
		return bMuted;
	}

	/** Mute or un-mute master output
	  * @return True if the output is muted and return false otherwise
	  */
	bool mute(){
		return (bMuted = !bMuted);
	}

	/** Set master output volume
	  * Volume clamped to range [0, 1]
	  */
	void setVolume(const float& volume){
		if(bMuted) // Un-mute
			bMuted = false;
		fMasterVolume = clamp(volume);
		if(fMasterVolume == 0.f)
			bMuted = true;
	}

	/** Increase volume by a specified amount (clamped to between 0 and 1)
	  */
	void increaseVolume(const float& change=0.1f){
		setVolume(fMasterVolume + change);
	}
	
	/** Decrease volume by a specified amount (clamped to between 0 and 1)
	  */
	void decreaseVolume(const float& change=0.1f){
		setVolume(fMasterVolume - change);
	}
	
	/** Set the volume for one of the input channels
	  */
	void setChannelVolume(const unsigned char& ch, const float& volume){
		if(ch < 4)
			fInputVolume[ch] = clamp(volume);
	}

	/** Set the negative DC offset of the output audio effectively making the output up to 100% louder (default is 0)
	  * Offset clamped to range [0, 1]
	  */
	void setOffsetDC(const float& offset){
		fOffsetDC = clamp(offset);
	}

	/** Set output level for left and right output channels
	  * Volumes clamped to range [0, 1]
	  */
	void setOutputLevels(const float& l, const float& r);
	
	/** Set left/right audio output balance
	  * A value of -1 is 100% left and 0% right, 0 is 100% left and 100% right, +1 is 0% left and 100% right
	  */
	void setBalance(const float& bal);
	
	/** Average independent left and right output and send the result to both output channels
	  */
	void setMonoOutput(){
		bStereoOutput = false;
	}
	
	/** Send left and right output directly to output channels (default)
	  */
	void setStereoOutput(){
		bStereoOutput = true;
	}
	
	/** Send an audio input channel to an audio output channel
	  * @param input Desired input channel (0 to 3)
	  * @param output Desired output channel (0 to 1)
	  */
	void setInputToOutput(const int& input, const int& output, bool state=true){
		bSendInputToOutput[output][input] = state;
	}
	
	/** Set the 4-bit audio sample for a specified channel (clamped to 0 to 15)
	  * Also sets the mixer's modified flag
	  */
	void setInputSample(const unsigned char& ch, const unsigned char& vol){
		bModified = true;
		fInputSamples[ch] = clamp(vol / 15.f, 0.f, 1.f);
	}
	
	/** 
	  */
	void setSampleRateMultiplier(const float &freq){
		nPeriod = (unsigned short)(MIXER_CLOCK_PERIOD * freq);
	}
	
	/** Modify mixer timer period for CGB double speed mode (2 MHz)
	  */
	void setDoubleSpeedMode(){
		nPeriod = MIXER_CLOCK_PERIOD * 2; 
	}

	/** Modify mixer timer period for CGB normal speed mode (1 MHz)
	  */
	void setNormalSpeedMode(){
		nPeriod = MIXER_CLOCK_PERIOD;
	}
	
	/** Update output audio samples
	  * This update should occur any time an audio unit clocks over.
	  * @return True if at least one of the input samples was modified and return false otherwise
	  */
	bool update();

private:
	bool bMuted; ///< Audio output muted by user

	bool bModified; ///< Flag indiciating that one or more input samples were modified

	bool bStereoOutput; ///< Stereo output flag

	float fMasterVolume; ///< Master output volume
	
	float fOffsetDC; ///< "DC" offset of output audio waveform (in range 0 to 1)
	
	float fOutputVolume[2]; ///< Left and right output channel volume

	float fOutputSamples[2]; ///< Output audio sample buffer for left/right output
	
	float fInputVolume[4]; ///< Volumes for all four CGB input audio channels

	float fInputSamples[4]; ///< Audio sample buffer for CGB inputs

	bool bSendInputToOutput[2][4]; ///< Flags for which input signals will be sent to which output signals
	
	/** Clamp an input value to the range [low, high]
	  */
	float clamp(const float& input, const float& low=0.f, const float& high=1.f) const ;
	
	/** Method called when unit timer clocks over (every nPeriod input clock ticks)
	  */	
	virtual void rollover();
};

#endif
