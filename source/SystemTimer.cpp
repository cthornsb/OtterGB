#include "SystemTimer.hpp"
#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"

SystemTimer::SystemTimer() : 
	SystemComponent("Timer", 0x524d4954), // "TIMR"
	ComponentTimer(), 
	nDividerCycles(0),
	dividerRegister(0),
	timerModulo(0),
	clockSelect(0)
{
}

bool SystemTimer::writeRegister(const unsigned short &reg, const unsigned char &val){
	switch(reg){
		case 0xFF04: // DIV (Divider register)
			// Register incremented at 16384 Hz (256 cycles) or 32768 Hz (128 cycles) in 
			// GBC double speed mode. Writing any value resets to zero.
			rDIV->setValue(0x0);
			break;
		case 0xFF05: // TIMA (Timer counter)
			// Timer incremented by clock frequency specified by TAC. When
			// value overflows, it is set to TMA and a timer interrupt is
			// issued.
			break;
		case 0xFF06: // TMA (Timer modulo)
			break;
		case 0xFF07: // TAC (Timer control)
			bEnabled = rTAC->getBit(2); // Timer stop [0: Stop, 1: Start]
			clockSelect = rTAC->getBits(0,1); // Input clock select (see below)
			/** Input clocks:
				0:   4096 Hz (256 cycles)
				1: 262144 Hz (4 cycles)
				2:  65536 Hz (16 cycles)
				3:  16384 Hz (64 cycles) **/
			switch(clockSelect){
				case 0: // 256 cycles
					nPeriod = 256;
					break;
				case 1: // 4 cycles
					nPeriod = 4;
					break;
				case 2: // 16 cycles
					nPeriod = 16;
					break;
				case 3: // 64 cycles
					nPeriod = 64;
					break;
				default:
					break;
			}
			break;
		default:
			return false;
	}
	return true;
}

bool SystemTimer::readRegister(const unsigned short &reg, unsigned char &dest){
	switch(reg){
		case 0xFF04: // DIV (Divider register)
			break;
		case 0xFF05: // TIMA (Timer counter)
			break;
		case 0xFF06: // TMA (Timer modulo)
			break;
		case 0xFF07: // TAC (Timer control)
			break;
		default:
			return false;
	}
	return true;
}

bool SystemTimer::onClockUpdate(){
	if(!bEnabled) return false;
	if((nDividerCycles++) >= 64){ // DIV (divider register) incremented at a rate of 16384 Hz
		nDividerCycles = 0;
		(*rDIV)++;
	}
	nCyclesSinceLastTick++;
	while(nCyclesSinceLastTick / nPeriod){ // Handle a timer tick.
		if(++(*rTIMA) == 0x0) // Timer counter has rolled over
			rollover();
		// Reset the cycle counter.
		nCyclesSinceLastTick -= nPeriod;
	}
	return true;
}

void SystemTimer::rollover(){
	rTIMA->setValue(rTMA->getValue());
	sys->handleTimerInterrupt();
}

void SystemTimer::defineRegisters(){
	sys->addSystemRegister(this, 0x04, rDIV , "DIV" , "33333333");
	sys->addSystemRegister(this, 0x05, rTIMA, "TIMA", "33333333");
	sys->addSystemRegister(this, 0x06, rTMA , "TMA" , "33333333");
	sys->addSystemRegister(this, 0x07, rTAC , "TAC" , "33300000");
}

void SystemTimer::userAddSavestateValues(){
	unsigned int sizeUShort = sizeof(unsigned short);
	unsigned int sizeUChar = sizeof(unsigned char);
	/// ComponentTimer fields
	addSavestateValue(&nCyclesSinceLastTick, sizeUShort);
	addSavestateValue(&nPeriod,  sizeUShort);
	addSavestateValue(&nCounter, sizeUShort);
	addSavestateValue(&bEnabled, sizeof(bool));
	// SystemTimer fields
	addSavestateValue(&nDividerCycles,  sizeUShort);
	addSavestateValue(&dividerRegister, sizeUChar);
	addSavestateValue(&timerModulo,     sizeUChar);
	addSavestateValue(&clockSelect,     sizeUChar);
}

void SystemTimer::onUserReset(){
	nDividerCycles = 0;
	dividerRegister = 0;
	timerModulo = 0;
	clockSelect = 0;
	reload();
}

