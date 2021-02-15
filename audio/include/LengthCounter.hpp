#ifndef LENGTH_COUNTER_HPP
#define LENGTH_COUNTER_HPP

#include "UnitTimer.hpp"

class LengthCounter : public UnitTimer {
public:
	/** Default constructor (maximum length of 64 ticks)
	  */
	LengthCounter() :
		UnitTimer(64),
		bRefilled(false)
	{
	}
	
	/** Explicit maximum length constructor
	  */
	LengthCounter(const unsigned short& maxLength) :
		UnitTimer(maxLength),
		bRefilled(false)
	{
	}
	
	/** Get the remaining audio length 
	  */
	unsigned short getLength() const { 
		return nCounter; 
	}

	/** Return true if the length counter was automatically refilled when triggered with a length of zero
	  */
	bool wasRefilled() const {
		return bRefilled;
	}
	
	/** Return true if the length has depleted, and return false if at least one length unit remains
	  */
	bool empty() const {
		return (nCounter == 0);
	}
	
	/** Set the audio length
	  */
	void setLength(const unsigned char& length){
		nCounter = nPeriod - length;
	}
	
	/** Trigger the length counter
	  * If the current length is zero, the length is refilled with the maximum
	  */
	void trigger();

	/** Upon enabling the length counter, check whether or not an extra length clock should be performed
	  * An extra clock may occur when enabled on a frame sequencer cycle which clocks the length counter.
	  * If the counter is going from disabled to enabled with a non-zero length an extra clock will be performed.
	  * If the extra clock caused the counter to roll over, and the length counter is being enabled without being triggered, the channel should be disabled.
	  * @param nTicks The current 512 Hz frame sequencer cycle number
	  * @param bWillBeTriggered Bit 7 of NR4x indicating that the length counter will be triggered once enabled
	  * @return True if the extra clock rolled over the length counter and return false otherwise
	  */
	bool extraClock(const unsigned int& nTicks, bool bWillBeTriggered);

	/** Upon triggering the length counter, check whether or not an extra length clock should be performed
	  * An extra clock may occur when triggered on a frame sequencer cycle which clocks the length counter.
	  * If the length counter is enabled and triggering it caused a previously empty timer to be refilled with the maximum length an extra clock will be performed.
	  * @param nTicks The current 512 Hz frame sequencer cycle number
	  * @return True if an extra length clock was performed and return false otherwise
	  */	
	bool extraClock(const unsigned int& nTicks);

	/** Reset all counter values and flags
	  * Period and frequency are not reset because they are used to compute audio length.
	  */
	void reset() override ;
	
private:
	bool bRefilled; ///< Flag indicating that length counter was refilled with maximum length on most recent trigger

	/** Length counter was depleted
	  * Do not automatically reload the length counter
	  */
	void rollover() override { }
};

#endif
