#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include <memory>

#include "SoundMixer.hpp"

#ifdef AUDIO_ENABLED

#include "portaudio.h"

typedef int (*portCallback)( const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* );

#endif // ifdef AUDIO_ENABLED

class SoundManager{
public:
	/** Copy constructor (deleted)
	  */
	SoundManager(const SoundMixer&) = delete;

	/** Terminate audio stream
	  */
	~SoundManager();
	
	/** Copy operator (deleted)
	  */
	SoundManager& operator = (const SoundMixer&) = delete;

	/** Get instance of singleton
	  */
	static SoundManager& getInstance(){
		static SoundManager instance;
		return instance;			
	}

	/** Get the number of audio channels
	  */
	int getNumberOfChannels() const { 
		return nChannels; 
	}
	
	/** Get the audio sample rate (in Hz)
	  */
	double getSampleRate() const { 
		return dSampleRate; 
	}
	
	/** Get the number of audio samples per buffer
	  */
	unsigned long getFramesPerBuffer() const { 
		return nFramesPerBuffer; 
	}

	/** Get pointer to the output audio buffer
	  */
	SoundMixer* getAudioMixer() { 
		return &mixer; 
	}

	/** Return true if the audio interface is running, and return false otherwise
	  */
	bool isRunning() const { 
		return bRunning; 
	}

	/** Set the number of audio channels (default = 2)
	  * Has no effect if called after audio stream is initialized.
	  */
	void setNumberOfChannels(const int& channels){ 
		nChannels = channels; 
	}
	
	/** Set the audio sample rate in Hz (default = 44100 Hz)
	  * Has no effect if called after audio stream is initialized
	  */
	void setSampleRate(const double& rate){ 
		dSampleRate = rate;
	}

	/** Set the number of frames per audio buffer (default = 256)
	  * Has no effect if called after audio stream is initialized
	  */	
	void setFramesPerBuffer(const unsigned long& frames){ 
		nFramesPerBuffer = frames;
	}

#ifdef AUDIO_ENABLED
	/** Set the audio callback function
	  * Has no effect if called after audio stream is initialized
	  */
	void setCallbackFunction(portCallback call){ 
		callback = call;
	}
#endif // ifdef AUDIO_ENABLED

	/** Initialize audio stream
	  */
	bool init();
	
	/** Terminate audio stream
	  */
	bool terminate();

	/** Start audio stream
	  */
	bool start();
	
	/** Pause audio stream
	  */
	void sleep(const long& length);
	
	/** Stop audio stream
	  */
	bool stop();
	
	/** 
	  */
	void quit(){
		bQuitting = true;
	}
	
	/** Loop until quit() is called
	  * Stream will be terminated before returning.
	  */
	void execute();

#ifdef AUDIO_ENABLED
	/** Default port callback function
	  */
	static int defaultCallback( 
		const void *input, 
		void *output, 
		unsigned long framesPerBuffer, 
		const PaStreamCallbackTimeInfo* timeInfo, 
		PaStreamCallbackFlags statusFlags,
		void *data 
	);
#endif // ifdef AUDIO_ENABLED

private:
	bool bQuitting;

	bool bInitialized;
	
	bool bRunning;
	
	int nChannels;
	
	double dSampleRate; ///< Audio sample rate
	
	unsigned long nFramesPerBuffer;

	SoundMixer mixer; ///< Audio output mixer

#ifdef AUDIO_ENABLED    	
    PaStream* stream; ///< Port audio stream pointer

    portCallback callback; ///< Port audio callback function pointer
#endif // ifdef AUDIO_ENABLED
    
	/** Default constructor
	  */
	SoundManager();

	/** Number of voices constructor
	  */
	SoundManager(const int& voices);
};

#endif

