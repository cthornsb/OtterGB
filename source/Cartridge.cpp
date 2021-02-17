
#include <iostream>

#include "Support.hpp"
#include "SystemRegisters.hpp"
#include "Cartridge.hpp"

constexpr unsigned short ROM_ZERO_LOW = 0x0000;
constexpr unsigned short ROM_SWAP_LOW = 0x4000;
constexpr unsigned short ROM_HIGH     = 0x8000;

/////////////////////////////////////////////////////////////////////
// class Cartridge
/////////////////////////////////////////////////////////////////////

Cartridge::Cartridge() : 
	SystemComponent("Cartridge", 0x54524143), // "CART" 
	ramSelect(false), 
	extRamEnabled(false),
	extRamSupport(false),
	batterySupport(false),
	timerSupport(false),
	rumbleSupport(false),
	leader(0),
	programStart(0),
	nintendoString(""),
	titleString(""),
	manufacturer(""),
	gbcFlag(0),
	licensee(""),
	sgbFlag(0),
	cartridgeType(0),
	romSize(0),
	ramSize(0),
	language(0),
	oldLicensee(0),
	versionNumber(0),
	headerChecksum(0),
	globalChecksum(0),
	ram("SRAM", 0x4d415253),
	mbcType(CartMBC::UNKNOWN)
{ 
}

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
	switch(mbcType){
		case CartMBC::ROMONLY: // ROM only
			break;
		case CartMBC::MBC1: // MBC1 (1-3)
			if(reg <= 0x2000){ // RAM enable (write only)
				// Any value written to this area with 0x0A in its lower 4 bits will enable cartridge RAM
				// Note: Type 0x01 does not have cartridge RAM
				extRamEnabled = ((val & 0x0F) == 0x0A);
			}
			else if(reg < 0x4000){ // ROM bank number register (write only)
				// 5-bit register [0x01, 0x1F] (write only)
				// Specify lower 5 bits of ROM bank number
				bs &= 0xE0; // Clear bits 0-4
				bs |= (val & 0x1F); // Set bits 0-4
				// Note: MBC1 translates a 0x0 here as bank 1
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
		case CartMBC::MBC2: // MBC2 (5-6)
			if(reg < 0x2000){ // RAM enable (write only)
				if((val & 0x100) == 0) // Bit 0 of upper address byte must be 0 to enable RAM
					extRamEnabled = true;
			}
			else if(reg < 0x4000){ // ROM bank number (write only)
				if((val & 0x100) != 0) // Bit 0 of upper address byte must be 1 to select ROM bank
					bs = (val & 0x0F);
			}
			break;
		case CartMBC::MMM01: // MMM01
			break;
		case CartMBC::MBC3: // MBC3
			break;
		case CartMBC::MBC4: // MBC4
			break;
		case CartMBC::MBC5: // MBC5
			if(reg < 0x2000){ // RAM enable (write only)
				// Any value written to this area with 0x0A in its lower 4 bits will enable cartridge RAM
				extRamEnabled = ((val & 0x0F) == 0x0A);
			}
			else if(reg < 0x3000){ // Lower 8 bits of ROM bank number register (write only)
				// 9-bit register [0x01, 0x1FF] (write only)
				// Specify lower 8 bits of ROM bank number
				bs &= 0xFF00; // Clear bits 0-7
				bs |= (val & 0x00FF); // Set bits 0-7
			}
			else if(reg < 0x4000){ // 9th bit of ROM bank number (write only)
				// 1-bit register [0,3] (write only)
				// Specify upper 1 bit of ROM bank number
				bs &= 0xFEFF; // Clear bit 9;
				bs |= ((val & 0x0001) << 8); // Set bit 9
			}
			else if(reg < 0x6000){ // RAM bank number
				// Select the RAM bank [0,F]
				ram.setBank(val & 0x0F);
			}
			break;
		default:
			break;	
	}
	return false;
}

bool Cartridge::readRegister(const unsigned short &reg, unsigned char &val){
	switch(mbcType){
		case CartMBC::ROMONLY: // ROM only
			break;
		case CartMBC::MBC1: // MBC1 (1-3)
			break;
		case CartMBC::MBC2: // MBC2 (5-6)
			break;
		case CartMBC::MMM01: // MMM01
			break;
		case CartMBC::MBC3: // MBC3
			break;
		case CartMBC::MBC4: // MBC4
			break;
		case CartMBC::MBC5: // MBC5
			break;
		default:
			break;	
	}
	return false;
}

