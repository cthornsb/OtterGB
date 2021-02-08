#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include "portaudio.h"

#include "AudioMixer.hpp"

typedef int (*portCallback)( const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* );

extern AudioMixer *audioptr;

class SoundManager{
public:
	/** Default constructor
	  */
	SoundManager();

	/** Number of voices constructor
	  */
	SoundManager(const int& voices);

	SoundManager(const SoundManager&) = delete;
	
	SoundManager& operator = (const SoundManager&) = delete;

	/** Terminate audio stream
	  */
	~SoundManager();

	/** Get reference to one of the output audio channels
	  */
	AudioData& operator [] (const size_t& index) { 
		return mixer[index]; 
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

	AudioMixer* getMixer() { 
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
	
	double dSampleRate;
	
	unsigned long nFramesPerBuffer;
	
	float fTimeStep;
	
	AudioMixer mixer;
	
	void* dptr;
	
    PaStream* stream;
    
    portCallback callback;
};

#endif

