
#include "SerialController.hpp"
#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"

void SerialController::defineRegisters(){
	sys->addSystemRegister(this, 0x01, rSB, "SB", "33333333");
	sys->addSystemRegister(this, 0x02, rSC, "SC", "33000003");
}
