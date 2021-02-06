#ifndef UNIT_TIMER_HPP
#define UNIT_TIMER_HPP

class UnitTimer{
public:
	/** Default constructor
	  */
	UnitTimer() :
		bEnabled(false),
		nPeriod(0),
		nCounter(0),
		nFrequency(0),
		nPeriodMultiplier(1),
		nCyclesSinceLastClock(0),
		nMasterClockPeriod(8)
	{
	}

	/** Constructor specifying the master clock period (in 1 MHz clock ticks)
	  */
	UnitTimer(const unsigned int& period) :
		bEnabled(false),
		nPeriod(period),
		nCounter(0),
		nFrequency(0),
		nPeriodMultiplier(1),
		nCyclesSinceLastClock(0),
		nMasterClockPeriod(8)
	{
	}
	
	/** Get the current period of the timer (computed from the channel's 11-bit frequency)
	  */
	unsigned short getPeriod() const { 
		return (nPeriod / nPeriodMultiplier); 
	}
	
	/** Get the reciprocal of the 11-bit frequency of the timer
	  */
	unsigned short getFrequency() const {
		return (2048 - getPeriod());
	}

	/** Get the actual frequency (in Hz)
	  */
	virtual float getRealFrequency() const {
		return ((1048576.f / nMasterClockPeriod) / (2048 - nFrequency)); // in Hz
	}

	/** Return true if the timer is enabled, and return false otherwise
	  */
	bool isEnabled() const {
		return bEnabled;
	}

	/** Set the period of the timer (in 1 MHz master clock ticks)
	  */
	void setPeriod(const unsigned short& period) {
		nPeriod = nPeriodMultiplier * period;
	}

	/** Set the frequency and period of the timer
	  */
	void setFrequency(const unsigned short& freq){
		nFrequency = (freq & 0x7ff);
		setPeriod(2048 - nFrequency);
	}
	
	/** Set the frequency and period of the timer using two input bytes
	  */
	void setFrequency(const unsigned char& lowByte, const unsigned char& highByte){
		nFrequency = ((highByte & 0x7) << 8) + lowByte;
		setPeriod(2048 - nFrequency);
	}

	/** Enable the timer
	  */
	void enable() { 
		nCyclesSinceLastClock = 0; // Reset master clock
		bEnabled = true; 
		userEnable();
	}
	
	/** Disable the timer
	  */
	void disable() { 
		bEnabled = false; 
		userDisable();
	}

	/** Reset the counter to the period of the timer
	  */
	void reset() {
		nCounter = nPeriod;
	}

	/** Clock the timer, returning true if the phase rolled over and returning false otherwise
	  */
	bool clock();
	
	/** Reload the unit timer with its period
	  */
	virtual void reload();

protected:
	bool bEnabled; ///< Timer enabled flag
	
	unsigned short nPeriod; ///< Period of timer
	
	unsigned short nCounter; ///< Current timer value
	
	unsigned short nFrequency; ///< 11-bit frequency (0 to 2047)
	
	unsigned short nPeriodMultiplier; ///< Period multiplier factor
	
	unsigned int nCyclesSinceLastClock; ///< Number of input clock ticks since last unit clock rollover
	
	unsigned int nMasterClockPeriod; ///< Number of input clock ticks per unit clock tick
	
	/** Additional operations performed whenever enable() is called
	  */
	virtual void userEnable() { }
	
	/** Additional operations performed whenever disable() is called
	  */
	virtual void userDisable() { }
	
	/** Method called when unit timer clocks over (every nPeriod input clock ticks)
	  */	
	virtual void rollover() { 
		reload();
	}
};

#endif
