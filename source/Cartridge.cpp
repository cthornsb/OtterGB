
#include <iostream>

#include "Support.hpp"
#include "SystemRegisters.hpp"
#include "Cartridge.hpp"

#define ROM_ZERO_LOW  0x0000
#define ROM_SWAP_LOW  0x4000
#define ROM_HIGH      0x8000

/////////////////////////////////////////////////////////////////////
// class Cartridge
/////////////////////////////////////////////////////////////////////

bool Cartridge::preReadAction(){
	if(readLoc < ROM_SWAP_LOW){ // ROM bank 0
		readBank = 0;
		return true;
	}
	else if(readLoc < ROM_HIGH){ // Switch to ROM bank 1
		readLoc = readLoc - ROM_SWAP_LOW;
		readBank = bs;
		return true;
	}
	
	return false;
}

bool Cartridge::writeRegister(const unsigned short &reg, const unsigned char &val){
	switch(cartridgeType){
		case 0x0: // ROM only
			break;
		case 0x1 ... 0x3: // MBC1
			if(reg < 0x2000){ // RAM enable (write only)
				// Any value written to this area with 0x0A in its lower 4 bits will enable cartridge RAM
				// Note: Type 0x01 does not have cartridge RAM
				extRamEnabled = ((val & 0x0F) == 0x0A);
			}
			else if(reg < 0x4000){ // ROM bank number register (write only)
				// 5-bit register [0x01, 0x1F] (write only)
				// Specify lower 5 bits of ROM bank number
				bs &= 0xE0; // Clear bits 0-4
				bs |= (val & 0x1F); // Set bits 0-4
			}
			else if(reg < 0x6000){
				// 2-bit register [0,3] (write only)
				if(cartridgeType == 0x1 || !ramSelect){ // Specify bits 5 & 6 of the ROM bank number (if in ROM select mode).
					bs &= 0x9F; // Clear bits 5-6
					bs |= ((val & 0x3) << 5); // Set bits 5-6
				}
				else // Specify RAM bank number (if in RAM select mode).
					ram.setBank(val & 0x3);
			}
			else if(reg < 0x8000){ // ROM/RAM mode select
				// 1-bit register which sets RAM/ROM mode select to 0x0 (ROM, default) or 0x1 (RAM)  
				// Note: Type 0x01 does not have cartridge RAM
				ramSelect = (val & 0x1) == 0x1;
			}
			break;
		case 0x5 ... 0x6: // MBC2
			if(reg < 0x2000){ // RAM enable (write only)
				if((val & 0x100) == 0) // Bit 0 of upper address byte must be 0 to enable RAM
					extRamEnabled = true;
			}
			else if(reg < 0x4000){ // ROM bank number (write only)
				if((val & 0x100) != 0) // Bit 0 of upper address byte must be 1 to select ROM bank
					bs = (val & 0x0F);
			}
			break;
		case 0xB ... 0xD: // MMM01
			break;
		case 0xF ... 0x13: // MBC3
			break;
		case 0x15 ... 0x17: // MBC4
			break;
		case 0x19 ... 0x1E: // MBC5
			break;
		default:
			return false;
	}
	return true;
}

bool Cartridge::readRegister(const unsigned short &reg, unsigned char &val){
	switch(cartridgeType){
		case 0x0: // ROM only
			break;
		case 0x1 ... 0x3: // MBC1
			// MBC1 has no registers which may be read
			return false;
		case 0x5 ... 0x6: // MBC2
			// MBC2 has no registers which may be read
			return false;
		case 0xB ... 0xD: // MMM01
			break;
		case 0xF ... 0x13: // MBC3
			break;
		case 0x15 ... 0x17: // MBC4
			break;
		case 0x19 ... 0x1E: // MBC5
			break;
		default:
			return false;
	}
	return true;
}

