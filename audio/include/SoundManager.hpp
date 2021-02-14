#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include <memory>

#include "portaudio.h"

#include "SoundMixer.hpp"

typedef int (*portCallback)( const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* );

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

	/** Set the audio callback function
	  * Has no effect if called after audio stream is initialized
	  */
	void setCallbackFunction(portCallback call){ 
		callback = call;
	}

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

private:
	bool bQuitting;

	bool bInitialized;
	
	bool bRunning;
	
	int nChannels;
	
	double dSampleRate; ///< Audio sample rate
	
	unsigned long nFramesPerBuffer;

	SoundMixer mixer; ///< Audio output mixer
	
    PaStream* stream;
    
    portCallback callback;
    
	/** Default constructor
	  */
	SoundManager();

	/** Number of voices constructor
	  */
	SoundManager(const int& voices);
};

#endif

