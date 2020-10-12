
#include <iostream>
#include <unistd.h>

#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Support.hpp"
#include "SystemTimer.hpp"

#define SYSTEM_CLOCK_FREQUENCY 4194304 // Hz

#define MODE2_START 0 // STAT mode 2 from 0 to 79 (80 cycles)
#define MODE3_START 80 // STAT mode 3 from 80 to 251 (172 cycles)
#define MODE0_START 252 // STAT mode 0 from 252 to 456 (204 cycles)
#define MODE1_START 65664 // STAT mode 1 from 65664 to 70224 (4560 cycles)

#define VERTICAL_SYNC_CYCLES   70224   // CPU cycles per VSYNC (~59.73 Hz)
#define HORIZONTAL_SYNC_CYCLES 456     // CPU cycles per HSYNC (per 154 scanlines)

SystemClock::SystemClock() : SystemComponent(), frequencyMultiplier(1.0), cyclesSinceLastVSync(0), lcdDriverMode(2) {
	timeOfInitialization = sclock::now();
	timeOfLastVSync = sclock::now();
}

// Wait a specified number of clock cycles
bool SystemClock::sync(const unsigned short &nCycles/*=1*/){
	const unsigned int lcdDriverModeBoundaries[4] = {HORIZONTAL_SYNC_CYCLES, VERTICAL_SYNC_CYCLES, MODE3_START, MODE0_START};

	bool vsync = false;
	bool hsync = false;
	cyclesSinceLastVSync += nCycles;
	cyclesSinceLastHSync += nCycles;

	// Check if we're on the next scanline (every 456 cycles)
	// Set the LCD STAT register mode flag:
	//  Complete screen refresh takes 70224 cycles
	//  Each scanline takes 456 cycles (154 total scanlines, 0-153)
	//  10 scanlines of overscan takes 4560 cycles (VBlank) (lines 144-153)
	// NOTE: The maximum number of cycles for a LR35902 is 24, therefore it
	//  is impossible for a single instruction to cross a STAT mode boundary by
	//  itself. It's also impossible to skip a scanline with only one instruction.
	//  The mode progression is 2->3->0->2->3->0 ... ->2->3->0->1->2 ...	
	if((*rLY) != (cyclesSinceLastVSync / HORIZONTAL_SYNC_CYCLES)){
		(*rLY) = (cyclesSinceLastVSync / HORIZONTAL_SYNC_CYCLES);
		hsync = true;
	}
		
	// Check if the display is enabled. If it's not, set STAT to mode 1
	if(((*rLCDC) & 0x80) == 0){
		if(lcdDriverMode != 1){
			lcdDriverMode = 1;
			(*rSTAT) = ((*rSTAT) & 0xFC) | 0x1; // Mode 1
		}
	}	
	else if(cyclesSinceLastVSync < VERTICAL_SYNC_CYCLES){
		if(cyclesSinceLastHSync >= lcdDriverModeBoundaries[lcdDriverMode]){ // Handle LCD STAT register mode switch
			//std::cout << " " << (int)(*rLY) << "\t" << cyclesSinceLastHSync << "\t" << getHex(lcdDriverMode) << "->";
			/*0 to 80 : mode2
			80 to 252 : mode3
			252 to 456 : mode0
			...
			65664 to 70224 : mode1*/
			if(lcdDriverMode == 0){ // Mode: 0->2(1)
				if(cyclesSinceLastVSync < MODE1_START){ // Mode: 0->2
					lcdDriverMode = 2;
					(*rSTAT) = ((*rSTAT) & 0xFC) | 0x2; // Mode 2 - Reading from OAM (mode 2)
					mode2Interrupt();
				}
				else{ // Mode: 0->1 (Vertical blanking (VBlank) interval)
					lcdDriverMode = 1;
					(*rSTAT) = ((*rSTAT) & 0xFC) | 0x1; // Mode 1 - Reading from OAM (mode 2)
					mode1Interrupt();
					vsync = true;
				}
			}
			else if(lcdDriverMode == 1){ // Mode: 1->2
				lcdDriverMode = 2;
				(*rSTAT) = ((*rSTAT) & 0xFC) | 0x2; // Mode 2 - Reading from OAM (mode 2)
				mode2Interrupt();
			}
			else if(lcdDriverMode == 2){ // Mode: 2->3
				lcdDriverMode = 3;
				(*rSTAT) = ((*rSTAT) & 0xFC) | 0x3; // Mode 3 - Reading from OAM/VRAM (mode 3)
			}
			else if(lcdDriverMode == 3){ // Mode: 3->0 (Horizontal blanking (HBlank) interval)
				lcdDriverMode = 0;
				(*rSTAT) = ((*rSTAT) & 0xFC); // Mode 0 - HBlank period (mode 0)
				mode0Interrupt();
			}
			//std::cout << getHex(lcdDriverMode) << std::endl;
		}	
	}
	
	if(hsync) // Draw the scanline
		sys->handleHBlankPeriod(); 
	
	if(cyclesSinceLastVSync >= VERTICAL_SYNC_CYCLES){ // In the next refresh period.
		cyclesSinceLastVSync = (cyclesSinceLastVSync % VERTICAL_SYNC_CYCLES);
		static unsigned int frameCount = 0;
		static double totalRenderTime = 0;
		const double framePeriod = VERTICAL_SYNC_CYCLES/(SYSTEM_CLOCK_FREQUENCY*frequencyMultiplier); // in seconds
		double wallTime = std::chrono::duration_cast<std::chrono::duration<double>>(sclock::now() - timeOfLastVSync).count();
		double timeToSleep = (framePeriod - wallTime)*1E6; // microseconds
		if(timeToSleep > 0)
			usleep((int)timeToSleep);
		totalRenderTime += std::chrono::duration_cast<std::chrono::duration<double>>(sclock::now() - timeOfLastVSync).count();;
		if((++frameCount % 60) == 0){
			std::cout << " fps=" << frameCount/totalRenderTime << "     \r" << std::flush;
			totalRenderTime = 0;
			frameCount = 0;
		}
		timeOfLastVSync = sclock::now();
	}

	return vsync;
}

