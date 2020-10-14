
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "Opcodes.hpp"
#include "OpcodeNames.hpp"

unsigned int splitString(const std::string &input, std::vector<std::string> &output, const char &delim='\t'){
	output.clear();
	size_t start = 0;
	size_t stop = 0;
	while(true){
		stop = input.find(delim, start);
		if(stop == std::string::npos){
			output.push_back(input.substr(start));
			break;
		}
		output.push_back(input.substr(start, stop-start));
		start = stop+1;
	}
	return output.size();
}

std::string getHex(const unsigned char &input){
	std::stringstream stream;
	stream << std::hex << (int)input;
	std::string output = stream.str();
	if(output.length() < 2) output = "0" + output;
	return ("$" + output);
}

std::string getHex(const unsigned short &input){
	std::stringstream stream;
	stream << std::hex << (int)input;
	std::string output = stream.str();
	output = (output.length() < 4 ? std::string(4-output.length(), '0') : "") + output;
	return ("$" + output);
}

bool injectArgs(std::string &input, const std::string &injectedText, const std::string &target="$"){
	size_t index = input.find(target);
	if(index == std::string::npos) return false;
	input = input.substr(0, index) + injectedText + input.substr(index+target.length());
	return true;
}

unsigned int readHeader(std::ifstream &f){
	char leader[2];
	unsigned short programStart;
	char nintendoString[48];
	char titleString[17];
	char manufacturerExt[2];
	char cartridgeType;
	char romSize;
	char ramSize;
	char language;
	char manufacturer;
	char versionNumber;
	char complementCheck;
	char checksum[2]; // High byte first

	std::string romSizes[5] = {"32kB", "64kB", "128kB", "256kB", "512kB"};
	std::string ramSizes[4] = {"2kB", "8kB", "32kB"};

	f.read(leader, 2);
	f.read((char*)&programStart, 2);
	f.read(nintendoString, 48);
	f.read(titleString, 16); titleString[16] = '\0';
	f.read(manufacturerExt, 2);
	f.read(&cartridgeType, 1);
	f.read(&romSize, 1);
	f.read(&ramSize, 1);
	f.read(&language, 1);
	f.read(&manufacturer, 1);
	f.read(&versionNumber, 1);
	f.read(&complementCheck, 1);
	f.read(checksum, 2);

	std::cout << "Title: " << std::string(titleString) << std::endl;
	std::cout << " ROM: " << romSizes[int(romSize)] << "\n";
	std::cout << " RAM: " << ramSizes[int(ramSize)] << "\n";
	std::cout << " Vers: " << int(versionNumber) << "\n";
	std::cout << " Lang: " << (language == 0x0 ? "J" : "E") << std::endl;
	std::cout << " Program entry at " << getHex(programStart) << "\n";

	return 79;
}

int main(int argc, char *argv[]){
	if(argc < 2)
		return 1;

	// Open the rom file
	std::ifstream rom(argv[1], std::ios::binary);
	if(!rom.good())
		return 2;

	// Open the output file
	std::ofstream fout;
	if(argc < 3)
		fout.open("out.s");
	else
		fout.open(argv[2]);
		
	if(!fout.good()){
		rom.close();
		return 3;
	}

	unsigned char z80[12] = {0xD3, 0xDB, 0xDD, 0xE3, 
	                         0xE4, 0xEB, 0xEC, 0xED,
	                         0xF2, 0xF4, 0xFC, 0xFD};
	
	// Set the initial value of the program counter (PC)
	unsigned short pc = 0x0000; // program counter
	if(pc != 0);
		rom.seekg(pc);

	// Start disassembling the rom
	unsigned char op;
	unsigned char val8, val16l, val16h;
	unsigned short val16;
	bool isPrefixCB = false;
	while(true){
		rom.read((char*)&op, 1); // Read the opcode byte
		if(rom.eof()) break;
		
		if(pc == 255){ // Read the rom header
			pc += readHeader(rom) + 1; // Add one to account for the opcode read
			continue;
		}
		
		std::string name;
		unsigned int len;
		if(op != 0xCB){
			// Look-up the opcode in the opcode table
			if(!isPrefixCB){ // Normal opcode
				name = opcodeNames[op];
				len = opcodeLengths[op];
			}
			else{ // CB prefix
				name = opcodeNamesCB[op];
				len = 1;
				isPrefixCB = false;
			}
			
			if(name.empty()){ // Check for undefined opcodes
				bool z80code = false;
				for(int i = 0; i < 12; i++){
					if(z80[i] == op){
						// The processor simply ignores invalid opcodes.
						//std::cout << "  HERE! " << getHex(op);
						//rom.read((char*)&op, 1);
						//std::cout << " " << getHex(op) << std::endl;
						std::cout << " WARNING: Encountered z80 opcode (" << getHex(op) << ") at position " << pc << ".\n";
						z80code = true;
						break;
					}
				}
				if(!z80code)
					std::cout << " WARNING: Read undefined opcode (" << getHex(op) << ") at position " << pc << ".\n";
				fout << getHex(pc) << "\tNOP" << std::endl;
			}
			
			val8 = 0x0;
			val16l = 0x0;
			val16h = 0x0;
			val16 = 0x0;

			// Read the opcode's accompanying value (if any)
			std::string arg;
			if(len == 1){ // Do nothing
			}
			else if(len == 2){ // Read 8 bits (valid targets: d8, a8, r8)
				rom.read((char*)&val8, 1);
				arg = getHex(val8);
				if(!injectArgs(name, arg, "d8") && op != 0x10){ // Ignore STOP 0
					if(!injectArgs(name, arg, "a8")){
						if(!injectArgs(name, arg, "r8")){
							std::cout << " WARNING: Found no target for opcode (" << getHex(op) << ") " << name << "\n";
						}
					}
				}
			}
			else if(len == 3){ // Read 16 bits (valid targets: d16, a16)
				rom.read((char*)&val16l, 1);
				rom.read((char*)&val16h, 1);
				val16 = ((val16h & 0x00FF) << 8) + val16l;
				arg = getHex(val16);
				if(!injectArgs(name, arg, "d16")){
					if(!injectArgs(name, arg, "a16")){
						std::cout << " WARNING: Found no target for opcode (" << getHex(op) << ") " << name << "\n";
					}
				}
			}
			else{
				std::cout << " ERROR: Encountered illegal opcode size (" << len << ") for opcode " << getHex(op) << "!\n";
				break;			
			}
			fout << getHex(pc) << "\t" << name << std::endl;
		}
		else{
			isPrefixCB = true;
			len = 1;
		}
			
		pc += len;
	}
	rom.close();
	fout.close();

	std::cout << " Done! Read " << pc << " bytes from input rom file.\n";

	return 0;
}
