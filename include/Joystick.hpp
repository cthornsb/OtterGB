#ifndef JOYSTICK_HPP
#define JOYSTICK_HPP

#include "SystemComponent.hpp"

class Window;

class JoystickController : public SystemComponent {
public:
	JoystickController() : SystemComponent(), window(0x0) { }
	
	void setWindow(Window *win){ window = win; }
	
	void clearInput();
	
	// The joystick controller has no associated RAM, so return false to avoid trying to access it.
	virtual bool preWriteAction(){ return false; }
	
	// The joystick controller has no associated RAM, so return false to avoid trying to access it.
	virtual bool preReadAction(){ return false; }

	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	virtual bool readRegister(const unsigned short &reg, unsigned char &val);

	virtual bool onClockUpdate(const unsigned short &nCycles);

private:
	bool selectButtonKeys;
	bool selectDirectionKeys;
	
	bool P13; // Down or Start
	bool P12; // Up or Select
	bool P11; // Left or B
	bool P10; // Right or A
	
	Window *window; ///< Pointer to the main LCD driver
};

#endif
