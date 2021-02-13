#include <memory>
#include <thread>

#include "SystemGBC.hpp"
#include "SoundManager.hpp"
#include "SoundBuffer.hpp"

#ifdef USE_QT_DEBUGGER
	#include <QApplication>
	#include "mainwindow.h"
#endif // ifdef USE_QT_DEBUGGER

// Main emulator object
std::unique_ptr<SystemGBC> gbc; 

// System audio interface
std::unique_ptr<SoundManager> audio; 

#ifndef USE_QT_DEBUGGER
void startMainThread() {
	// Start the main loop
	gbc->execute();
}

void startSoundManager() {
	// Start the sound manager thread
	audio->execute();
}
#endif // ifndef USE_QT_DEBUGGER

int main(int argc, char *argv[]){
	// Main emulator object
	gbc = std::unique_ptr<SystemGBC>(new SystemGBC(argc, argv));

	// Load the ROM into memory
	if(!gbc->reset()){
		return 1;
	}
	
	// System audio interface
	audio = std::unique_ptr<SoundManager>(new SoundManager());
	
	// Set the audio callback function
	audio->setCallbackFunction(SoundBuffer::callback);

	// Set audio sample rate
	audio->setSampleRate(16384);
	
	// Initialize interface
	audio->init();

	// Link sound processor to audio output interface
	gbc->setAudioInterface(audio.get());

#ifndef USE_QT_DEBUGGER
	// Start threads
	// Audio manager runs in its own thread so that sound is not linked to emulator framerate
	std::thread masterThread(startMainThread);
	std::thread soundThread(startSoundManager);
	
	masterThread.join();
	soundThread.join();
#else
	std::unique_ptr<QApplication> application;
	std::unique_ptr<MainWindow> debugger;
	if(gbc->debugModeEnabled()){ // Start debugger
		application.reset(new QApplication(argc, argv));
		debugger.reset(new MainWindow());
		debugger->setApplication(application.get());
		debugger->show(); // Open the window
		gbc->setQtDebugger(debugger.get()); // Link the debugger to the emulator
	}
	
	// Start main emulator loop
	gbc->execute();
	
	if(debugger) // Clean up the Qt GUI
		debugger->closeAllWindows(); 
#endif // ifdef USE_QT_DEBUGGER

	return 0;
}
