#include "VolumeEnvelope.hpp"

void VolumeEnvelope::update(const unsigned char& nrx2){
	bool oldAddMode = bAdd;
	setVolume((nrx2 & 0xf0) >> 4);
	/*if(!nPeriod && bUpdating){ // Old envelope period is zero and still updating volume, add one to volume
		nVolume += 1;
	}
	else if(!bAdd){ // Old envelope is in subtract mode, add two to volume
		nVolume += 2;
	}*/	
	setAddMode((nrx2 & 0x8) == 0x8);
	setPeriod(nrx2 & 0x7);
	if(bAdd != oldAddMode){ // Add mode switched, set volume to 16-volume
		nVolume = 16 - nVolume;
	}
	nVolume &= 0x0f;
	bUpdating = true;
}

void VolumeEnvelope::trigger(){
	// TODO if next sequencer tick will clock volume envelope, nCounter is increased by 1
	reload(); // Volume envelope timer reloaded with period
	nVolume = nInitialVolume; // Reload initial channel volume
	bUpdating = true;
}

void VolumeEnvelope::reload(){
	nCounter = (nPeriod != 0 ? nPeriod : 8); // Reset period counter
}

void VolumeEnvelope::rollover(){
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
	reload();
}

