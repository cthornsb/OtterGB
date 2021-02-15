#ifndef FREQUENCY_SWEEP_HPP
#define FREQUENCY_SWEEP_HPP

#include "UnitTimer.hpp"

class FrequencySweep : public UnitTimer {
public:
	/** Default constructor
	  */
	FrequencySweep() :
		UnitTimer(),
		bOverflow(false),
		bOverflow2(false),
		bNegate(false),
		nTimer(0),
		nShift(0),
		nShadowFrequency(0),
		nNewFrequency(0),
		extTimer(0x0)
	{
	}

	/** Return true if primary frequency calculation overflowed, and return false otherwise
	  */	
	bool overflowed() const { 
		return bOverflow; 
	}
	
	/** Return true if secondary frequency calculation overflowed, and return false otherwise
	  */
	bool overflowed2() const {
		return bOverflow2;
	}
	
	/** Get the sweep bit shift
	  */
	unsigned char getBitShift() const { 
		return nShift; 
	}
	
	/** Get the most recently computed new frequency value
	  */
	unsigned short getNewFrequency() const { 
		return nShadowFrequency; 
	}

	/** Enable negate mode such that the new frequency (f') is given by
	  * f' = f - (f >> shift)
	  * using 11-bit two's complement.
	  * @return False if negate mode was previously used in a calculation and if mode is being switched from enabled to disabled, else return true
	  */
	bool setNegate(const bool& negate);

	/** Set the sweep period (in 128 Hz frame sequencer clocks)
	  * If the sweep is changed from zero to non-zero with an empty timer, the timer is immediately reloaded with the new period.
	  */
	void setPeriod(const unsigned short& period) override ;
	
	/** Set the sweep bit shift such that the new frequency (f') is given by
	  * f' = f +/- (f >> shift)
	  */
	void setBitShift(const unsigned char& shift) { 
		nShift = shift; 
	}
	
	/** Set a pointer to the external timer from which the initial frequency will be read
	  * and to which new frequencies will be written
	  */
	void setUnitTimer(UnitTimer* timer) { 
		extTimer = timer; 
	}
	
	/** Trigger the frequency sweep, performing the following actions
	  * The external timer's frequency (period) is copied to the shadow frequency
	  * The sweep timer is refilled with its period (or with 8 in the event that the period is zero)
	  * Overflow flags are reset
	  * If either the period or sweep bit-shift are non-zero the sweep is enabled, otherwise it is disabled
	  * If the sweep bit-shift is non-zero, a new frequency is calculated immediately
	  */
	void trigger();

	/** Compute a new frequency f' = f +/- (f >> shift)
	  * @return True if the new frequency is < 2048 and return false otherwise
	  */
	bool compute();
	
	/** Reload the unit timer with its period (or with 8 in the event that the period is zero)
	  */
	void reload() override ;

private:
	bool bOverflow;
	
	bool bOverflow2;
	
	bool bNegate;
	
	bool bNegateModeUsed; ///< Flag indicating that negate mode was used for a calculation at least once
	
	unsigned char nTimer; // Sweep timer
	
	unsigned char nShift; // Sweep shift
	
	unsigned short nShadowFrequency;
	
	unsigned short nNewFrequency;
	
	UnitTimer* extTimer;
	
	/** Sweep timer rolled over, the following actions are performed
	  * The sweep timer is refilled with its period (or with 8 in the event that the period is zero).
	  * If the sweep period is non-zero a new frequency is computed.
	  * If the new frequency does not overflow and the sweep bit-shift is non-zero the .
	  *   new frequency is written to the shadow frequency and to the external unit timer.
	  * If the new frequency DID overflow, the overflow flag is set and the sweep timer is disabled.
	  */
	void rollover() override ;
};

#endif
