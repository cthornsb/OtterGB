#include <iostream>

#include "SystemClock.hpp"
#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "ConfigFile.hpp"

// System clock
constexpr unsigned int SYSTEM_CLOCK_FREQUENCY = 1048576; // Hz

// Pixel clock
constexpr unsigned int MODE2_START = 0;     // STAT mode 2 start at 0 (80 cycles)
constexpr unsigned int MODE3_START = 80;    // STAT mode 3 start at 80 (168-291 cycles)
constexpr unsigned int MODE0_START = 248;   // STAT mode 0 start at 248-371 (85-208 cycles)
constexpr unsigned int MODE1_START = 65664; // STAT mode 1 from 65664 (4560 cycles)
constexpr unsigned int VERTICAL_SYNC_CYCLES   = 70224; // CPU cycles per VSYNC (~59.73 Hz)
constexpr unsigned int HORIZONTAL_SYNC_CYCLES = 456;   // CPU cycles per HSYNC (per 154 scanlines)

SystemClock::SystemClock() : 
	OTTFrameTimer(),
	SystemComponent("Clock", 0x204b4c43), // "CLK "
	bLcdEnabled(true), // LCD driver starts enabled in the event the ROM does not activate it
	bDoubleSpeedMode(false),
	bVSync(false), 
	cyclesSinceLastVSync(0), 
	cyclesSinceLastHSync(0),
	currentClockSpeed(0),
	cyclesPerVSync(0),
	cyclesPerHSync(0),
	lcdDriverMode(2), 
	cycleCounter(0),
	cyclesPerSecond(0),
	modeStart(),
	delayedModeStart0(0)
{
	setFramerateCap(4.0 * SYSTEM_CLOCK_FREQUENCY / VERTICAL_SYNC_CYCLES); // Set default framerate cap (59.73 fps)
}

void SystemClock::setFramerateMultiplier(const float &freq){
	setFramerateCap(freq * 4.0 * SYSTEM_CLOCK_FREQUENCY / VERTICAL_SYNC_CYCLES);
}

void SystemClock::setDoubleSpeedMode(){
	this->sync();
	currentClockSpeed = SYSTEM_CLOCK_FREQUENCY * 2;
	modeStart[0] = MODE0_START * 2;
	modeStart[1] = MODE1_START * 2;
	modeStart[2] = MODE2_START * 2;
	modeStart[3] = MODE3_START * 2;
	delayedModeStart0 = modeStart[0];
	cyclesPerVSync = VERTICAL_SYNC_CYCLES * 2;
	cyclesPerHSync = HORIZONTAL_SYNC_CYCLES * 2;
	cyclesSinceLastVSync = 0;
	cyclesSinceLastHSync = 0;
	if(verboseMode){
		std::cout << " [Clock] Switched CPU speed to double-speed mode." << std::endl;
	}
	bDoubleSpeedMode = true;
}

void SystemClock::setNormalSpeedMode(){
	this->sync();
	currentClockSpeed = SYSTEM_CLOCK_FREQUENCY;
	modeStart[0] = MODE0_START;
	modeStart[1] = MODE1_START;
	modeStart[2] = MODE2_START;
	modeStart[3] = MODE3_START;
	delayedModeStart0 = modeStart[0];
	cyclesPerVSync = VERTICAL_SYNC_CYCLES;
	cyclesPerHSync = HORIZONTAL_SYNC_CYCLES;
	cyclesSinceLastVSync = 0;
	cyclesSinceLastHSync = 0;
	if(verboseMode){
		std::cout << " [Clock] Switched CPU speed to normal." << std::endl;
	}
	bDoubleSpeedMode = false;
}

void SystemClock::setLcdState(const bool& state) {
	if (!state) { // LCD & PPU disabled
		// Enter PPU mode 1
		bVSync = true;
		startMode1(false);
		bLcdEnabled = false;
	}
	else{ // LCD & PPU enabled
		if (!bLcdEnabled) { // LCD is being turned on
			// PPU immediately resumes drawing operations
			// Reset to a new frame to start drawing
			bVSync = false;
			cyclesSinceLastVSync = 4;
			cyclesSinceLastHSync = 4;	
			startMode2(false);
			rLY->clear();
			rWLY->clear();	
		}
		bLcdEnabled = true;
	}
}

double SystemClock::getCyclesPerSecond() const {
	return (VERTICAL_SYNC_CYCLES * dFramerateCap / 4.0);
}

