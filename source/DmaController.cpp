#include <iostream>

#include "DmaController.hpp"
#include "SystemGBC.hpp"
#include "SystemRegisters.hpp"
#include "Support.hpp"

void DmaController::startTransferOAM(){
	// Source:      XX00-XX9F with XX in range [00,F1]
	// Destination: FE00-FE9F
	// DMA transfer takes 160 us (80 us in double speed) and CPU may only access HRAM during this interval
	index = 0;
	destStart = 0xFE00;
	srcStart = (((*rDMA) & 0x00F1) << 8);
	nBytes = 1;
	nCyclesRemaining = 160;
}

void DmaController::startTransferVRAM(){
	unsigned char dmaSourceH = (*rHDMA1);
	unsigned char dmaSourceL = (*rHDMA2 & 0xF0); // Bits (4-7) of LSB of source address
	unsigned char dmaDestinationH = (*rHDMA3 & 0x1F); // Bits (0-4) of MSB of destination address
	unsigned char dmaDestinationL = (*rHDMA4 & 0xF0); // Bits (4-7) of LSB of destination address
					
	// Start a VRAM DMA transfer
	// source:      0000-7FF0 or A000-DFF0
	// destination: 8000-9FF0 (VRAM)
	index = 0;
	destStart = 0x8000 + ((dmaDestinationH & 0x00FF) << 8) + dmaDestinationL;
	srcStart = ((dmaSourceH & 0x00FF) << 8) + dmaSourceL;
	nBytes = 2; // VRAM DMA takes ~1 us per two bytes transferred

	unsigned short transferLength = (((*rHDMA5) & 0x7F) * + 1) * 0x10; // Specifies the number of bytes to transfer
	nCyclesRemaining = transferLength / nBytes;
	
	// Transfer mode:
	// 0: Transfer all bytes at once
	// 1: Transfer 0x10 bytes per HBlank
	transferMode = ((*rHDMA5) & 0x80) == 0x80; // 0: General DMA, 1: H-Blank DMA
}

bool DmaController::onClockUpdate(){
	if(transferMode || !nCyclesRemaining)
		return false;
	transferByte();
	nCyclesRemaining--;
	return true;
}

void DmaController::onHBlank(){
	if(transferMode && nCyclesRemaining){
		transferByte();
		nCyclesRemaining--;
	}
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
