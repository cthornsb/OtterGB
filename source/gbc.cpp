#include <memory>
#include <string>

#include "OTTWindow.hpp"

#include "SystemGBC.hpp"
#include "GPU.hpp"

#ifdef USE_QT_DEBUGGER
	#include <QApplication>
	#include "mainwindow.h"
#endif // ifdef USE_QT_DEBUGGER

SystemGBC* ottergb = 0x0;

void handlePathDrop(const std::string& path) {
	ottergb->setRomPath(path);
	ottergb->reset();
}

void handleWindowFocus(GLFWwindow*, int focused) {
	if (focused == GLFW_TRUE)
		ottergb->unpause();
	else
		ottergb->pause();
}

int main(int argc, char *argv[]){
	// Main emulator object
	std::unique_ptr<SystemGBC> gbc = std::unique_ptr<SystemGBC>(new SystemGBC(argc, argv));

	// Load the ROM into memory
	if(!gbc->reset()){
		// Failed to read input ROM, boot to the console
		gbc->openDebugConsole();
	}
	
	ottergb = gbc.get();
	OTTWindow* window = gbc->getGPU()->getWindow();
	window->setPathDropCallback(handlePathDrop);
	window->setWindowFocusCallback(handleWindowFocus);

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

	// Start main emulator loop
	gbc->execute();

#ifdef USE_QT_DEBUGGER	
	if(debugger) // Clean up the Qt GUI
		debugger->closeAllWindows(); 
#endif // ifdef USE_QT_DEBUGGER

	return 0;
}