// Tick the system clock.
bool SystemClock::onClockUpdate(){
	/*if(++cycleCounter % (currentClockSpeed*10) == 0){ // Every 10 seconds
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
	}*/

	// Tick the pixel clock (~4 MHz)
	cyclesSinceLastVSync += 4;

	if (!bLcdEnabled) { // LCD disabled
		if (cyclesSinceLastVSync >= cyclesPerVSync) {
			this->sync();
			cyclesSinceLastVSync = 0;
		}
		return bVSync;
	}

	// Tick the current scanline's pixel clock
	cyclesSinceLastHSync += 4;

	// Check if we're on the next scanline (every 456 cycles)
	// Set the LCD STAT register mode flag:
	//  Complete screen refresh takes 70224 cycles
	//  Each scanline takes 456 cycles (154 total scanlines, 0-153)
	//  10 scanlines of overscan takes 4560 cycles (VBlank) (lines 144-153)
	//  The mode progression is 2->3->0->2->3->0 ... ->2->3->0->1->2 ...	
	if(cyclesSinceLastVSync <= modeStart[1]){ // Visible scanlines (0-143)
		// Pixel clock (~4 MHz, 70224 per frame)
		// 0 to 80        : mode 2 (80 ticks)
		// 80 to 248+X    : mode 3 (168 to 291 ticks, X in range 0 to 123)
		// 248+X to 456   : mode 0 (85 to 208 ticks, depending on X) 
		// ...            : 143 more scanlines (143 * 456 = 65208 ticks)
		// 65664 to 70224 : mode 1 (4560 ticks)
		if(cyclesSinceLastHSync == modeStart[3]){ // Mode 2->3 (Drawing the scanline)
			startMode3();
		}
		else if(cyclesSinceLastHSync == delayedModeStart0){ // Mode 3->0 (HBlank), delayed up to 123 pixel clock ticks
			startMode0();
		}	
		else if(cyclesSinceLastHSync == cyclesPerHSync){ // Mode 0->2(1)
			incrementScanline(); // Increment the scanline
			if(cyclesSinceLastVSync != modeStart[1]){ // Mode 0->2 (Reading from OAM)
				startMode2();
			}
			else{ // Mode 0->1 (VBlank, overscan)
				startMode1(); // vsync flag set to true
			}
		}
	}
	else if(cyclesSinceLastVSync < cyclesPerVSync){ // Mode 1 - Overscan (144-153)
		if(cyclesSinceLastHSync == cyclesPerHSync){
			incrementScanline();
		}
	}
	else{ // Start the next frame (Mode 1->2)
		this->sync(); // Sync to frame timer
		resetScanline(); // VBlank period has ended, next frame started
	}
	
	return bVSync;
}

void SystemClock::readConfigFile(ConfigFile* config) {
	if (config->search("TARGET_FRAMERATE", true)) // Set framerate target (fps)
		setFramerateCap(config->getDouble());
	if (config->search("FRAME_TIME_OFFSET", true)) // Set frame timer offset (in microseconds)
		setFramePeriodOffset(config->getDouble());
}

void SystemClock::resetScanline(){
	bVSync = false;
	cyclesSinceLastVSync = 0;
	cyclesSinceLastHSync = 0;
	delayedModeStart0 = modeStart[0];
	startMode2();
	rLY->clear();
	rWLY->clear();	
	compareScanline(); // Handle LY / LYC coincidence
}

void SystemClock::incrementScanline(){
	cyclesSinceLastHSync = 0; // Reset HSync cycle count
	(*rLY)++; // Increment LY register
	compareScanline(); // Handle LY / LYC coincidence
}

bool SystemClock::compareScanline(){
	if((*rLY) != (*rLYC)) // LY != LYC
		rSTAT->resetBit2(); // Reset bit 2 of STAT (coincidence flag)
	else{ // LY == LYC
		rSTAT->setBit2(); // Set bit 2 of STAT (coincidence flag)
		if(rSTAT->bit6()) // Issue LCD STAT interrupt
			sys->handleLcdInterrupt();
		return true;
	}
	return false;
}

void SystemClock::startMode0(){
	lcdDriverMode = 0;
	rSTAT->setBits(0, 1, 0x0);
	if (rSTAT->bit3())  // Request LCD STAT interrupt (INT 48)
		sys->handleLcdInterrupt();
	sys->startMode0(); // VRAM and OAM now accessible
}

void SystemClock::startMode1(bool requestInterrupt/* = true*/){
	bVSync = true;
	lcdDriverMode = 1;
	rSTAT->setBits(0, 1, 0x1);
	if (requestInterrupt) {
		if (rSTAT->bit4())
			sys->handleLcdInterrupt(); // Request LCD STAT interrupt (INT 48)
		sys->handleVBlankInterrupt(); // Request VBlank interrupt (INT 40)
	}
	sys->startMode1(); // VRAM and OAM now accessible
}

void SystemClock::startMode2(bool requestInterrupt/* = true*/){
	lcdDriverMode = 2;
	rSTAT->setBits(0, 1, 0x2);
	if (requestInterrupt && rSTAT->bit5()) // Request LCD STAT interrupt (INT 48)
		sys->handleLcdInterrupt();
	sys->startMode2(); // OAM inaccessible
}

void SystemClock::startMode3(){
	lcdDriverMode = 3;
	rSTAT->setBits(0, 1, 0x3);
	sys->startMode3(); // VRAM and OAM inaccessible
}

void SystemClock::userAddSavestateValues(){
	unsigned int sizeULong = sizeof(unsigned int);
	unsigned int sizeDouble = sizeof(double);
	// Ints
	addSavestateValue(&cyclesSinceLastVSync, sizeULong);
	addSavestateValue(&cyclesSinceLastHSync, sizeULong);
	addSavestateValue(&currentClockSpeed,    sizeULong);
	addSavestateValue(&cyclesPerVSync,       sizeULong);
	addSavestateValue(&cyclesPerHSync,       sizeULong);
	addSavestateValue(&cycleCounter,         sizeULong);
	addSavestateValue(modeStart,             sizeULong * 4);
	// Bytes
	addSavestateValue(&lcdDriverMode, sizeof(unsigned char));
	addSavestateValue(&bVSync,        sizeof(bool));
	// Doubles
	addSavestateValue(&dFramerate, sizeDouble);
	addSavestateValue(&dFramePeriod, sizeDouble);
	addSavestateValue(&cyclesPerSecond, sizeDouble);
}

void SystemClock::onUserReset(){
	bVSync = false;
	this->resetTimer();
	setNormalSpeedMode();
	//timeOfInitialization = hrclock::now();
	//timeOfLastVSync = hrclock::now();
	//cycleTimer = hrclock::now();
	cycleCounter = 0;
	lcdDriverMode = 2;
}

