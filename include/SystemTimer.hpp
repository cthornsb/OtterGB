#ifndef SYSTEM_TIMER_HPP
#define SYSTEM_TIMER_HPP

#include "SystemComponent.hpp"
#include "ComponentTimer.hpp"

class SystemTimer : public SystemComponent, public ComponentTimer {
public:
	SystemTimer();

	// The system timer has no associated RAM, so return false.
	bool preWriteAction() override { 
		return false; 
	}
	
	// The system timer has no associated RAM, so return false.
	bool preReadAction() override { 
		return false; 
	}
	
	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;
	
	bool readRegister(const unsigned short &reg, unsigned char &val) override ;
	
	bool onClockUpdate() override ;
	
	void defineRegisters() override ;
	
private:
	unsigned short nDividerCycles;

	unsigned char dividerRegister;
	
	unsigned char timerModulo;
	
	unsigned char clockSelect;

	void rollover() override ;
	
	/** Add elements to a list of values which will be written to / read from an emulator savestate
	  */
	void userAddSavestateValues() override;
	
	/** Reset system timer
	  */
	void onUserReset() override;
};

#endif

