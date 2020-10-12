#ifndef JOYSTICK_HPP
#define JOYSTICK_HPP

#include "SystemComponent.hpp"

class JoystickController : public SystemComponent {
public:
	JoystickController() : SystemComponent(128) { }
	
	void handleButtonPress(const unsigned char &button);
	
	void clearInput();
	
	// The joystick controller has no associated RAM, so return false to avoid trying to access it.
	virtual bool preWriteAction(){ return false; }
	
	// The joystick controller has no associated RAM, so return false to avoid trying to access it.
	virtual bool preReadAction(){ return false; }

	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	virtual bool readRegister(const unsigned short &reg, unsigned char &val);

private:
	bool selectButtonKeys;
	bool selectDirectionKeys;
	
	bool P13; // Down or Start
	bool P12; // Up or Select
	bool P11; // Left or B
	bool P10; // Right or A
};

#endif
