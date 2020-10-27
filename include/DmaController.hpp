#ifndef DMA_CONTROLLER_HPP
#define DMA_CONTROLLER_HPP

#include "SystemComponent.hpp"

class DmaController : public SystemComponent {
public:
	DmaController() : SystemComponent(), transferMode(0), nCyclesRemaining(0), index(0), nBytes(1), srcStart(0), destStart(0) { }

	bool active() const { return (nCyclesRemaining != 0); }

	void startTransferOAM();
	
	void startTransferVRAM();

	// The system timer has no associated RAM, so return false.
	virtual bool preWriteAction(){ return false; }
	
	// The system timer has no associated RAM, so return false.
	virtual bool preReadAction(){ return false; }
	
	virtual bool onClockUpdate();
	
	void onHBlank();

private:
	bool transferMode; ///< 0: General DMA, 1: H-Blank DMA
	
	unsigned short nCyclesRemaining; ///< The number of system clock cycles remaining.
	unsigned short index; ///< Current index in system memory.
	unsigned short nBytes; ///< The number of bytes transferred per cycle.
	unsigned short srcStart; ///< Start location of the source block in memory.
	unsigned short destStart; ///< Start location of the destination block in memory.
	
	/** Transfer the next chunk of bytes. Increment the memory index by nBytes.
	  */
	void transferByte();
};

#endif
