#include <iostream>

#include "SoundManager.hpp"

SoundManager::~SoundManager(){
	// Terminate stream
	PaError err = Pa_Terminate();
	if( err != paNoError ){
		std::cout << " [error] Failed to initialize port audio\n";
		std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
	}
}

bool SoundManager::init(){
	if(initialized) // Already initialized
		return true;

	// Initialize port audio
	PaError err = Pa_Initialize();
	if( err != paNoError ){
		std::cout << " [error] Failed to initialize port audio\n";
		std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
		return false;
	}

	// Open audio stream
    err = Pa_OpenDefaultStream( &stream,
                                0,          /* no input channels */
                                nChannels,          /* stereo output */
                                paFloat32,  /* 32 bit floating point output */
                                dSampleRate,
                                nFramesPerBuffer,        /* frames per buffer, i.e. the number
                                                   of sample frames that PortAudio will
                                                   request from the callback. Many apps
                                                   may want to use
                                                   paFramesPerBufferUnspecified, which
                                                   tells PortAudio to pick the best,
                                                   possibly changing, buffer size.*/
                                callback, /* this is your callback function */
                                dptr ); /*This is a pointer that will be passed to
                                                   your callback*/
    if( err != paNoError ){
    	std::cout << " [error] Failed to initialize audio stream\n";
		std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
		return false;
	}

	initialized = true;
	return initialized;
}

bool SoundManager::start(){
	if(!initialized)
		return false;
	if(running) // Already started
		return true;

	// Start stream
	PaError err = Pa_StartStream( stream );
	if( err != paNoError ){
		std::cout << " [error] Failed to start audio stream\n";
		std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
		return false;
	}
	
	running = true;
	return true;
}

void SoundManager::sleep(const long& length){
	Pa_Sleep(length);
}

bool SoundManager::stop(){
	if(!initialized)
		return false;
	if(!running) // Already stopped
		return true;

	PaError err = Pa_StopStream( stream );
	if( err != paNoError ){
		std::cout << " [error] Failed to start audio stream\n";
		std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
		return false;
	}
	
	running = false;
	return false;
}

int SoundManager::defaultCallback( 
	const void *input, 
	void *output, 
	unsigned long framesPerBuffer, 
	const PaStreamCallbackTimeInfo* timeInfo, 
	PaStreamCallbackFlags statusFlags,
	void *data )
{
	float* out = static_cast<float*>(output);
	AudioData* audio = static_cast<AudioData*>(data);
	audio->getSamples(out, framesPerBuffer);
	return 0;
}

