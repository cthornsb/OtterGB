#ifndef SYSTEM_TIMER_HPP
#define SYSTEM_TIMER_HPP

#include "SystemComponent.hpp"
#include "ComponentTimer.hpp"

class SystemTimer : public SystemComponent, public ComponentTimer {
public:
	SystemTimer();

	// The system timer has no associated RAM, so return false.
	virtual bool preWriteAction(){ 
		return false; 
	}
	
	// The system timer has no associated RAM, so return false.
	virtual bool preReadAction(){ 
		return false; 
	}
	
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

