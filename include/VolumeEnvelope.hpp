#ifndef VOLUME_ENVELOPE_HPP
#define VOLUME_ENVELOPE_HPP

#include "UnitTimer.hpp"

class VolumeEnvelope : public UnitTimer {
public:
	/** Default constructor
	  */
	VolumeEnvelope() :
		UnitTimer(),
		bAdd(true),
		nVolume(0)
	{ 
	}

	/** Return true if the current output volume is zero and return false otherwise
	  */
	bool silent() const {
		return (nVolume == 0);
	}

	/** Get the current volume
	  */
	float getVolume() const { 
		return (nVolume / 15.f); 
	}

	/** Set the initial 4-bit volume
	  */
	void setVolume(const unsigned char& volume) { 
		nVolume = volume & 0xf; 
	}
	
	/** Enable additive volume mode (louder)
	  */
	void setAddMode(const bool& add) { 
		bAdd = add; 
	}

	/** Trigger the volume envelope, 
	  * Timer is reloaded with period.
	  * Channel volume is reloaded from NRx2.
	  */
	void trigger();

private:
	bool bAdd; ///< Increase volume on timer rollover

	unsigned char nVolume; ///< Current 4-bit volume
	
	/** Counter rolled over, increase or decrease the output volume and refill the timer
	  * If the new volume is outside the range [0, 15] the volume is left un-changed and the timer is not refilled.
	  */
	virtual void rollover();
};

#endif
