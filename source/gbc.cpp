#include <iostream>
#include <memory>

#include "SystemGBC.hpp"

int main(int argc, char *argv[]){
	std::unique_ptr<SystemGBC> gbc(new SystemGBC(argc, argv));

	// Read the ROM into memory
	if(!gbc->reset()){
		std::cout << " ERROR! Failed to read input ROM file \"" << gbc->getRomFilename() << "\"!\n";
		return 3;
	}

	// Execute the ROM
	gbc->execute();

	return 0;
}