bool SystemClock::wait(const double &microseconds){
	unsigned short nCycles = (unsigned short)(SYSTEM_CLOCK_FREQUENCY * (microseconds*1E-6));
	return sync(nCycles);
}

void SystemClock::mode0Interrupt(){
	if((*rSTAT) & 0x8 != 0) // Request LCD STAT interrupt (INT 48)
		sys->handleLcdInterrupt();
}

void SystemClock::mode1Interrupt(){
	if((*rSTAT) & 0x10 != 0){ // Request VBlank and LCD STAT interrupt (INT 48)
		sys->handleVBlankInterrupt();
		sys->handleLcdInterrupt();
	}
}

void SystemClock::mode2Interrupt(){
	// Reset the HBlank cycle counter
	cyclesSinceLastHSync = (cyclesSinceLastHSync % HORIZONTAL_SYNC_CYCLES);
	if((*rSTAT) & 0x20 != 0) // Request LCD STAT interrupt (INT 48)
		sys->handleLcdInterrupt();
}

/////////////////////////////////////////////////////////////////////
// class ComponentTimer
/////////////////////////////////////////////////////////////////////

bool ComponentTimer::onClockTick(const unsigned short &ticks){
	nCycles += ticks;
	if(nCycles >= timerPeriod){
		nCycles = nCycles % timerPeriod;
		timerCounter++;
		rollOver();
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////
// class SystemTimer
/////////////////////////////////////////////////////////////////////

SystemTimer::SystemTimer() : SystemComponent(), ComponentTimer(), nDividerCycles(0) {
}

bool SystemTimer::writeRegister(const unsigned short &reg, const unsigned char &val){
	switch(reg){
		case 0xFF04: // DIV (Divider register)
			// Register incremented at 16384 Hz (256 cycles) or 32768 Hz (128 cycles) in 
			// GBC double speed mode. Writing any value resets to zero.
			(*rDIV) = 0x0;
			break;
		case 0xFF05: // TIMA (Timer counter)
			// Timer incremented by clock frequency specified by TAC. When
			// value overflows, it is set to TMA and a timer interrupt is
			// issued.
			(*rTIMA) = val;
			break;
		case 0xFF06: // TMA (Timer modulo)
			(*rTMA) = val; // Value loaded when TIMA overflows
			break;
		case 0xFF07: // TAC (Timer control)
			(*rTAC) = val;
			timerEnable = (val & 0x4) != 0; // Timer stop [0: Stop, 1: Start]
			clockSelect = (val & 0x3); // Input clock select (see below)
			/** Input clocks:
				0:   4096 Hz (1024 cycles)
				1: 262144 Hz (16 cycles)
				2:  65536 Hz (64 cycles)
				3:  16384 Hz (256 cycles) **/
			switch(clockSelect){
				case 0: // 1024 cycles
					timerPeriod = 1024;
					break;
				case 1: // 16 cycles
					timerPeriod = 16;
					break;
				case 2: // 64 cycles
					timerPeriod = 64;
					break;
				case 3: // 256 cycles
					timerPeriod = 256;
					break;
				default:
					break;
			}
			break;
		default:
			return false;
	}
	return true;
}

bool SystemTimer::readRegister(const unsigned short &reg, unsigned char &dest){
	switch(reg){
		case 0xFF04: // DIV (Divider register)
			dest = (*rDIV);
			break;
		case 0xFF05: // TIMA (Timer counter)
			dest = (*rTIMA);
			break;
		case 0xFF06: // TMA (Timer modulo)
			dest = (*rTMA);
			break;
		case 0xFF07: // TAC (Timer control)
			dest = (*rTAC);
			break;
		default:
			return false;
	}
	return true;
}

bool SystemTimer::onClockTick(const unsigned short &ticks){
	if(!timerEnable) return false;
	if((nDividerCycles += ticks) >= 256){ // DIV (divider register) incremented at a rate of 16384 Hz
		nDividerCycles = nDividerCycles % 256;
		(*rDIV)++;
	}
	if((nCycles += ticks) >= timerPeriod){
		// Handle timerPeriod==16 here!!!
		rollOver();
		return true;
	}
	return false;
}

void SystemTimer::rollOver(){
	nCycles = nCycles % timerPeriod;
	(*rTIMA) = (*rTMA);
	sys->handleTimerInterrupt();
}
