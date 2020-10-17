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
	handler.add(optionExt("debug", no_argument, NULL, 'd', "", "Enable CPU instruction debugging mode."));
	handler.add(optionExt("framerate", no_argument, NULL, 'f', "", "Output emulation framerate."));
	handler.add(optionExt("frequency", required_argument, NULL, 'F', "<frequency>", "Set the CPU frequency multiplier."));
	handler.add(optionExt("verbose", no_argument, NULL, 'v', "", "Toggle verbose mode."));
	handler.add(optionExt("break", required_argument, NULL, 'B', "<NN>", "Set 16-bit instruction breakpoint, NN (base 16)."));
	handler.add(optionExt("write-watch", required_argument, NULL, 'W', "<NN|NN1:NN2>", "Watch memory write to location NN or in range [NN1,NN2] (base 16)."));
	handler.add(optionExt("read-watch", required_argument, NULL, 'R', "<NN|NN1:NN2>", "Watch memory read of memory location NN or in range [NN1,NN2] (base 16)."));
	handler.add(optionExt("opcode-watch", required_argument, NULL, 'O', "<N>", "Watch for specific opcode calls."));
	handler.add(optionExt("frame-skip", required_argument, NULL, 's', "<N>", "Render 1 out of every N frames (improves framerate)."));
	handler.add(optionExt("scale-factor", required_argument, NULL, 'S', "<N>", "Set the integer size multiplier for the screen (default 2)."));
			
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
		gbc.getCPU()->setBreakpoint((unsigned short)strtoul(handler.getOption(5)->argument.c_str(), NULL, 16));

	if(handler.getOption(6)->active){ // Set memory write watch region
		std::vector<std::string> args;
		unsigned int nArgs = splitString(handler.getOption(6)->argument, args, ':');
		if(nArgs >= 2)
			gbc.setMemoryWriteRegion((unsigned short)strtoul(args[0].c_str(), NULL, 16), (unsigned short)strtoul(args[1].c_str(), NULL, 16));
		else if(nArgs == 1)
			gbc.setMemoryWriteRegion((unsigned short)strtoul(args[0].c_str(), NULL, 16));
		else
			std::cout << " WARNING: Invalid memory range (" << handler.getOption(6)->argument << ").\n";			
	}

	if(handler.getOption(7)->active){ // Set memory read watch region
		std::vector<std::string> args;
		unsigned int nArgs = splitString(handler.getOption(7)->argument, args, ':');
		if(nArgs >= 2)
			gbc.setMemoryReadRegion((unsigned short)strtoul(args[0].c_str(), NULL, 16), (unsigned short)strtoul(args[1].c_str(), NULL, 16));
		else if(nArgs == 1)
			gbc.setMemoryReadRegion((unsigned short)strtoul(args[0].c_str(), NULL, 16));
		else
			std::cout << " WARNING: Invalid memory range (" << handler.getOption(7)->argument << ").\n";			
	}

	if(handler.getOption(8)->active) // Set frame-skip
		gbc.getCPU()->setOpcode((unsigned short)strtoul(handler.getOption(8)->argument.c_str(), NULL, 16));

	if(handler.getOption(9)->active) // Set frame-skip
		gbc.setFrameSkip((unsigned short)strtoul(handler.getOption(9)->argument.c_str(), NULL, 10));

	if(handler.getOption(10)->active) // Set pixel scaling factor
		gbc.getGPU()->setPixelScale(strtoul(handler.getOption(10)->argument.c_str(), NULL, 10));

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
