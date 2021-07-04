
#include <iostream>

#include "Support.hpp"
#include "Cartridge.hpp"

constexpr unsigned short ROM_ZERO_LOW = 0x0000;
constexpr unsigned short ROM_SWAP_LOW = 0x4000;
constexpr unsigned short ROM_HIGH     = 0x8000;

/////////////////////////////////////////////////////////////////////
// class Cartridge
/////////////////////////////////////////////////////////////////////

Cartridge::Cartridge() : 
	SystemComponent("Cartridge", 0x54524143), // "CART" 
	bLoaded(false),
	extRamSupport(false),
	batterySupport(false),
	timerSupport(false),
	rumbleSupport(false),
	leader(0),
	programStart(0),
	bootBitmapString(""),
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
	mbc()
{ 
}

bool Cartridge::writeToRam(const unsigned short& addr, const unsigned char& value) {
	if (!extRamSupport || !getExternalRamEnabled())
		return false;
	if (addr >= 0xa000 && addr < 0xc000) {
		if (mbc->writeToRam(addr, value))
			return true;
		ram.write(addr, value);
		return true;
	}
	return false;
}

bool Cartridge::readFromRam(const unsigned short& addr, unsigned char& value) {
	if (!extRamSupport || !getExternalRamEnabled())
		return false;
	if (addr >= 0xa000 && addr < 0xc000) {	
		if (mbc->readFromRam(addr, value))
			return true;	
		ram.read(addr, value);
		return true;
	}
	return false;
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

bool Cartridge::writeRegister(const unsigned short &reg, const unsigned char &val) {
	return mbc->writeRegister(reg, val);
}

bool Cartridge::readRegister(const unsigned short &reg, unsigned char &val) {
	return mbc->readRegister(reg, val);
}

bool Cartridge::readRom(const std::string &fname, bool verbose/*=false*/){
	// Open the rom file
	std::ifstream rom(fname.c_str(), std::ios::binary);
	if(!rom.good())
		return false;

	// Unload previously loaded ROM
	if (isLoaded()) 
		unload();

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
	
	// Set ROM loaded flag
	bLoaded = true;
	
	return true;
}

void Cartridge::unload(){
	mem.clear();
	bLoaded = false;
}

unsigned int Cartridge::readHeader(std::ifstream &f){
	f.seekg(0x0101);
	f.read((char*)&leader, 1); // JP (usually)
	f.read((char*)&programStart, 2); // Program entry point
	f.read((char*)bootBitmapString, 48); // Boot bitmap
	f.read((char*)titleString, 11); titleString[11] = '\0'; // Cartridge title
	f.read((char*)manufacturer, 4); manufacturer[4] = '\0'; // Manufacturer string
	f.read((char*)&gbcFlag, 1); // CGB flag
	f.read((char*)licensee, 2); licensee[2] = '\0'; // Licensee
	f.read((char*)&sgbFlag, 1); // SGB flag
	f.read((char*)&cartridgeType, 1); // Cartridge type
	f.read((char*)&romSize, 1); // Size of onboard ROM
	f.read((char*)&ramSize, 1); // Size of onboard RAM (if present)
	f.read((char*)&language, 1); // ROM language
	f.read((char*)&oldLicensee, 1); // Old manufacturer code
	f.read((char*)&versionNumber, 1); // ROM version number
	f.read((char*)&headerChecksum, 1); //
	f.read((char*)&globalChecksum, 2); // High byte first

	// Set cartridge memory bank controller (MBC)
	if(cartridgeType == 0x0 || cartridgeType == 0x8 || cartridgeType == 0x9)
		mbc.reset(new mbcs::NoMBC);
	else if(cartridgeType >= 0x01 && cartridgeType <= 0x03) // MBC1
		mbc.reset(new mbcs::MBC1);
	else if(cartridgeType >= 0x05 && cartridgeType <= 0x06) // MBC2
		mbc.reset(new mbcs::MBC2);
	else if(cartridgeType >= 0x0F && cartridgeType <= 0x13) // MBC3
		mbc.reset(new mbcs::MBC3);
	else if(cartridgeType >= 0x19 && cartridgeType <= 0x1E) // MBC5
		mbc.reset(new mbcs::MBC5);
	else{
		mbc.reset(new mbcs::MemoryController);
		std::cout << " [" << sName << "] Error! Unknown cartridge type (" << getHex(cartridgeType) << ")." << std::endl;
	}
	
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
	
	// Build list of MBC registers
	mbc->createRegisters();
	
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
		case 0x08: // 8 MB (512 banks)
			initialize(16384, 512);
			break;
		case 0x52: // 1.1 MB (72 banks, unofficial)
			initialize(16384, 72);
			break;
		case 0x53: // 1.2 MB (80 banks, unofficial)
			initialize(16384, 80);
			break;
		case 0x54: // 1.5 MB (96 banks, unofficial)
			initialize(16384, 96);
			break;
		default:
			std::cout << " [" << sName << "] Error! Unknown cartridge ROM size (" << getHex(romSize) << ")." << std::endl;
			break;
	}

	// Initialize cartridge RAM
	switch(ramSize){
		case 0x0: // No onboard RAM
			break;
		case 0x1: // 2 kB (unofficial size)
			ram.initialize(2048);
			break;
		case 0x2: // 8 kB
			ram.initialize(8192);
			break;
		case 0x3: // 32 kB (4 banks)
			ram.initialize(8192, 4);
			break;
		case 0x4: // 128 kB (16 banks)
			ram.initialize(8192, 16);
			break;
		case 0x5: // 64 kB (8 banks)
			ram.initialize(8192, 8);
			break;
		default:
			std::cout << " [" << sName << "] Error! Unknown cartridge RAM size (" << getHex(ramSize) << ")." << std::endl;
			break;
	}

	// Link MBC to cartridge ROM and RAM
	mbc->setMemory(this, &ram);
	
	// Set the default ROM bank for SWAP
	bs = 1;

	return 79;
}

void Cartridge::print(){
	std::cout << " Title: " << getTitleString() << std::endl;
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

bool Cartridge::onClockUpdate() {
	if (timerSupport) {
		return mbc->onClockUpdate();
	}
	return false;
}
