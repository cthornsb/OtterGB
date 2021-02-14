#include <thread>
#include <iostream>

#include "SystemClock.hpp"
#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"

constexpr unsigned int SYSTEM_CLOCK_FREQUENCY = 1048576; // Hz

constexpr unsigned int MODE2_START = 0;     // STAT mode 2 from 0 to 79 (80 cycles)
constexpr unsigned int MODE3_START = 20;    // STAT mode 3 from 80 to 251 (172 cycles)
constexpr unsigned int MODE0_START = 63;    // STAT mode 0 from 252 to 456 (204 cycles)
constexpr unsigned int MODE1_START = 16416; // STAT mode 1 from 65664 to 70224 (4560 cycles)

constexpr unsigned int VERTICAL_SYNC_CYCLES   = 17556; // CPU cycles per VSYNC (~59.73 Hz)
constexpr unsigned int HORIZONTAL_SYNC_CYCLES = 114;   // CPU cycles per HSYNC (per 154 scanlines)

SystemClock::SystemClock() : 
	SystemComponent("Clock"), 
	vsync(false), 
	cyclesSinceLastVSync(0), 
	cyclesSinceLastHSync(0),
	currentClockSpeed(0),
	cyclesPerVSync(0),
	cyclesPerHSync(0),
	lcdDriverMode(2), 
	framerate(0),
	framePeriod(0),
	timeOfInitialization(hrclock::now()),
	timeOfLastVSync(hrclock::now()),
	cycleTimer(hrclock::now()),
	cycleCounter(0),
	cyclesPerSecond(0)
{
	setNormalSpeedMode();
}

void SystemClock::setFramerateMultiplier(const float &freq){
	framePeriod = 1E6*VERTICAL_SYNC_CYCLES/SYSTEM_CLOCK_FREQUENCY/freq; // in microseconds
}

void SystemClock::setDoubleSpeedMode(){
	waitUntilNextVSync();
	currentClockSpeed = SYSTEM_CLOCK_FREQUENCY * 2;
	modeStart[0] = MODE0_START * 2;
	modeStart[1] = MODE1_START * 2;
	modeStart[2] = MODE2_START * 2;
	modeStart[3] = MODE3_START * 2;
	cyclesPerVSync = VERTICAL_SYNC_CYCLES * 2;
	cyclesPerHSync = HORIZONTAL_SYNC_CYCLES * 2;
	framePeriod = 1E6*VERTICAL_SYNC_CYCLES/SYSTEM_CLOCK_FREQUENCY; // in microseconds
}

void SystemClock::setNormalSpeedMode(){
	waitUntilNextVSync();
	currentClockSpeed = SYSTEM_CLOCK_FREQUENCY;
	modeStart[0] = MODE0_START;
	modeStart[1] = MODE1_START;
	modeStart[2] = MODE2_START;
	modeStart[3] = MODE3_START;
	cyclesPerVSync = VERTICAL_SYNC_CYCLES;
	cyclesPerHSync = HORIZONTAL_SYNC_CYCLES;
	framePeriod = 1E6*VERTICAL_SYNC_CYCLES/SYSTEM_CLOCK_FREQUENCY; // in microseconds
}

// Tick the system clock.
bool SystemClock::onClockUpdate(){
	if(++cycleCounter % (currentClockSpeed*10) == 0){ // Every 10 seconds
		std::chrono::duration<double> wallTime = hrclock::now() - cycleTimer;
		cycleTimer = hrclock::now();
		cyclesPerSecond = currentClockSpeed*10/wallTime.count();
		if(verboseMode){
			double percentDifference = 100.0*(cyclesPerSecond-currentClockSpeed)/currentClockSpeed;
			std::cout << " SystemClock: " << wallTime.count() << " seconds (" << cyclesPerSecond << " cycles/s, " << percentDifference;
			if(percentDifference < 0)
				std::cout << "% [slow])\n";
			else
				std::cout << "% [fast])\n"; 
		}
	}

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
	if(cyclesSinceLastVSync <= modeStart[1]){ // Visible scanlines (0-143)
		// 0 to 20   : mode2 - OAM sprite search (20 cycles)
		// 20 to 63  : mode3 - Drawing pixels (43+X cycles)
		// 63 to 114 : mode0 - HBlank (51-X cycles)
		// ...
		// 16416 to 17556 : mode1 - VBlank
		if(cyclesSinceLastHSync == modeStart[3]){ // Mode 2->3 (Drawing the scanline)
			lcdDriverMode = 3;
			rSTAT->setBits(0,1); // Mode 3
			sys->handleHBlankPeriod(); // Start drawing the scanline
		}		
		else if(cyclesSinceLastHSync == modeStart[0]){ // Mode 3->0 (HBlank)
			lcdDriverMode = 0;
			rSTAT->resetBits(0,1); // Mode 0
			mode0Interrupt();
		}		
		else if(cyclesSinceLastHSync == cyclesPerHSync){ // Mode 0->2(1)
			incrementScanline(); // Increment the scanline
			if(cyclesSinceLastVSync != modeStart[1]){ // Mode 0->2 (Reading from OAM)
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
	else if(cyclesSinceLastVSync <= cyclesPerVSync){ // Mode 1 - Overscan (144-153)
		if(cyclesSinceLastHSync == cyclesPerHSync){
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
	rLY->clear();
	rWLY->clear();
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
	std::chrono::duration<double, std::micro> wallTime = hrclock::now() - timeOfLastVSync;
	double timeToSleep = framePeriod - wallTime.count(); // microseconds
	if(timeToSleep > 0)
		std::this_thread::sleep_for(std::chrono::microseconds((long long)timeToSleep));
	totalRenderTime += std::chrono::duration_cast<std::chrono::duration<double>>(hrclock::now() - timeOfLastVSync).count();
	if((++frameCount % 60) == 0){
		framerate = frameCount/totalRenderTime;
		totalRenderTime = 0;
		frameCount = 0;
	}
	timeOfLastVSync = hrclock::now();
	vsync = false;
}

void SystemClock::mode0Interrupt(){
	if(rSTAT->getBit(3)){ // Request LCD STAT interrupt (INT 48)
		sys->handleLcdInterrupt();
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
	}
}
