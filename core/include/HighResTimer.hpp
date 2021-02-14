#ifndef HIGH_RES_TIMER_HPP
#define HIGH_RES_TIMER_HPP

#include <chrono>

// Make a typedef for clarity when working with chrono.
typedef std::chrono::high_resolution_clock hrclock;

class HighResTimer{
public:
	HighResTimer();

	/** Get the time elapsed since the timer was reset
	  */
	double operator () () {
		return uptime();
	}
	
	/** Start the timer
	  */
	void start();
	
	/** Stop the timer and return the time elapsed since start() called
	  */
	double stop() const;
	
	/** Get total time since timer initialization
	  */
	double uptime() const;
	
	/** Reset time accumulator value
	  */
	void reset();
	
private:
	hrclock::time_point tInitialization; ///< The time that the timer was initialized
	
	hrclock::time_point tLastStart; ///< The most recent time that the timer was started
};

#endif

