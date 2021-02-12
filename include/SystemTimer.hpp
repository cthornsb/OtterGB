#ifndef SYSTEM_TIMER_HPP
#define SYSTEM_TIMER_HPP

#include <chrono>

#include "SystemComponent.hpp"

// Make a typedef for clarity when working with chrono.
typedef std::chrono::high_resolution_clock hrclock;

class HighResTimer{
public:
	HighResTimer();

	/** Stop the timer and return the duration
	  */
	double operator () () {
		return stop();
	}
	
	/** Start the timer
	  */
	void start();
	
	/** Stop the timer, storing timer duration to dWallTime and returning the duration
	  */
	double stop();
	
	/** Get total time since timer initialization
	  */
	double uptime() const;
	
private:
	hrclock::time_point tInitialization; ///< The time that the timer was initialized
	
	hrclock::time_point tLastStart; ///< The most recent time that the timer was started
};

class SystemClock : public SystemComponent {
public:
	SystemClock();

	double getFramerate() const { return framerate; }
	
	void setFramerateMultiplier(const float &freq);

	void setDoubleSpeedMode();
	
	void setNormalSpeedMode();

	double getFrequency() const { return currentClockSpeed; }

	double getCyclesPerSecond() const { return cyclesPerSecond; }
	
	unsigned int getCyclesSinceVBlank() const { return cyclesSinceLastVSync; }
	
	unsigned int getCyclesSinceHBlank() const { return cyclesSinceLastHSync; }
	
	unsigned char getDriverMode() const { return lcdDriverMode; }
	
	bool getVSync() const { return vsync; }
	
	/** Poll the VSync (VBlank) flag and reset it.
	  */
	bool pollVSync(){ return (vsync ? !(vsync = false) : false); }
	
	/** Perform one clock tick.
	  * @return True if the system has entered VBlank, and false otherwise.
	  */
	virtual bool onClockUpdate();
	
	/** Sleep until the start of the next VSync cycle (i.e. wait until the
	  * start of the next frame). Useful for maintaining desired framerate
	  * without advancing the system clock.
	  */
	void wait();
	
	/** Reset cycle counters.
	  */
	void resetScanline();

private:
	bool vsync;

	unsigned int cyclesSinceLastVSync; ///< The number of cycles since the last Vertical Sync
	unsigned int cyclesSinceLastHSync; ///< The number of cycles since the last  Horizontal Sync

	unsigned int currentClockSpeed; ///< The current number of machine clock cycles per second
	unsigned int cyclesPerVSync; ///< The current number of clock cycles per vertical sync
	unsigned int cyclesPerHSync; ///< The current number of clock cycles per horizontal sync

	unsigned char lcdDriverMode;

	double framerate;
	
	double framePeriod; ///< Wall clock time between successive frames (microseconds)

	hrclock::time_point timeOfInitialization; ///< The time that the system clock was initialized
	hrclock::time_point timeOfLastVSync; ///< The time at which the screen was last refreshed
	hrclock::time_point cycleTimer;

	unsigned int cycleCounter;

	double cyclesPerSecond;

	unsigned int modeStart[4];

	/** Increment the current scanline (register LY).
	  * @return True if there is coincidence with register LYC, and return false otherwise.
	  */
	bool incrementScanline();

	/** Sleep for the required amount of time to maintain the set framerate.
	  */
	void waitUntilNextVSync();

	void mode0Interrupt();

	void mode1Interrupt();

	void mode2Interrupt();
};

class ComponentTimer {
public:
	ComponentTimer() : nCyclesSinceLastTick(0), timerPeriod(1), timerCounter(0), timerEnable(true) { }
	
	ComponentTimer(const unsigned short &period) : nCyclesSinceLastTick(0), timerPeriod(period), timerCounter(0), timerEnable(true) { }
	
	void enableTimer(){ timerEnable = true; }
	
	void disableTimer(){ timerEnable = false; }	

	void setTimerPeriod(const unsigned short &period){ timerPeriod = period; }
	
	unsigned short getTimerPeriod() const { return timerPeriod; }
	
	unsigned short getTimerCounter() const { return timerCounter; }

	void reset(){ nCyclesSinceLastTick = 0; }

protected:
	unsigned short nCyclesSinceLastTick;
	unsigned short timerPeriod;
	unsigned short timerCounter;
	
	bool timerEnable;

	virtual void rollOver(){ }
};

class SystemTimer : public SystemComponent, public ComponentTimer {
public:
	SystemTimer();

	// The system timer has no associated RAM, so return false.
	virtual bool preWriteAction(){ return false; }
	
	// The system timer has no associated RAM, so return false.
	virtual bool preReadAction(){ return false; }
	
	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	virtual bool readRegister(const unsigned short &reg, unsigned char &val);
	
	virtual bool onClockUpdate();
	
	virtual void defineRegisters();
	
private:
	unsigned short nDividerCycles;

	unsigned char dividerRegister;
	unsigned char timerModulo;
	unsigned char clockSelect;

	virtual void rollOver();
};

#endif
