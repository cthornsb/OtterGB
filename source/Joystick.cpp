#include <iostream>

#include "Support.hpp"
#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Joystick.hpp"
#include "Graphics.hpp"

/////////////////////////////////////////////////////////////////////
// class JoystickController
/////////////////////////////////////////////////////////////////////

void JoystickController::handleButtonPress(const BUTTON &button){
	// P13 - Down or Start
	// P12 - Up or Select
	// P11 - Left or B
	// P10 - Right or A
	if(selectButtonKeys){
		switch(button){
			case START: // P13 - bit3
				(*rJOYP) &= 0xF7;
				break;
			case SELECT: // P12 - bit2
				(*rJOYP) &= 0xFB;
				break;
			case B: // P11 - bit1
				(*rJOYP) &= 0xFD;
				break;		
			case A: // P10 - bit0
				(*rJOYP) &= 0xFE;
				break;		
			default:
				break;
		}
	}
	if(selectDirectionKeys){
		switch(button){
			case DOWN: // P13 - bit3
				(*rJOYP) &= 0xF7;
				break;		
			case UP: // P12 - bit2
				(*rJOYP) &= 0xFB;
				break;		
			case LEFT: // P11 - bit1
				(*rJOYP) &= 0xFD;
				break;
			case RIGHT: // P10 - bit0
				(*rJOYP) &= 0xFE;
				break;	
			default:
				break;
		}
	}
}

void JoystickController::clearInput(){
	(*rJOYP) |= 0x0F;
}

bool JoystickController::writeRegister(const unsigned short &reg, const unsigned char &val){
	if(reg == 0xFF00){ // The joystick controller has only one register
		(*rJOYP) &= 0xCF; // Zero bits 4 and 5
		(*rJOYP) |= (val & 0x30); // Only bits 4,5 are writable 
		selectButtonKeys    = !((writeVal & 0x20) == 0x20); // P15 [0: Select, 1: No action]
		selectDirectionKeys = !((writeVal & 0x10) == 0x10); // P14 [0: Select, 1: No action]
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

	// P13 - Down or Start
	// P12 - Up or Select
	// P11 - Left or B
	// P10 - Right or A
	// [0: Pressed, 1: Not pressed]

	// Poll the screen controller to check for button presses.
	KeypressEvent *keypress = window->getKeypress();
	if(!keypress->down){ // Button is released
		clearInput();
		return false; 
	}
	
	// Check which key is down.
	switch(keypress->key){
		case 0x0D: // Enter
			handleButtonPress(START);
			break;
		case 0x09: // Tab
			handleButtonPress(SELECT);
			break;
		case 0x6A: // J
			handleButtonPress(B);
			break;
		case 0x6B: // K
			handleButtonPress(A);
			break;
		case 0x77: // W
			handleButtonPress(UP);
			break;
		case 0x61: // A
			handleButtonPress(LEFT);
			break;
		case 0x73: // S
			handleButtonPress(DOWN);
			break;
		case 0x64: // D
			handleButtonPress(RIGHT);
			break;
		case 0x52: // up
			handleButtonPress(UP);
			break;
		case 0x50: // left
			handleButtonPress(LEFT);
			break;
		case 0x51: // down
			handleButtonPress(DOWN);
			break;
		case 0x4F: // right
			handleButtonPress(RIGHT);
			break;
		default:
			break;
	}
	
	if((*rJOYP & 0x0F) != 0) // Request a joypad interrupt
		sys->handleJoypadInterrupt();
	
	return true;
}