std::string Cartridge::getCartridgeType() const {
	std::string retval = "UNKNOWN";
	switch(mbcType){
		case CartMBC::ROMONLY: // ROM only
			retval = "ROM ONLY";
			break;
		case CartMBC::MBC1: // MBC1 (1-3)
			retval = "MBC1";
			break;
		case CartMBC::MBC2: // MBC2 (5-6)
			retval = "MBC2";
			break;
		case CartMBC::MMM01: // MMM01
			retval = "MMM01";
			break;
		case CartMBC::MBC3: // MBC3
			retval = "MBC3";
			break;
		case CartMBC::MBC4: // MBC4
			retval = "MBC4";
			break;
		case CartMBC::MBC5: // MBC5
			retval = "MBC5";
			break;
		default:
			break;	
	}
	return retval;
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
		if(rom.eof() || !rom.good() || (currRomBank >= nBanks)) 
			break;
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
	
	// Set cartridge type
	mbcType = CartMBC::UNKNOWN;
	if(cartridgeType == 0x0 || cartridgeType == 0x8 || cartridgeType == 0x9)
		mbcType = CartMBC::ROMONLY;
	else if(cartridgeType >= 0x01 && cartridgeType <= 0x03) // MBC1
		mbcType = CartMBC::MBC1;
	else if(cartridgeType >= 0x05 && cartridgeType <= 0x06) // MBC2
		mbcType = CartMBC::MBC2;
	else if(cartridgeType >= 0x0B && cartridgeType <= 0x0D) // MMM01
		mbcType = CartMBC::MMM01;
	else if(cartridgeType >= 0x0F && cartridgeType <= 0x13) // MBC3
		mbcType = CartMBC::MBC3;
	else if(cartridgeType >= 0x15 && cartridgeType <= 0x17) // MBC4
		mbcType = CartMBC::MBC4;
	else if(cartridgeType >= 0x19 && cartridgeType <= 0x1E) // MBC5
		mbcType = CartMBC::MBC5;
	else
		std::cout << " Warning! Unknown cartridge type (" << getHex(cartridgeType) << ")." << std::endl;
	
	// Set cartridge component flags
	if(cartridgeType <= 30){
		const unsigned char cartFlags[31] = {0x01, 0x01, 0x03, 0x07, 0x00, 0x01, 0x03, 0x00, 0x03, 0x07, 
			                                 0x00, 0x01, 0x03, 0x07, 0x00, 0x0D, 0x0F, 0x01, 0x03, 0x07, 
			                                 0x00, 0x01, 0x03, 0x07, 0x00, 0x01, 0x03, 0x07, 0x11, 0x13, 
			                                 0x17};
		extRamSupport = bitTest(cartFlags[cartridgeType], 1);
		batterySupport = bitTest(cartFlags[cartridgeType], 2);
		timerSupport = bitTest(cartFlags[cartridgeType], 3);
		rumbleSupport = bitTest(cartFlags[cartridgeType], 4);
	}
	
	// Initialize ROM storage
	mem.clear();
	switch (romSize) {
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
	std::cout << "Title: " << getTitleString() << std::endl;
	std::cout << " ROM: " << getRomSize() << " kB (banks=" << nBanks << ", size=" << nBytes / 1024 << " kB)" << std::endl;
	std::cout << " RAM: " << getRamSize() << " kB" << std::endl;
	std::cout << " Type: " << getHex(cartridgeType) << " (" << getCartridgeType() << ")" << std::endl;
	std::cout << " Vers: " << getHex(versionNumber) << std::endl;
	std::cout << " Lang: " << getLanguage() << std::endl;
	std::cout << " Battery? " << (batterySupport ? "Yes" : "No") << std::endl;
	std::cout << " Rumble?  " << (rumbleSupport ? "Yes" : "No") << std::endl;
	std::cout << " Timer?   " << (timerSupport ? "Yes" : "No") << std::endl;
	std::cout << " Program entry at " << getHex(programStart) << std::endl;
}

