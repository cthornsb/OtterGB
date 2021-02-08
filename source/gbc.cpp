#include <iostream>
#include <memory>
#include <thread>

#include "SystemGBC.hpp"
#include "SoundManager.hpp"
#include "SoundBuffer.hpp"

// Main emulator object
std::unique_ptr<SystemGBC> gbc; 

// System audio interface
std::unique_ptr<SoundManager> audio; 

void startMainThread(){
	// Start the main loop
	gbc->execute();
}

void startSoundManager(){
	// Start the sound manager
	audio->execute();
}

int main(int argc, char *argv[]){
	// Main emulator object
	gbc = std::unique_ptr<SystemGBC>(new SystemGBC(argc, argv));

	// Read the ROM into memory
	if(!gbc->reset()){
		return 1;
	}

	// System audio interface
	audio = std::unique_ptr<SoundManager>(new SoundManager());
	
	// Set the audio callback function
	audio->setCallbackFunction(SoundBuffer::callback);

	// Set sample rate	
	audio->setSampleRate(16384);
	
	// Initialize interface
	audio->init();

	// Link sound processor to audio output interface
	gbc->setAudioInterface(audio.get());
	
	// Start threads
	// Audio manager runs in its own thread so that sound is not linked to emulator framerate
	std::thread masterThread(startMainThread);
	std::thread soundThread(startSoundManager);
	
	masterThread.join();
	soundThread.join();

	return 0;
}
