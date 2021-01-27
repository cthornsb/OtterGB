#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include "portaudio.h"

#include "AudioData.hpp"

typedef int (*pulseCallback)( const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* );

class SoundManager{
public:
	/** Default constructor
	  */
	SoundManager() :
		initialized(false),
		running(false),
		nChannels(2),
		dSampleRate(44100),
		nFramesPerBuffer(256),
		fTimeStep(1.f / 44100.f),
		audio(2, this),
		dptr(static_cast<void*>(&audio)),
		stream(0x0),
		callback(defaultCallback)
	{ 
	}

	SoundManager(const SoundManager&) = delete;
	
	SoundManager& operator = (const SoundManager&) = delete;

	/** Terminate audio stream
	  */
	~SoundManager();

	/** Get the number of audio channels
	  */
	int getNumberOfChannels() const { return nChannels; }
	
	/** Get the audio sample rate (in Hz)
	  */
	double getSampleRate() const { return dSampleRate; }
	
	/** Get the number of audio samples per buffer
	  */
	unsigned long getFramesPerBuffer() const { return nFramesPerBuffer; }

	/** Set the number of audio channels (default = 2)
	  * Has no effect if called after audio stream is initialized.
	  */
	void setNumberOfChannels(const int& channels){ nChannels = channels; }
	
	/** Set the audio sample rate in Hz (default = 44100 Hz)
	  * Has no effect if called after audio stream is initialized
	  */
	void setSampleRate(const double& rate){ dSampleRate = rate; }

	/** Set the number of frames per audio buffer (default = 256)
	  * Has no effect if called after audio stream is initialized
	  */	
	void setFramesPerBuffer(const unsigned long& frames){ nFramesPerBuffer = frames; }

	/** Set the audio callback function
	  * Has no effect if called after audio stream is initialized
	  */
	void setCallbackFunction(pulseCallback call){ callback = call; }

	/** Set the audio data pointer
	  * Has no effect if called after audio stream is initialized
	  */	
	void setAudioData(void* data){ dptr = data; }

	/** Initialize audio stream
	  */
	bool init();

	/** Start audio stream
	  */
	bool start();
	
	/** Pause audio stream
	  */
	void sleep(const long& length);
	
	/** Stop audio stream
	  */
	bool stop();

	/** Default pulse callback function
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
	bool initialized;
	bool running;
	
	int nChannels;
	
	double dSampleRate;
	
	unsigned long nFramesPerBuffer;
	
	float fTimeStep;
	
	AudioData audio;
	
	void* dptr;
	
    PaStream* stream;
    
    pulseCallback callback;
};

#endif

