#include <iostream>

#include "Support.hpp"
#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Joystick.hpp"
#include "Graphics.hpp"

// Button map
#define KEYBOARD_ENTER 0x0D
#define KEYBOARD_TAB   0x09
#define KEYBOARD_J     0x6A
#define KEYBOARD_K     0x6B
#define KEYBOARD_W     0x77
#define KEYBOARD_A     0x61
#define KEYBOARD_S     0x73
#define KEYBOARD_D     0x64
#define KEYBOARD_DOWN  0x51
#define KEYBOARD_UP    0x52
#define KEYBOARD_LEFT  0x50
#define KEYBOARD_RIGHT 0x4F

#define JOYPAD_P13_MASK 0xF7 // Bit 3
#define JOYPAD_P12_MASK 0xFB // Bit 2
#define JOYPAD_P11_MASK 0xFD // Bit 1
#define JOYPAD_P10_MASK 0xFE // Bit 0

/////////////////////////////////////////////////////////////////////
// class JoystickController
/////////////////////////////////////////////////////////////////////

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
	KeyStates *keys = window->getKeypress();
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
		if(keys->check(KEYBOARD_ENTER)) //  START - Enter (P13 - bit3)
			(*rJOYP) &= JOYPAD_P13_MASK;
		if(keys->check(KEYBOARD_TAB))   // SELECT - Tab   (P12 - bit2)
			(*rJOYP) &= JOYPAD_P12_MASK;
		if(keys->check(KEYBOARD_J))     //      B - j     (P11 - bit1)
			(*rJOYP) &= JOYPAD_P11_MASK;
		if(keys->check(KEYBOARD_K))     //      A - k     (P10 - bit0)
			(*rJOYP) &= JOYPAD_P10_MASK;
	}
	else if(selectDirectionKeys){
		if(keys->check(KEYBOARD_S) || keys->check(KEYBOARD_DOWN))  // P13 - s (down)  (P13 - bit3)
			(*rJOYP) &= JOYPAD_P13_MASK;
		if(keys->check(KEYBOARD_W) || keys->check(KEYBOARD_UP))    // P12 - w (up)    (P12 - bit2)
			(*rJOYP) &= JOYPAD_P12_MASK;
		if(keys->check(KEYBOARD_A) || keys->check(KEYBOARD_LEFT))  // P11 - a (left)  (P11 - bit1)
			(*rJOYP) &= JOYPAD_P11_MASK;
		if(keys->check(KEYBOARD_D) || keys->check(KEYBOARD_RIGHT)) // P10 - d (right) (P10 - bit0)
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
