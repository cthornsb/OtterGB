#ifndef SERIAL_CONTROLLER_HPP
#define SERIAL_CONTROLLER_HPP

#include "SystemComponent.hpp"

class Register;

class SerialController : public SystemComponent {
public:
	SerialController() : SystemComponent("Serial") { }

	virtual void defineRegisters();

private:
};

#endif
