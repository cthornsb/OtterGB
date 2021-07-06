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
		bUpdating(false),
		nVolume(0),
		nInitialVolume(0)
	{ 
	}

	/** Get the current volume
	  */
	unsigned char operator () () const {
		return nVolume;
	}
	
	/** Scale an input volume by the current envelope volume
	  */
	unsigned char operator () (const unsigned char& input) const {
		return (unsigned char)(input * (nVolume / 15.f));
	}

	/** Return true if the current output volume is zero and return false otherwise
	  */
	bool silent() const {
		return (nVolume == 0);
	}

	/** Get the current volume as a float in range [0, 1]
	  */
	float getVolume() const { 
		return (nVolume / 15.f); 
	}

	/** Return true if volume envelope is in add mode (increasing volume)
	  */
	bool getAddMode() const {
		return bAdd;
	}

	/** Set the initial 4-bit volume as well as the current output volume
	  */
	void setVolume(const unsigned char& volume) { 
		nInitialVolume = volume & 0xf;
		nVolume = nInitialVolume; 
	}
	
	/** Enable additive volume mode (louder)
	  */
	void setAddMode(const bool& add) { 
		bAdd = add; 
	}

	/** Add an extra sequencer clock to the volume envelope timer
	  */
	void addExtraClock() {
		nCounter++;
	}

	/** Update volume envelope initial volume, add mode, and sequencer period.
	  * Writing to NRx2 while sound playing:
	  * -if Envelope period was 0 (and still updating) -> volume incremented by 1
	  * else if envelope was in subtract mode -> volume incremented by 2
	  * -if mode changed (+ -> - or - -> +) -> volume set to 16
	  * -nVolume &= 0xf;
	  */
	void update(const unsigned char& nrx2);

	/** Trigger the volume envelope, 
	  * Timer is reloaded with period.
	  * Channel volume is reloaded from NRx2.
	  */
	void trigger();

	/** Reload the unit timer with its period (or with 8 in the event that the period is zero)
	  */
	void reload() override ;

private:
	bool bAdd; ///< Increase volume on timer rollover

	bool bUpdating; ///< Set when volume is still being automatically updated

	unsigned char nVolume; ///< Current 4-bit volume
	
	unsigned char nInitialVolume; ///< Initial 4-bit volume (register NRx2)
	
	/** Counter rolled over, increase or decrease the output volume and refill the timer
	  * If the new volume is outside the range [0, 15] the volume is left un-changed and the timer is not refilled.
	  */
	void rollover() override ;
};

#endif
