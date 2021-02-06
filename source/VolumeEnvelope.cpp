#include "VolumeEnvelope.hpp"

void VolumeEnvelope::trigger(){
	reload(); // Volume envelope timer reloaded with period
	//nVolume = ; // Initial channel volume reloaded from NRx2
}

void VolumeEnvelope::rollover(){
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

