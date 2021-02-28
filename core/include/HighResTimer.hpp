#ifndef HIGH_RES_TIMER_HPP
#define HIGH_RES_TIMER_HPP

#include <chrono>

// Make a typedef for clarity when working with chrono.
typedef std::chrono::high_resolution_clock hrclock;

class HighResTimer{
public:
	HighResTimer();

	/** Get the average elapsed time for each start / stop cycle (in seconds)
	  */
	double operator () () {
		return (dTotalTime / nStops);
	}
	
	/** Get the number of start / stop cycles
	  */
	unsigned int getCount() const {
		return nStops;
	}
	
	/** Start the timer
	  */
	void start();
	
	/** Stop the timer and return the time elapsed since start() called (in seconds)
	  * Time elapsed is added to total time accumulator.
	  */
	double stop();
	
	/** Get total time since timer initialization (in seconds)
	  */
	double uptime() const;
	
	/** Reset time accumulator value
	  */
	void reset();
	
private:
	double dTotalTime; ///< Total time between all starts and stops (in seconds)
	
	unsigned int nStops; ///< Number of times the timer has been started and stopped

	hrclock::time_point tInitialization; ///< The time that the timer was initialized
	
	hrclock::time_point tLastStart; ///< The most recent time that the timer was started
};

#endif

