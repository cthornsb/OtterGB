
#include <iostream>
#include <string>
#include <stdlib.h>

#include "LR35902.hpp"
#include "Opcodes.hpp"
#include "OpcodeNames.hpp"

int main(){
	LR35902 cpu;
	
	// Initialize the LR35902
	cpu.initialize();
	
	std::cout << " Type \"quit\" to exit.\n\n";
	
	unsigned char op;
	std::string input;
	while(true){
		std::cout << "LR35902-> ";
		getline(std::cin, input);
		if(input == "quit")
			break;
		if(input.empty())
			continue;
		op = (unsigned char)strtoul(input.c_str(), NULL, 16);
		std::cout << " " << opcodeNames[op] << std::endl;
		std::cout << "  [" << opcodeLengths[op] << ", " << opcodeCycles[op] << "]\n";
		//cpu.execute(op);
	}

	return 0;
}
