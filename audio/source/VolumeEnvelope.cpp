#include "VolumeEnvelope.hpp"

void VolumeEnvelope::update(const unsigned char& nrx2){
	// Set initial envelope volume. Current volume will not be set until triggered
	nInitialVolume = ((nrx2 & 0xf0) >> 4); // Bits 4-7

	// Set envelope volume add mode (0: Subtract, 1: Add)
	bAdd = ((nrx2 & 0x8) == 0x8); // Bit 3

	// Set envelope period
	unsigned short envelopePeriod = (unsigned short)(nrx2 & 0x7); // Bits 0-2
	if (envelopePeriod > 0) {
		setPeriod(envelopePeriod);
		bUpdating = true;
	}
	else { // Envelope period is zero
		// Disable automatic volume updates
		setPeriod(0);
		bUpdating = false;

		// Using add mode with period == 0 allows manual volume control.
		// Writing 0x08 to NRx2 increments the current volume by one (without triggering).
		if (bAdd)
			nVolume++;
		nVolume &= 0x0f;
	}
}

void VolumeEnvelope::trigger(){
	// TODO if next sequencer tick will clock volume envelope, nCounter is increased by 1
	reload(); // Volume envelope timer reloaded with period
	nVolume = nInitialVolume; // Reload initial channel volume
	bUpdating = true;
	if (nPeriod)
		bEnabled = true;
	else
		bEnabled = false;
}

void VolumeEnvelope::reload(){
	nCounter = (nPeriod != 0 ? nPeriod : 8); // Reset period counter
}

void VolumeEnvelope::rollover(){
	reload();
	if(nPeriod && bUpdating){
		if(bAdd){ // Add (louder)
			if(nVolume + 1 <= 15)
				nVolume++;
			else
				bUpdating = false;
		}
		else{ // Subtract (quieter)
			if(nVolume > 0)
				nVolume--;
			else
				bUpdating = false;
		}
	}
}

