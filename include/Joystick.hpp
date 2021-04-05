#ifndef JOYSTICK_HPP
#define JOYSTICK_HPP

#include "OTTJoypad.hpp"

#include "SystemComponent.hpp"

class OTTWindow;
class ConfigFile;

class JoystickController : public SystemComponent {
public:
	JoystickController();
	
	void setWindow(OTTWindow *win){ window = win; }
	
	void setButtonMap(ConfigFile* config=0x0);

	void clearInput();
	
	// The joystick controller has no associated RAM, so return false to avoid trying to access it.
	bool preWriteAction() override { 
		return false; 
	}
	
	// The joystick controller has no associated RAM, so return false to avoid trying to access it.
	bool preReadAction() override { 
		return false; 
	}

	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;
	
	bool readRegister(const unsigned short &reg, unsigned char &val) override ;

	bool onClockUpdate() override ;

	void defineRegisters() override ;

private:
	bool selectButtonKeys; ///< Set when buttons are selected
	
	bool selectDirectionKeys; ///< Set when directions are selected
	
	bool P13; ///< Down or Start
	
	bool P12; ///< Up or Select
	
	bool P11; ///< Left or B
	
	bool P10; ///< Right or A
	
	OTTWindow *window; ///< Pointer to the main LCD driver

	unsigned char keyMapArray[8]; ///< Array which maps the 8 DMG buttons to keyboard keys
	
	GamepadInput gamepadMapArray[8]; ///< Array which maps the 8 DMG buttons to a 360-style controller
};

#endif
