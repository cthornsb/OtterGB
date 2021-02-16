#ifndef SYSTEM_CLOCK_HPP
#define SYSTEM_CLOCK_HPP

#include "SystemComponent.hpp"
#include "HighResTimer.hpp" // typedef hrclock

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
	bool onClockUpdate() override ;
	
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

	/** Add elements to a list of values which will be written to / read from an emulator savestate
	  */
	void userAddSavestateValues() override;
};

#endif

