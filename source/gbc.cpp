#include <iostream>
#include <string>
#include <stdlib.h>

#include "optionHandler.hpp"
#include "SystemGBC.hpp"

int main(int argc, char *argv[]){
	optionHandler handler;
	handler.add(optionExt("input", required_argument, NULL, 'i', "<filename>", "Specify an input geant macro."));
	handler.add(optionExt("debug", no_argument, NULL, 'd', "", "Enable CPU instruction debugging mode."));
	handler.add(optionExt("framerate", no_argument, NULL, 'f', "", "Output emulation framerate."));
	handler.add(optionExt("frequency", required_argument, NULL, 'F', "<frequency>", "Set the CPU frequency multiplier."));
	handler.add(optionExt("verbose", no_argument, NULL, 'v', "", "Toggle verbose mode."));
	handler.add(optionExt("break", required_argument, NULL, 'B', "<breakpoint>", "Set 16-bit instruction breakpoint (base 16)."));
	
	// Handle user input.
	if(!handler.setup(argc, argv))
		return 1;

	// Main system bus
	SystemGBC gbc;

	std::string inputFilename;
	if(handler.getOption(0)->active) // Set input filename
		inputFilename = handler.getOption(0)->argument;

	if(handler.getOption(1)->active) // Toggle debug flag
		gbc.setDebugMode(true);

	if(handler.getOption(2)->active) // Toggle framerate output
		gbc.setDisplayFramerate(true);

	if(handler.getOption(3)->active) // Set CPU frequency multiplier
		gbc.setCpuFrequency(strtod(handler.getOption(3)->argument.c_str(), NULL));

	if(handler.getOption(4)->active) // Toggle verbose flag
		gbc.setVerboseMode(true);

	if(handler.getOption(5)->active) // Set program breakpoint
		gbc.setBreakpoint((unsigned short)strtoul(handler.getOption(5)->argument.c_str(), NULL, 16));

	// Check for ROM filename.
	if(inputFilename.empty()){
		std::cout << " ERROR! Input gb/gbc ROM file not specified!\n";
		return 2;
	}

	// Execute the ROM
	if(!gbc.initialize(inputFilename)){
		std::cout << " ERROR! Failed to read input ROM file \"" << inputFilename << "\"!\n";
		return 3;
	}
	gbc.execute();

	return 0;
}
