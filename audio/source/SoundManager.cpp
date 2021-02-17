#include <iostream>
#include <thread>
#include <cmath>

#include "SoundManager.hpp"
#include "SoundMixer.hpp"

SoundManager::SoundManager() :
	bQuitting(false),
	bInitialized(false),
	bRunning(false),
	nChannels(2),
	dSampleRate(44100),
	nFramesPerBuffer(512),
#ifdef AUDIO_ENABLED
	mixer(),
	stream(0x0),
	callback(defaultCallback)
#else
	mixer()
#endif // ifdef AUDIO_ENABLED
{ 
}

SoundManager::SoundManager(const int& voices) :
	bQuitting(false),
	bInitialized(false),
	bRunning(false),
	nChannels(2),
	dSampleRate(44100),
	nFramesPerBuffer(512),
#ifdef AUDIO_ENABLED
	mixer(),
	stream(0x0),
	callback(defaultCallback)
#else
	mixer()
#endif // ifdef AUDIO_ENABLED
{
}

SoundManager::~SoundManager(){
	if(bInitialized)
		terminate(); // Terminate stream
}

bool SoundManager::init(){
#ifdef AUDIO_ENABLED
	if(bInitialized) // Already initialized
		return true;

	// Initialize port audio
	PaError err = Pa_Initialize();
	if( err != paNoError ){
		std::cout << " [error] Failed to initialize port audio\n";
		std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
		return false;
	}

	// Open audio stream
    err = Pa_OpenDefaultStream(
    	&stream,
		0,
		nChannels,
		paFloat32,
		dSampleRate,
		nFramesPerBuffer,
		callback,
		static_cast<void*>(&mixer)
	);
	
    if( err != paNoError ){
    	std::cout << " [error] Failed to initialize audio stream\n";
		std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
		return false;
	}

	bInitialized = true;
	return bInitialized;
#else
	return false;
#endif // ifdef AUDIO_ENABLED
}

bool SoundManager::terminate(){
#ifdef AUDIO_ENABLED
	if(bInitialized){
		if(bRunning){
			stop();
		}
		PaError err = Pa_Terminate();
		bInitialized = false;
		if( err != paNoError ){
			std::cout << " [error] Failed to terminate port audio\n";
			std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
			return false;
		}
	}
	return true;
#else
	return false;
#endif // ifdef AUDIO_ENABLED
}

bool SoundManager::start(){
#ifdef AUDIO_ENABLED
	if(!bInitialized)
		return false;
	if(bRunning) // Already started
		return true;

	// Start stream
	PaError err = Pa_StartStream( stream );
	if( err != paNoError ){
		std::cout << " [error] Failed to start audio stream\n";
		std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
		return false;
	}
	
	bRunning = true;
	return true;
#else
	return false;
#endif // ifdef AUDIO_ENABLED
}

void SoundManager::sleep(const long& length){
#ifdef AUDIO_ENABLED
	Pa_Sleep(length);
#endif // ifdef AUDIO_ENABLED
}

bool SoundManager::stop(){
#ifdef AUDIO_ENABLED
	if(!bInitialized)
		return false;
	if(!bRunning) // Already stopped
		return true;

	PaError err = Pa_StopStream( stream );
	if( err != paNoError ){
		std::cout << " [error] Failed to start audio stream\n";
		std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
		return false;
	}
	
	bRunning = false;
	return true;
#else
	return false;
#endif // ifdef AUDIO_ENABLED
}

void SoundManager::execute(){
#ifdef AUDIO_ENABLED
	if(!bInitialized)
		return;
	while(!bQuitting){
		Pa_Sleep(500); // Sleep for user specified length of time
	}
	// Terminate stream
	terminate();
#endif // ifdef AUDIO_ENABLED
}

#ifdef AUDIO_ENABLED
int SoundManager::defaultCallback( 
	const void *input, 
	void *output, 
	unsigned long framesPerBuffer, 
	const PaStreamCallbackTimeInfo* timeInfo, 
	PaStreamCallbackFlags statusFlags,
	void *data )
{
	float* out = static_cast<float*>(output);
	SoundMixer* audio = static_cast<SoundMixer*>(data);
	audio->getSamples(out, framesPerBuffer);
	return 0;
}
#endif // ifdef AUDIO_ENABLED

