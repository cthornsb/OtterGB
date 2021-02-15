#ifndef COMPONENT_TIMER_HPP
#define COMPONENT_TIMER_HPP

class ComponentTimer {
public:
	/** Default constructor
	  */
	ComponentTimer() : 
		nCyclesSinceLastTick(0), 
		nPeriod(1), 
		nCounter(0), 
		bEnabled(true) 
	{ 
	}
	
	/** Period constructor
	  */
	ComponentTimer(const unsigned short &period) : 
		nCyclesSinceLastTick(0), 
		nPeriod(period), 
		nCounter(0), 
		bEnabled(true) 
	{ 
	}

	/** Get the current timer period
	  */
	unsigned short getTimerPeriod() const { 
		return nPeriod; 
	}
	
	/** Get the current timer counter
	  */
	unsigned short getTimerCounter() const { 
		return nCounter; 
	}
	
	/** Enable the timer
	  * Timer will be incremented when clocked.
	  */
	void enableTimer(){ 
		bEnabled = true; 
	}
	
	/** Disable the timer
	  * Timer will do nothing when clocked.
	  */
	void disableTimer(){ 
		bEnabled = false; 
	}	

	/** Set the timer period
	  */
	void setTimerPeriod(const unsigned short &period){ 
		nPeriod = period; 
	}
	
	/** Clock the timer
	  * If incrementing the timer causes it to roll over, rollover() is called and true is returned
	  */
	bool clock();
	
	/** Reset the number of cycles since the last time the clock rolled over
	  */
	void reset(){ 
		nCyclesSinceLastTick = 0; 
	}

protected:
	unsigned short nCyclesSinceLastTick; ///< Number of clock ticks since last timer rollover
	
	unsigned short nPeriod; ///< Timer period
	
	unsigned short nCounter; ///< Current timer counter
	
	bool bEnabled; ///< Enabled flag

	/** Timer rollover event
	  * Called when clocking the timer causes it to reset
	  */
	virtual void rollover(){ 
		reset();
	}
};

#endif
