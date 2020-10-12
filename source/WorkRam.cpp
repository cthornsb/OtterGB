#include <iostream>

#include "Support.hpp"
#include "WorkRam.hpp"

#define WRAM_ZERO_LOW 0xC000
#define WRAM_SWAP_LOW 0xD000
#define WRAM_HIGH     0xE000
#define WRAM_ECHO_LOW  0xE000
#define WRAM_ECHO_HIGH 0xFE00

WorkRam::WorkRam() : SystemComponent(4096, 8) { 
	bs = 1; // Lowest WRAM swap bank is bank 1
}

bool WorkRam::preWriteAction(){
	if(writeLoc >= WRAM_ZERO_LOW && writeLoc < WRAM_SWAP_LOW){ // Bank 0
		writeBank = 0;
		return true;
	}
	else if(writeLoc >= WRAM_SWAP_LOW && writeLoc < WRAM_ECHO_LOW){ // Bank 1-7
		writeLoc = writeLoc - 4096;
		writeBank = bs;
		return true;
	}
	else if(writeLoc >= WRAM_ECHO_LOW && writeLoc < WRAM_ECHO_HIGH){ // Echo of bank 0
		writeLoc = writeLoc - 8192;
		writeBank = 0;
		return true;
	}

	return false;
}

bool WorkRam::preReadAction(){
	if(readLoc >= WRAM_ZERO_LOW && readLoc < WRAM_SWAP_LOW){ // Bank 0
		readBank = 0;
		return true;
	}
	else if(readLoc >= WRAM_SWAP_LOW && readLoc < WRAM_ECHO_LOW){ // Bank 1-7
		readLoc = readLoc - 4096;
		readBank = bs;
		return true;
	}
	else if(readLoc >= WRAM_ECHO_LOW && readLoc < WRAM_ECHO_HIGH){ // Echo of bank 0
		readLoc = readLoc - 8192;
		readBank = 0;
		return true;
	}
	
	return false;
}
