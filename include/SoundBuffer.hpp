#ifndef SOUND_BUFFER_HPP
#define SOUND_BUFFER_HPP

#include <queue>
#include <mutex>

#include "portaudio.h"

class SoundBuffer : public std::queue<std::pair<float, float> > {
public:
	/** Default constructor
	  */
	SoundBuffer() :
		std::queue<std::pair<float, float> >(),
		fEmptyLeft(0.f),
		fEmptyRight(0.f)
	{
	}

	/** Destructor
	  */
	~SoundBuffer() { }
	
	/** Copy constructor (deleted)
	  */
	SoundBuffer(const SoundBuffer&) = delete;
	
	/** Copy operator (deleted)
	  */
	SoundBuffer& operator = (const SoundBuffer&) = delete;

	/** Get instance of singleton
	  */
	static SoundBuffer& getInstance(){
		static SoundBuffer instance;
		return instance;			
	}

	/** Psuedo-overload for push() taking explicit left and right arguments
	  */ 
	void pushSample(const float& l, const float& r);
	
	/** Retrieve a single sample from the audio buffer
	  * If the buffer is empty, the most recent sample will be returned. If this is the case, false will be returned.
	  * @param output Array of samples to copy into (must have a length of at least 2)
	  * @return True if at least one sample was contained in the buffer and return false otherwise
	  */
	bool getSample(float* output);
	
	/** Retrieve N samples from the audio buffer
	  * If the number of samples is greater than the length of the buffer, attempt to interpolate between existing samples
	  * to create the illusion of there being more audio data. If this is the case, false will be returned.
	  * @parm output Array of samples to copy into (must have a length of at least 2 * N)
	  * @return True if the buffer contained at least N samples and return false otherwise
	  */
	bool getSamples(float* output, const size_t& N);

	/** Port callback function
	  */
	static int callback( 
		const void *input, 
		void *output, 
		unsigned long framesPerBuffer, 
		const PaStreamCallbackTimeInfo* timeInfo, 
		PaStreamCallbackFlags statusFlags,
		void *data 
	);

private:
	float fEmptyLeft; ///< In the event that the sound buffer is now empty, the last audio sample for the left output channel
	
	float fEmptyRight; ///< In the event that the sound buffer is now empty, the last audio sample for the right output channel

	std::mutex lock; ///< Mutex lock for buffer read/write access

	/** Get the next sample for the left output channel
	  * If the buffer is empty, the backup value will be returned
	  */
	float left() const ;
	
	/** Get the next sample for the right output channel
	  * If the buffer is empty, the backup value will be returned
	  */
	float right() const ;

	/** Pop the oldest sample off the buffer
	  * If the buffer will be made empty by the pop, backup the current sample.
	  */
	void popSample();
};

#endif