bool Cartridge::readRom(const std::string &fname, bool verbose/*=false*/){
	// Open the rom file
	std::ifstream rom(fname.c_str(), std::ios::binary);
	if(!rom.good())
		return false;
		
	// Read the rom header
	readHeader(rom);
	
	// Read the ROM into memory.
	unsigned int currRomBank = 0;
	rom.seekg(0);
	while(true){
		// Read an entire bank at a time.
		rom.read((char*)getPtrToBank(currRomBank++), 16384);
		
		if(rom.eof() || !rom.good()) break;
	}
	
	// Make the ROM read-only
	setReadOnly();
	
	// Print the cartridge header
	if(verbose) 
		print();
	
	return true;
}

unsigned int Cartridge::readHeader(std::ifstream &f){
	f.seekg(0x0101);
	f.read((char*)&leader, 1); // JP (usually)
	f.read((char*)&programStart, 2); // Program entry point
	f.read((char*)nintendoString, 48); // Nintendo bitmap
	f.read((char*)titleString, 11); titleString[11] = '\0'; // Cartridge title
	f.read((char*)manufacturer, 4); manufacturer[4] = '\0'; // Manufacturer string
	f.read((char*)&gbcFlag, 1); // Gameboy Color flag
	f.read((char*)licensee, 2); licensee[2] = '\0'; // Licensee
	f.read((char*)&sgbFlag, 1); // Super Gameboy flag
	f.read((char*)&cartridgeType, 1); // Cartridge type
	f.read((char*)&romSize, 1); // Size of onboard ROM
	f.read((char*)&ramSize, 1); // Size of onboard RAM (if present)
	f.read((char*)&language, 1); // ROM language
	f.read((char*)&oldLicensee, 1); // Old manufacturer code
	f.read((char*)&versionNumber, 1); // ROM version number
	f.read((char*)&headerChecksum, 1); //
	f.read((char*)&globalChecksum, 2); // High byte first

	// Check for Gameboy Color mode flag
	bGBCMODE = ((gbcFlag & 0x80) != 0);
	
	// Initialize ROM storage
	switch(romSize){
		case 0x00: // 32 kB (2 bank)
			initialize(16384, 2);
			break;
		case 0x01: // 64 kB (4 banks)
			initialize(16384, 4);
			break;
		case 0x02: // 128 kB (8 banks)
			initialize(16384, 8);
			break;
		case 0x03: // 256 kB (16 banks)
			initialize(16384, 16);
			break;
		case 0x04: // 512 kB (32 banks)
			initialize(16384, 32);
			break;
		case 0x05: // 1 MB (64 banks)
			initialize(16384, 64);
			break;
		case 0x06: // 2 MB (128 banks)
			initialize(16384, 128);
			break;
		case 0x07: // 4 MB (256 banks)
			initialize(16384, 256);
			break;
		case 0x52: // 1.1 MB (72 banks)
			initialize(16384, 72);
			break;		
		case 0x53: // 1.2 MB (80 banks)
			initialize(16384, 80);
			break;
		case 0x54: // 1.5 MB (96 banks)
			initialize(16384, 96);
			break;
		default:
			break;
	}

	// Initialize cartridge RAM
	switch(ramSize){
		case 0x0: // No onboard RAM
			break;
		case 0x1: // 2 kB
			ram.initialize(2048);
			break;
		case 0x2: // 8 kB
			ram.initialize(8192);
			break;
		case 0x3: // 32 kB (4 banks)
			ram.initialize(8192, 4);
			break;
		default:
			break;
	}
	
	// Set the default ROM bank for SWAP
	bs = 1;
	
	return 79;
}

void Cartridge::print(){
	std::cout << "Title: " << std::string(titleString) << std::endl;
	std::cout << " ROM: " << size/1024 << " kB\n";
	std::cout << " RAM: " << ram.getSize()/1024 << " kB\n";
	std::cout << " Type: " << getHex(cartridgeType) << std::endl;
	std::cout << " Vers: " << getHex(versionNumber) << "\n";
	std::cout << " Lang: " << (language == 0x0 ? "J" : "E") << std::endl;
	std::cout << " Program entry at " << getHex(programStart) << "\n";
}
