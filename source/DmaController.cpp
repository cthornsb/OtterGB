#include <iostream>

#include "DmaController.hpp"
#include "SystemGBC.hpp"

void DmaController::startTransfer(const unsigned short &dest, const unsigned short &src, const unsigned short &N, const unsigned short &n/*=1*/){
	index = 0;
	destStart = dest;
	srcStart = src;
	nBytes = n;
	nCyclesRemaining = N/n;
}

bool DmaController::onClockUpdate(){
	if(!nCyclesRemaining)
		return false;
	transferByte();
	nCyclesRemaining--;
	return true;
}

void DmaController::transferByte(){
	unsigned char byte;
	for(unsigned short i = 0; i < nBytes; i++){
		// Read a byte from memory.
		sys->read(srcStart + index, byte);
		// Write it to a different location.
		sys->write(destStart + index, byte);
		index++;
	}
}
