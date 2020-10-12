#include <iostream>

#include "Support.hpp"
#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Joystick.hpp"

#define BUTTON_START  0
#define BUTTON_SELECT 1
#define BUTTON_B      2
#define BUTTON_A      3
#define BUTTON_DOWN   4
#define BUTTON_UP     5
#define BUTTON_LEFT   6
#define BUTTON_RIGHT  7

/////////////////////////////////////////////////////////////////////
// class JoystickController
/////////////////////////////////////////////////////////////////////

void JoystickController::handleButtonPress(const unsigned char &button){
	// P13 - Down or Start
	// P12 - Up or Select
	// P11 - Left or B
	// P10 - Right or A
	// [0: Pressed, 1: Not pressed]
	if(selectButtonKeys){
		switch(button){
			case BUTTON_START: // P13 - bit3
				(*rJOYP) &= 0xF7;
				break;
			case BUTTON_SELECT: // P12 - bit2
				(*rJOYP) &= 0xFB;
				break;
			case BUTTON_B: // P11 - bit1
				(*rJOYP) &= 0xFD;
				break;		
			case BUTTON_A: // P10 - bit0
				(*rJOYP) &= 0xFE;
				break;		
			default:
				break;
		}
	}
	else if(selectDirectionKeys){
		switch(button){
			case BUTTON_DOWN: // P13 - bit3
				(*rJOYP) &= 0xF7;
				break;		
			case BUTTON_UP: // P12 - bit2
				(*rJOYP) &= 0xFB;
				break;		
			case BUTTON_LEFT: // P11 - bit1
				(*rJOYP) &= 0xFD;
				break;
			case BUTTON_RIGHT: // P10 - bit0
				(*rJOYP) &= 0xFE;
				break;	
			default:
				break;
		}
	}
	sys->handleJoypadInterrupt();
}

void JoystickController::clearInput(){
	(*rJOYP) |= 0x0F;
}

bool JoystickController::writeRegister(const unsigned short &reg, const unsigned char &val){
	if(reg == 0xFF00){ // The joystick controller has only one register
		(*rJOYP) = (val & 0x30) | 0x0F; // Only bits 4,5 are writable 
		selectButtonKeys    = (writeVal & 0x20) != 0; // P15 [0: Select, 1: No action]
		selectDirectionKeys = (writeVal & 0x10) != 0; // P14 [0: Select, 1: No action]
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
