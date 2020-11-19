
#include <iostream>
#include <unistd.h>

#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Support.hpp"
#include "SystemTimer.hpp"

#define SYSTEM_CLOCK_FREQUENCY 1048576 // Hz

#define MODE2_START 0 // STAT mode 2 from 0 to 79 (80 cycles)
#define MODE3_START 20 // STAT mode 3 from 80 to 251 (172 cycles)
#define MODE0_START 63 // STAT mode 0 from 252 to 456 (204 cycles)
#define MODE1_START 16416 // STAT mode 1 from 65664 to 70224 (4560 cycles)

#define VERTICAL_SYNC_CYCLES   17556 // CPU cycles per VSYNC (~59.73 Hz)
#define HORIZONTAL_SYNC_CYCLES 114   // CPU cycles per HSYNC (per 154 scanlines)

SystemClock::SystemClock() : SystemComponent(), vsync(false), frequencyMultiplier(1.0), cyclesSinceLastVSync(0), lcdDriverMode(2), framerate(0) {
	timeOfInitialization = sclock::now();
	timeOfLastVSync = sclock::now();
}

// Tick the system clock.
bool SystemClock::onClockUpdate(){
	// Check if the display is enabled. If it's not, set STAT to mode 1
	if(!rLCDC->getBit(7)){
		if(lcdDriverMode != 1){
			lcdDriverMode = 1;
			rSTAT->setBits(0,1,1); // Mode 1
		}
		return false;
	}

	bool hsync = false;
	cyclesSinceLastVSync++;
	cyclesSinceLastHSync++;

	// Check if we're on the next scanline (every 456 cycles)
	// Set the LCD STAT register mode flag:
	//  Complete screen refresh takes 70224 cycles
	//  Each scanline takes 456 cycles (154 total scanlines, 0-153)
	//  10 scanlines of overscan takes 4560 cycles (VBlank) (lines 144-153)
	//  The mode progression is 2->3->0->2->3->0 ... ->2->3->0->1->2 ...	
	if(cyclesSinceLastVSync <= MODE1_START){ // Visible scanlines (0-143)
		// 0 to 20   : mode2 - OAM sprite search (20 cycles)
		// 20 to 63  : mode3 - Drawing pixels (43+X cycles)
		// 63 to 114 : mode0 - HBlank (51-X cycles)
		// ...
		// 16416 to 17556 : mode1 - VBlank
		if(cyclesSinceLastHSync == MODE3_START){ // Mode 2->3 (Drawing the scanline)
			lcdDriverMode = 3;
			rSTAT->setBits(0,1); // Mode 3
			sys->handleHBlankPeriod(); // Start drawing the scanline
		}		
		else if(cyclesSinceLastHSync == MODE0_START){ // Mode 3->0 (HBlank)
			lcdDriverMode = 0;
			rSTAT->resetBits(0,1); // Mode 0
			mode0Interrupt();
		}		
		else if(cyclesSinceLastHSync == HORIZONTAL_SYNC_CYCLES){ // Mode 0->2(1)
			incrementScanline(); // Increment the scanline
			if(cyclesSinceLastVSync != MODE1_START){ // Mode 0->2 (Reading from OAM)
				lcdDriverMode = 2;
				rSTAT->setBits(0,1,2); // Mode 2
				mode2Interrupt();
			}
			else{ // Mode 0->1 (VBlank, overscan)
				lcdDriverMode = 1;
				rSTAT->setBits(0,1,1); // Mode 1
				mode1Interrupt();
				vsync = true;
			}
		}
	}
	else if(cyclesSinceLastVSync <= VERTICAL_SYNC_CYCLES){ // Mode 1 - Overscan (144-153)
		if(cyclesSinceLastHSync == HORIZONTAL_SYNC_CYCLES){
			incrementScanline();
		}
	}
	else{ // Start the next frame (Mode 1->2)
		rSTAT->setBits(0,1,2); // Mode 2
		waitUntilNextVSync();	
		resetScanline(); // VBlank period has ended, next frame started
		mode2Interrupt();
	}
	
	return vsync;
}

void SystemClock::wait(){
	waitUntilNextVSync();
}

void SystemClock::resetScanline(){
	vsync = false;
	//cyclesSinceLastVSync -= (*rLY) * HORIZONTAL_SYNC_CYCLES;
	cyclesSinceLastVSync = 0;
	cyclesSinceLastHSync = 0;	
	if(rLCDC->getBit(7)){ // LCD enabled
		lcdDriverMode = 2;
		rSTAT->setBits(0,1,2); // Mode 2
	}
	else{ // LCD disabled
		lcdDriverMode = 1;
		rSTAT->setBits(0,1,1); // Mode 1
	}
	(*rLY) = 0;
}

