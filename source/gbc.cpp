#include <memory>
#include <thread>

#include "SystemGBC.hpp"
#include "GPU.hpp"
#include "ComponentThread.hpp"

#ifdef USE_QT_DEBUGGER
	#include <QApplication>
	#include "mainwindow.h"
#endif // ifdef USE_QT_DEBUGGER

int main(int argc, char *argv[]){
	// Main emulator object
	std::unique_ptr<SystemGBC> gbc = std::unique_ptr<SystemGBC>(new SystemGBC(argc, argv));

	// Load the ROM into memory
	if(!gbc->reset()){
		return 1;
	}
	
#ifdef USE_QT_DEBUGGER
	std::unique_ptr<QApplication> application;
	std::unique_ptr<MainWindow> debugger;
	if(gbc->debugModeEnabled()){ // Start debugger
		application.reset(new QApplication(argc, argv));
		debugger.reset(new MainWindow());
		debugger->setApplication(application.get());
		debugger->show(); // Open the window
		gbc->setQtDebugger(debugger.get()); // Link the debugger to the emulator
	}
#endif // ifdef USE_QT_DEBUGGER

	// Graphical window
	/*GPU* gpu = gbc->getGPU();
	
	// Setup the thread manager to synchronize emulator and graphics
	ThreadManager manager;
	
	manager.addThread(gbc.get());
	manager.addThread(gpu);
	
	// Lock threads so that they will be able to sync with eachother
	manager.lock();
	
	// Define threads
	std::thread threadMainloop( [&](){ gbc->execute(); } );
	std::thread threadGraphics( [&](){ gpu->execute(); } );
	
	// Start threads
	threadMainloop.join();
	threadGraphics.join();*/
	
	// Start main emulator loop
	gbc->execute();

#ifdef USE_QT_DEBUGGER	
	if(debugger) // Clean up the Qt GUI
		debugger->closeAllWindows(); 
#endif // ifdef USE_QT_DEBUGGER

	return 0;
}
