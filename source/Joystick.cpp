#include "OTTWindow.hpp"
#include "OTTKeyboard.hpp"

#include "Support.hpp"
#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Joystick.hpp"
#include "ConfigFile.hpp"

// Default button map
constexpr unsigned char KEYBOARD_ENTER = 0x0D;
constexpr unsigned char KEYBOARD_TAB   = 0x09;
constexpr unsigned char KEYBOARD_J     = 0x6A;
constexpr unsigned char KEYBOARD_K     = 0x6B;
constexpr unsigned char KEYBOARD_W     = 0x77;
constexpr unsigned char KEYBOARD_A     = 0x61;
constexpr unsigned char KEYBOARD_S     = 0x73;
constexpr unsigned char KEYBOARD_D     = 0x64;
constexpr unsigned char KEYBOARD_DOWN  = 0x51;
constexpr unsigned char KEYBOARD_UP    = 0x52;
constexpr unsigned char KEYBOARD_LEFT  = 0x50;
constexpr unsigned char KEYBOARD_RIGHT = 0x4F;

constexpr unsigned char JOYPAD_P13_MASK = 0xF7; // Bit 3
constexpr unsigned char JOYPAD_P12_MASK = 0xFB; // Bit 2
constexpr unsigned char JOYPAD_P11_MASK = 0xFD; // Bit 1
constexpr unsigned char JOYPAD_P10_MASK = 0xFE; // Bit 0

void getKeyFromString(ConfigFile* config, unsigned char& key) {
	std::string str = toLowercase(config->getCurrentParameterString());
	if (str == "backspace")
		key = 0x8;
	else if (str == "tab")
		key = 0x9;
	else if (str == "enter")
		key = 0xD;
	else if (str == "escape") // Avoid using this because it's reserved by the graphics handler
		key = 0x1B;
	else if (str == "space")
		key = 0x20;
	else if (str == "delete")
		key = 0x7F;
}

/////////////////////////////////////////////////////////////////////
// class JoystickController
/////////////////////////////////////////////////////////////////////

JoystickController::JoystickController() :
	SystemComponent("Joypad", 0x50594f4a), // "JOYP"
	selectButtonKeys(false),
	selectDirectionKeys(false),
	P13(false),
	P12(false),
	P11(false),
	P10(false),
	window(0x0) 
{ 
	setButtonMap(); // Set default key mapping
}

void JoystickController::setButtonMap(ConfigFile* config/*=0x0*/) {
	if (config) { // Read the configuration file
		if (config->search("KEY_START", true))
			getKeyFromString(config, keyMapArray[0]);
		if (config->search("KEY_SELECT", true))
			getKeyFromString(config, keyMapArray[1]);
		if (config->search("KEY_B", true))
			getKeyFromString(config, keyMapArray[2]);
		if (config->search("KEY_A", true))
			getKeyFromString(config, keyMapArray[3]);
		if (config->search("KEY_DOWN", true))
			getKeyFromString(config, keyMapArray[4]);
		if (config->search("KEY_UP", true))
			getKeyFromString(config, keyMapArray[5]);
		if (config->search("KEY_LEFT", true))
			getKeyFromString(config, keyMapArray[6]);
		if (config->search("KEY_RIGHT", true))
			getKeyFromString(config, keyMapArray[7]);
	}
	else { // Set default keys
		keyMapArray[0] = KEYBOARD_ENTER; // Start
		keyMapArray[1] = KEYBOARD_TAB;   // Select
		keyMapArray[2] = KEYBOARD_J;     // B
		keyMapArray[3] = KEYBOARD_K;     // A
		keyMapArray[4] = KEYBOARD_S;     // Down
		keyMapArray[5] = KEYBOARD_W;     // Up
		keyMapArray[6] = KEYBOARD_A;     // Left
		keyMapArray[7] = KEYBOARD_D;     // Right
	}
}

void JoystickController::clearInput(){
	(*rJOYP) |= 0x0F;
}

bool JoystickController::writeRegister(const unsigned short &reg, const unsigned char &val){
	if(reg == 0xFF00){ // The joystick controller has only one register
		rJOYP->setValue(0xF); // Zero bits 4 and 5 and clear input lines
		(*rJOYP) |= (val & 0x30); // Only bits 4,5 are writable 
		selectButtonKeys    = !rJOYP->getBit(5); // P15 [0: Select, 1: No action]
		selectDirectionKeys = !rJOYP->getBit(4); // P14 [0: Select, 1: No action]
		return true;
	}
	return false;
}

bool JoystickController::readRegister(const unsigned short &reg, unsigned char &dest){
	if(reg == 0xFF00) // The joystick controller has only one register
		return true;
	return false;
}

bool JoystickController::onClockUpdate(){
	if(!window) return false;

	if(!selectButtonKeys && !selectDirectionKeys)
		return false;

	// Poll the screen controller to check for button presses.
	OTTKeyboard *keys = window->getKeypress();
	if(keys->empty()){ // No buttons pressed.
		clearInput();
		return false; 
	}
	
	// Get the initial state of the input lines.
	unsigned char initialState = rJOYP->getValue() & 0xF;
	
	// Check which key is down.
	// P13 - Down or Start
	// P12 - Up or Select
	// P11 - Left or B
	// P10 - Right or A
	if(selectButtonKeys){
		if(keys->check(keyMapArray[0])) //  START - Enter (P13 - bit3)
			(*rJOYP) &= JOYPAD_P13_MASK;
		if(keys->check(keyMapArray[1]))   // SELECT - Tab   (P12 - bit2)
			(*rJOYP) &= JOYPAD_P12_MASK;
		if(keys->check(keyMapArray[2]))     //      B - j     (P11 - bit1)
			(*rJOYP) &= JOYPAD_P11_MASK;
		if(keys->check(keyMapArray[3]))     //      A - k     (P10 - bit0)
			(*rJOYP) &= JOYPAD_P10_MASK;
	}
	else if(selectDirectionKeys){
		if(keys->check(keyMapArray[4]) || keys->check(KEYBOARD_DOWN))  // P13 - s (down)  (P13 - bit3)
			(*rJOYP) &= JOYPAD_P13_MASK;
		if(keys->check(keyMapArray[5]) || keys->check(KEYBOARD_UP))    // P12 - w (up)    (P12 - bit2)
			(*rJOYP) &= JOYPAD_P12_MASK;
		if(keys->check(keyMapArray[6]) || keys->check(KEYBOARD_LEFT))  // P11 - a (left)  (P11 - bit1)
			(*rJOYP) &= JOYPAD_P11_MASK;
		if(keys->check(keyMapArray[7]) || keys->check(KEYBOARD_RIGHT)) // P10 - d (right) (P10 - bit0)
			(*rJOYP) &= JOYPAD_P10_MASK;
	}
	
	// Detect when one or more input lines go low
	if(rJOYP->getBits(0,3) != initialState){
		// Request a joypad interrupt
		sys->handleJoypadInterrupt();
	}
	
	return true;
}

void JoystickController::defineRegisters(){
	sys->addSystemRegister(this, 0x00, rJOYP, "JOYP", "33333333");
}
