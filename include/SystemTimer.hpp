#ifndef SYSTEM_TIMER_HPP
#define SYSTEM_TIMER_HPP

#include <chrono>

#include "SystemComponent.hpp"

// Make a typedef for clarity when working with chrono.
typedef std::chrono::system_clock sclock;

class SystemClock : public SystemComponent {
public:
	SystemClock();
	
	void setFrequencyMultiplier(const double &mult){ frequencyMultiplier = mult; }
	
	void setDisplayFramerate(bool state=true){ displayFramerate = state; }
	
	bool sync(const unsigned short &nCycles=1);

	bool wait(const double &microseconds);

private:
	bool displayFramerate;

	double frequencyMultiplier;
	
	unsigned int cyclesSinceLastVSync; ///< The number of cycles since the last Vertical Sync
	unsigned int cyclesSinceLastHSync; ///< The number of cycles since the last  Horizontal Sync

	unsigned char lcdDriverMode;

	sclock::time_point timeOfInitialization; ///< The time that the system clock was initialized
	sclock::time_point timeOfLastVSync; ///< The time at which the screen was last refreshed

	void mode0Interrupt();

	void mode1Interrupt();

	void mode2Interrupt();
};

class ComponentTimer {
public:
	ComponentTimer() : nCyclesSinceLastTick(0), timerPeriod(1), timerCounter(0), timerEnable(true) { }
	
	ComponentTimer(const unsigned short &period) : nCyclesSinceLastTick(0), timerPeriod(period), timerCounter(0), timerEnable(true) { }
	
	void enable(){ timerEnable = true; }
	
	void disable(){ timerEnable = false; }	

	void setTimerPeriod(const unsigned short &period){ timerPeriod = period; }
	
	unsigned short getTimerPeriod() const { return timerPeriod; }
	
	unsigned short getTimerCounter() const { return timerCounter; }

	void reset(){ nCyclesSinceLastTick = 0; }
	
	virtual bool onClockUpdate(const unsigned short &nCycles);

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
	
	virtual bool onClockUpdate(const unsigned short &nCycles);
	
private:
	unsigned short nDividerCycles;

	unsigned char dividerRegister;
	unsigned char timerModulo;
	unsigned char clockSelect;
	
	virtual void rollOver();
};

#endif
