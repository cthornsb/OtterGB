#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include "portaudio.h"

#include "AudioMixer.hpp"

typedef int (*pulseCallback)( const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* );

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
	AudioData& operator [] (const size_t& index) { return mixer[index]; }

	/** Get the number of audio channels
	  */
	int getNumberOfChannels() const { return nChannels; }
	
	/** Get the audio sample rate (in Hz)
	  */
	double getSampleRate() const { return dSampleRate; }
	
	/** Get the number of audio samples per buffer
	  */
	unsigned long getFramesPerBuffer() const { return nFramesPerBuffer; }

	AudioMixer* getMixer() { return &mixer; }

	/** Return true if the audio interface is running, and return false otherwise
	  */
	bool isRunning() const { return running; }

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
	//void setAudioData(void* data){ dptr = data; }

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
	
	AudioMixer mixer;
	
	void* dptr;
	
    PaStream* stream;
    
    pulseCallback callback;
};

#endif

