#ifndef SERIAL_CONTROLLER_HPP
#define SERIAL_CONTROLLER_HPP

#include "SystemComponent.hpp"

class Register;

class SerialController : public SystemComponent {
public:
	SerialController() : 
		SystemComponent("Serial", 0x4c524553) // "SERL"
	{ 
	}

	void defineRegisters() override ;

private:
};

#endif
