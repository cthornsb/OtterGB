#include <iostream>

#include "SoundManager.hpp"

AudioMixer* audioptr = 0x0;

SoundManager::SoundManager() :
	bQuitting(false),
	bInitialized(false),
	bRunning(false),
	nChannels(2),
	dSampleRate(44100),
	nFramesPerBuffer(256),
	fTimeStep(1.f / 44100.f),
	mixer(2, 1, this),
	dptr(static_cast<void*>(&mixer)),
	stream(0x0),
	callback(defaultCallback)
{ 
	audioptr = &mixer;
}

SoundManager::SoundManager(const int& voices) :
	bQuitting(false),
	bInitialized(false),
	bRunning(false),
	nChannels(2),
	dSampleRate(44100),
	nFramesPerBuffer(256),
	fTimeStep(1.f / 44100.f),
	mixer(2, voices, this),
	dptr(static_cast<void*>(&mixer)),
	stream(0x0),
	callback(defaultCallback)
{
	audioptr = &mixer;
}

SoundManager::~SoundManager(){
	if(bInitialized)
		terminate(); // Terminate stream
}

bool SoundManager::init(){
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
		dptr 
	);
	
    if( err != paNoError ){
    	std::cout << " [error] Failed to initialize audio stream\n";
		std::cout << " [error]  err=" << Pa_GetErrorText(err) << std::endl;
		return false;
	}

	bInitialized = true;
	return bInitialized;
}

bool SoundManager::terminate(){
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
}

bool SoundManager::start(){
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
}

void SoundManager::sleep(const long& length){
	Pa_Sleep(length);
}

bool SoundManager::stop(){
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
	return false;
}

void SoundManager::execute(){
	if(!bInitialized)
		return;
	while(!bQuitting){
		Pa_Sleep(500); // Sleep for user specified length of time
	}
	// Terminate stream
	terminate();
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
	//AudioMixer* audio = static_cast<AudioMixer*>(data);
	audioptr->getSamples(out, framesPerBuffer);
	return 0;
}

