#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>

#include "Support.hpp"
#include "optionHandler.hpp"
#include "SystemGBC.hpp"

int main(int argc, char *argv[]){
	optionHandler handler;
	handler.add(optionExt("input", required_argument, NULL, 'i', "<filename>", "Specify an input geant macro."));
	handler.add(optionExt("frequency", required_argument, NULL, 'F', "<frequency>", "Set the CPU frequency multiplier."));
	handler.add(optionExt("verbose", no_argument, NULL, 'v', "", "Toggle verbose mode."));
	handler.add(optionExt("scale-factor", required_argument, NULL, 'S', "<N>", "Set the integer size multiplier for the screen (default 2)."));
	handler.add(optionExt("use-color", no_argument, NULL, 'C', "", "Use GBC mode for original GB games."));
#ifdef USE_QT_DEBUGGER			
	handler.add(optionExt("debug", no_argument, NULL, 'd', "", "Enable Qt debugging GUI."));
#endif

	// Handle user input.
	if(!handler.setup(argc, argv))
		return 1;

	// Main system bus
	SystemGBC gbc;

	std::string inputFilename;
	if(handler.getOption(0)->active) // Set input filename
		inputFilename = handler.getOption(0)->argument;

	if(handler.getOption(1)->active) // Set CPU frequency multiplier
		gbc.setCpuFrequency(strtod(handler.getOption(1)->argument.c_str(), NULL));

	if(handler.getOption(2)->active) // Toggle verbose flag
		gbc.setVerboseMode(true);

	if(handler.getOption(3)->active) // Set pixel scaling factor
		gbc.getGPU()->setPixelScale(strtoul(handler.getOption(3)->argument.c_str(), NULL, 10));

	if(handler.getOption(4)->active) // Use GBC mode for original GB games
		gbc.setForceColorMode(true);

#ifdef USE_QT_DEBUGGER			
	if(handler.getOption(5)->active) // Toggle debug flag
		gbc.setDebugMode(true);
#endif

	// Check for ROM filename.
	if(inputFilename.empty()){
		std::cout << " ERROR! Input gb/gbc ROM file not specified!\n";
		return 2;
	}

	// Read the ROM into memory
	if(!gbc.initialize(inputFilename)){
		std::cout << " ERROR! Failed to read input ROM file \"" << inputFilename << "\"!\n";
		return 3;
	}

	// Execute the ROM
	gbc.execute();

	return 0;
}
