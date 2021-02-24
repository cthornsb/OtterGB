#include "VolumeEnvelope.hpp"

void VolumeEnvelope::trigger(){
	reload(); // Volume envelope timer reloaded with period
	nVolume = nInitialVolume; // Reload initial channel volume
}

void VolumeEnvelope::rollover(){
	if(nPeriod){
		if(bAdd){ // Add (louder)
			if(nVolume + 1 <= 15){
				nVolume++;
				reload(); // Refill volume counter with its period
			}
		}
		else{ // Subtract (quieter)
			if(nVolume > 0){
				nVolume--;
				reload(); // Refill volume counter with its period
			}
		}
	}
}

