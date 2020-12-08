#include <iostream>

#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Support.hpp"
#include "WorkRam.hpp"

constexpr unsigned short WRAM_ZERO_LOW  = 0xC000;
constexpr unsigned short WRAM_SWAP_LOW  = 0xD000;
constexpr unsigned short WRAM_ECHO_LOW  = 0xE000;
constexpr unsigned short WRAM_ECHO_HIGH = 0xFE00;

WorkRam::WorkRam() : SystemComponent("WRAM", 4096, 8) { 
	bs = 1; // Lowest WRAM swap bank is bank 1
}

unsigned char *WorkRam::getPtr(const unsigned int &loc){
	if(loc < WRAM_ZERO_LOW || loc >= WRAM_ECHO_HIGH)
		return 0x0;

	unsigned char *ptr = 0x0;
	if(loc < WRAM_SWAP_LOW) // Bank 0
		ptr = &mem[0][loc-offset];
	else if(loc < WRAM_ECHO_LOW) // Bank 1-7
		ptr = &mem[bs][loc-offset-4096];
	else if(loc < WRAM_ECHO_HIGH) // Echo of bank 0
		ptr = &mem[0][loc-offset-8192];
		
	return ptr;
}

bool WorkRam::preWriteAction(){
	if(writeLoc < WRAM_ZERO_LOW || writeLoc >= WRAM_ECHO_HIGH)
		return false;

	if(writeLoc < WRAM_SWAP_LOW){ // Bank 0
		writeBank = 0;
		return true;
	}
	else if(writeLoc < WRAM_ECHO_LOW){ // Bank 1-7
		writeLoc = writeLoc - 4096;
		writeBank = bs;
		return true;
	}
	else if(writeLoc < WRAM_ECHO_HIGH){ // Echo of bank 0
		writeLoc = writeLoc - 8192;
		writeBank = 0;
		return true;
	}

	return false;
}

bool WorkRam::preReadAction(){
	if(readLoc < WRAM_ZERO_LOW || readLoc >= WRAM_ECHO_HIGH)
		return false;

	if(readLoc < WRAM_SWAP_LOW){ // Bank 0
		readBank = 0;
		return true;
	}
	else if(readLoc < WRAM_ECHO_LOW){ // Bank 1-7
		readLoc = readLoc - 4096;
		readBank = bs;
		return true;
	}
	else if(readLoc < WRAM_ECHO_HIGH){ // Echo of bank 0
		readLoc = readLoc - 8192;
		readBank = 0;
		return true;
	}
	
	return false;
}

bool WorkRam::writeRegister(const unsigned short &reg, const unsigned char &val){
	if(reg != 0xFF70)
		return false;
	unsigned char wramBank = rSVBK->getBits(0,2); // Select WRAM bank (1-7)
	if(wramBank == 0x0) 
		wramBank = 0x1; // Select bank 1 instead
	setBank(wramBank);
	return true;
}

bool WorkRam::readRegister(const unsigned short &reg, unsigned char &dest){	
	if(reg != 0xFF70) 
		return false;
	return true;	
}

void WorkRam::defineRegisters(){
	sys->addSystemRegister(this, 0x70, rSVBK, "SVBK", "33300000");
}