/** Increment the current scanline (register LY).
  * @return True if there is coincidence with register LYC, and return false otherwise.
  */
bool SystemClock::incrementScanline(){
	cyclesSinceLastHSync = 0; // Reset HSync cycle count
	(*rLY)++; // Increment LY register
	if((*rLY) != (*rLYC)) // LY != LYC
		rSTAT->resetBit(2); // Reset bit 2 of STAT (coincidence flag)
	else{ // LY == LYC
		rSTAT->setBit(2); // Set bit 2 of STAT (coincidence flag)
		if(rSTAT->getBit(6)) // Issue LCD STAT interrupt
			sys->handleLcdInterrupt();
		return true;
	}
	return false;
}

void SystemClock::waitUntilNextVSync(){
	static unsigned int frameCount = 0;
	static double totalRenderTime = 0;
	const double framePeriod = 1E6*VERTICAL_SYNC_CYCLES/SYSTEM_CLOCK_FREQUENCY; // in microseconds
	std::chrono::duration<double, std::micro> wallTime = sclock::now() - timeOfLastVSync;
	double timeToSleep = framePeriod - wallTime.count(); // microseconds
	if(timeToSleep > 0)
		usleep((int)timeToSleep);
	totalRenderTime += std::chrono::duration_cast<std::chrono::duration<double>>(sclock::now() - timeOfLastVSync).count();
	if((++frameCount % 60) == 0){
		framerate = frameCount/totalRenderTime;
		totalRenderTime = 0;
		frameCount = 0;
	}
	timeOfLastVSync = sclock::now();
	cyclesSinceLastVSync = 0;
	vsync = false;
}

void SystemClock::mode0Interrupt(){
	if(rSTAT->getBit(3)){ // Request LCD STAT interrupt (INT 48)
		sys->handleLcdInterrupt();
		std::cout << " mode 0\n";	
	}
}

void SystemClock::mode1Interrupt(){
	sys->handleVBlankInterrupt(); // Request VBlank interrupt (INT 40)
	if(rSTAT->getBit(4)) // Request LCD STAT interrupt (INT 48)
		sys->handleLcdInterrupt();
}

void SystemClock::mode2Interrupt(){
	if(rSTAT->getBit(5)){ // Request LCD STAT interrupt (INT 48)
		sys->handleLcdInterrupt();
		std::cout << " mode 2\n";
	}
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
			rDIV->setValue(0x0);
			break;
		case 0xFF05: // TIMA (Timer counter)
			// Timer incremented by clock frequency specified by TAC. When
			// value overflows, it is set to TMA and a timer interrupt is
			// issued.
			break;
		case 0xFF06: // TMA (Timer modulo)
			break;
		case 0xFF07: // TAC (Timer control)
			timerEnable = rTAC->getBit(2); // Timer stop [0: Stop, 1: Start]
			clockSelect = rTAC->getBits(0,1); // Input clock select (see below)
			/** Input clocks:
				0:   4096 Hz (256 cycles)
				1: 262144 Hz (4 cycles)
				2:  65536 Hz (16 cycles)
				3:  16384 Hz (64 cycles) **/
			switch(clockSelect){
				case 0: // 256 cycles
					timerPeriod = 256;
					break;
				case 1: // 4 cycles
					timerPeriod = 4;
					break;
				case 2: // 16 cycles
					timerPeriod = 16;
					break;
				case 3: // 64 cycles
					timerPeriod = 64;
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
			break;
		case 0xFF05: // TIMA (Timer counter)
			break;
		case 0xFF06: // TMA (Timer modulo)
			break;
		case 0xFF07: // TAC (Timer control)
			break;
		default:
			return false;
	}
	return true;
}

bool SystemTimer::onClockUpdate(){
	if(!timerEnable) return false;
	if((nDividerCycles++) >= 64){ // DIV (divider register) incremented at a rate of 16384 Hz
		nDividerCycles = nDividerCycles % 64;
		(*rDIV)++;
	}
	nCyclesSinceLastTick++;
	while(nCyclesSinceLastTick/timerPeriod){ // Handle a timer tick.
		if(++(*rTIMA) == 0x0) // Timer counter has rolled over
			rollOver();
		// Reset the cycle counter.
		nCyclesSinceLastTick -= timerPeriod;
	}
	return true;
}

void SystemTimer::rollOver(){
	rTIMA->setValue(rTMA->getValue());
	sys->handleTimerInterrupt();
}

void SystemTimer::defineRegisters(){
	sys->addSystemRegister(this, 0x04, rDIV , "DIV" , "33333333");
	sys->addSystemRegister(this, 0x05, rTIMA, "TIMA", "33333333");
	sys->addSystemRegister(this, 0x06, rTMA , "TMA" , "33333333");
	sys->addSystemRegister(this, 0x07, rTAC , "TAC" , "33300000");
}
