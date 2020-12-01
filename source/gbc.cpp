#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>

#include "SystemGBC.hpp"

int main(int argc, char *argv[]){
	SystemGBC gbc(argc, argv);

	// Read the ROM into memory
	if(!gbc.reset()){
		std::cout << " ERROR! Failed to read input ROM file \"" << gbc.getRomFilename() << "\"!\n";
		return 3;
	}

	// Execute the ROM
	gbc.execute();

	return 0;
}
