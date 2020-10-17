#include <iostream>

#include "Support.hpp"
#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Joystick.hpp"
#include "Graphics.hpp"

/////////////////////////////////////////////////////////////////////
// class JoystickController
/////////////////////////////////////////////////////////////////////

void JoystickController::clearInput(){
	(*rJOYP) |= 0x0F;
}

bool JoystickController::writeRegister(const unsigned short &reg, const unsigned char &val){
	if(reg == 0xFF00){ // The joystick controller has only one register
		(*rJOYP) = 0xF; // Zero bits 4 and 5
		(*rJOYP) |= (val & 0x30); // Only bits 4,5 are writable 
		selectButtonKeys    = ((*rJOYP & 0x20) == 0); // P15 [0: Select, 1: No action]
		selectDirectionKeys = ((*rJOYP & 0x10) == 0); // P14 [0: Select, 1: No action]
		return true;
	}
	return false;
}

bool JoystickController::readRegister(const unsigned short &reg, unsigned char &dest){
	if(reg == 0xFF00){ // The joystick controller has only one register
		dest = (*rJOYP);
		return true;
	}	
	return false;
}

bool JoystickController::onClockUpdate(const unsigned short &nCycles){
	if(!window) return false;

	if(!selectButtonKeys && !selectDirectionKeys)
		return false;

	// Poll the screen controller to check for button presses.
	KeypressEvent *keypress = window->getKeypress();
	if(!keypress->down){ // Button is released
		clearInput();
		return false; 
	}
	
	unsigned char key = keypress->key;

	// Check which key is down.
	// P13 - Down or Start
	// P12 - Up or Select
	// P11 - Left or B
	// P10 - Right or A
	bool valid = true;
	if(selectButtonKeys){
		if(key == 0x0D)       //  START - Enter (P13 - bit3)
			(*rJOYP) &= 0xF7;
		else if(key == 0x09)  // SELECT - Tab   (P12 - bit2)
			(*rJOYP) &= 0xFB;
		else if(key == 0x6A)  //      B - j     (P11 - bit1)
			(*rJOYP) &= 0xFD;
		else if(key == 0x6B)  //      A - k     (P10 - bit0)
			(*rJOYP) &= 0xFE;
		else
			valid = false;
	}
	else if(selectDirectionKeys){
		if(key == 0x73 || key == 0x51)      // P13 - s (down)  (P13 - bit3)
			(*rJOYP) &= 0xF7;
		else if(key == 0x77 || key == 0x52) // P12 - w (up)    (P12 - bit2)
			(*rJOYP) &= 0xFB;
		else if(key == 0x61 || key == 0x50) // P11 - a (left)  (P11 - bit1)
			(*rJOYP) &= 0xFD;
		else if(key == 0x64 || key == 0x4F) // P10 - d (right) (P10 - bit0)
			(*rJOYP) &= 0xFE;
		else
			valid = false;
	}
	else{ valid = false; }

	if(valid && ((*rJOYP & 0x0F) != 0)) // Request a joypad interrupt
		sys->handleJoypadInterrupt();
	
	return true;
}
