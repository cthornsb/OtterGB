
#include <unistd.h>
#include <iostream>
#include <string>

#include "Support.hpp"
#include "SystemGBC.hpp"
#include "SystemRegisters.hpp"
#include "Graphics.hpp"

#ifdef USE_QT_DEBUGGER
	#include <QApplication>
	#include "mainwindow.h"
#endif

#define ROM_ZERO_START  0x0000
#define ROM_SWAP_START  0x4000
#define VRAM_SWAP_START 0x8000
#define CART_RAM_START  0xA000
#define WRAM_ZERO_START 0xC000
#define WRAM_SWAP_START 0xD000
#define ECHO_RAM_START  0xE000
#define OAM_TABLE_START 0xFE00
#define RESERVED_START  0xFEA0
#define IO_PORTS_START  0xFF00
#define HIGH_RAM_START  0xFF80
#define INTERRUPT_ENABLE 0xFFFF

#ifdef GB_BOOT_ROM
	const std::string gameboyBootRomPath(GB_BOOT_ROM);
#else
	const std::string gameboyBootRomPath("");
#endif

#ifdef GBC_BOOT_ROM
	const std::string gameboyColorBootRomPath(GBC_BOOT_ROM);
#else
	const std::string gameboyColorBootRomPath("");
#endif

/** INTERRUPTS:
	0x40 - VBlank interrupt (triggered at beginning of VBlank period [~59.7 Hz])
	0x48 - LCDC status interrupt
	0x50 - Timer interrupt (triggered every time the timer overflows)
	0x58 - Serial interrupt (triggered after sending/receiving 8 bits)
	0x60 - Joypad interrupt (triggered any time a joypad button is pressed)
**/
	
ComponentList::ComponentList(SystemGBC *sys){
		list["APU"]       = (apu = &sys->sound);
		list["Cartridge"] = (cart = &sys->cart);
		list["CPU"]       = (cpu = &sys->cpu);
		list["DMA"]       = (dma = &sys->dma);
		list["GPU"]       = (gpu = &sys->gpu);
		list["HRAM"]      = (hram = &sys->hram);
		list["Joypad"]    = (joy = &sys->joy);
		list["OAM"]       = (oam = &sys->oam);
		list["Clock"]     = (sclk = &sys->clock);
		list["Serial"]    = (serial = &sys->serial);
		list["Timer"]     = (timer = &sys->timer);
		list["WRAM"]      = (wram = &sys->wram);
}
	
SystemGBC::SystemGBC() : nFrames(0), frameSkip(1), verboseMode(false), debugMode(false), cpuStopped(false), cpuHalted(false), 
                         emulationPaused(false), bootSequence(false), forceColor(false), prepareSpeedSwitch(false), currentClockSpeed(false), displayFramerate(false),
                         userQuitting(false), dmaSourceH(0), dmaSourceL(0), dmaDestinationH(0), dmaDestinationL(0), romFilename() { 
	// Disable memory region monitor
	memoryAccessWrite[0] = 1; 
	memoryAccessWrite[1] = 0;
	memoryAccessRead[0] = 1; 
	memoryAccessRead[1] = 0;

	// Add all components to the subsystem list
	subsystems = std::unique_ptr<ComponentList>(new ComponentList(this));

	// Initialize system
	this->initialize();
	
	pauseAfterNextInstruction = false;
	pauseAfterNextHBlank = false;
	pauseAfterNextVBlank = false;
}

SystemGBC::~SystemGBC(){
}

void SystemGBC::initialize(){ 
	hram.initialize(127);

	// Define system registers
	addSystemRegister(0x0, 0x0F, rIF,   "IF",   "33333000");
	addSystemRegister(0x0, 0x4D, rKEY1, "KEY1", "30000001");
	addSystemRegister(0x0, 0x56, rRP,   "RP",   "31000033");
	rIE  = new Register("IE",  "33333000");
	rIME = new Register("IME", "30000000");
	(*rIME) = 1; // Interrupts enabled by default

	// Undocumented registers
	addSystemRegister(0x0, 0x6C, rFF6C, "FF6C", "30000000");
	addSystemRegister(0x0, 0x72, rFF72, "FF72", "33333333");
	addSystemRegister(0x0, 0x73, rFF73, "FF73", "33333333");
	addSystemRegister(0x0, 0x74, rFF74, "FF74", "33333333");
	addSystemRegister(0x0, 0x75, rFF75, "FF75", "00003330");
	addSystemRegister(0x0, 0x76, rFF76, "FF76", "11111111");
	addSystemRegister(0x0, 0x77, rFF77, "FF77", "11111111");

	// Define sub-system registers and connect to the system bus.
	for(auto comp = subsystems->list.begin(); comp != subsystems->list.end(); comp++){
		comp->second->connectSystemBus(this);
		comp->second->defineRegisters();
	}

	// Set memory offsets for all components	
	gpu.setOffset(VRAM_SWAP_START);
	cart.getRam()->setOffset(CART_RAM_START);
	wram.setOffset(WRAM_ZERO_START);
	oam.setOffset(OAM_TABLE_START);
	hram.setOffset(HIGH_RAM_START);

	// Must connect the system bus BEFORE initializing the CPU.
	cpu.initialize();
	
	// Disable the system timer.
	timer.disable();

	// Initialize the window and link it to the joystick controller	
	gpu.initialize();
	joy.setWindow(gpu.getWindow());
}

bool SystemGBC::execute(){
	// Run the ROM. Main loop.
	while(true){
		// Check the status of the GPU and LCD screen
		if(!gpu.getWindowStatus() || userQuitting)
			break;

		if(!emulationPaused && !cpuStopped){
			// Check for interrupt out of HALT
			if(cpuHalted){
				if(((*rIE) & (*rIF)) != 0)
					cpuHalted = false;
			}

			// Check if the CPU is halted.
			if(!cpuHalted && !dma.onClockUpdate()){
				// Perform one instruction.
				if(cpu.onClockUpdate()){
#ifdef USE_QT_DEBUGGER
					if(pauseAfterNextInstruction){
						pauseAfterNextInstruction = false;					
						pause();
					}
					else if(breakpointOpcode.check(cpu.getLastOpcode()->index) ||
					   breakpointProgramCounter.check(cpu.getLastOpcode()->pc))
						pause();
#endif
				}
			}

			// Update system timer.
			timer.onClockUpdate();

			// Update sound processor.
			sound.onClockUpdate();

			// Update joypad handler.
			joy.onClockUpdate();

			// Tick the system clock.
			clock.onClockUpdate();

			// Sync with the framerate.	
			if(clock.pollVSync()){
				// Process window events
				gpu.getWindow()->processEvents();
				checkSystemKeys();
				
				// Render the current frame
				if(nFrames++ % frameSkip == 0 && !cpuStopped){
					if(displayFramerate)
						gpu.print(doubleToStr(clock.getFramerate(), 1)+" fps", 0, 17);
					gpu.render();
				}
#ifdef USE_QT_DEBUGGER
				if(debugMode){
					if(!pauseAfterNextVBlank){
						gui->processEvents();
						gui->update();
					}
					else{
						pauseAfterNextVBlank = false;
						pause();
					}
				}
#endif
			}
		}
		else{
			if(cpuStopped){ // STOP
				std::cout << " Stopped! " << getHex(rIE->getValue()) << " " << getHex(rIF->getValue()) << std::endl;
				//if((*rIF) == 0x10)
					resumeCPU();
			}
		
			// Process window events
			gpu.getWindow()->processEvents();
			checkSystemKeys();

			// Maintain framerate but do not advance the system clock.
			clock.wait();
#ifndef USE_QT_DEBUGGER
		}
	}
#else
			if(debugMode) // Process events for the Qt GUI
				gui->processEvents();
		}
	}
	if(debugMode)
		gui->closeAllWindows(); // Clean up the Qt GUI
#endif
	return true;
}

void SystemGBC::handleHBlankPeriod(){
	if(!emulationPaused){
		if(nFrames % frameSkip == 0)
			gpu.drawNextScanline(&oam);
		dma.onHBlank();
	}
#ifdef USE_QT_DEBUGGER
	if(debugMode && pauseAfterNextHBlank){
		pauseAfterNextHBlank = false;
		pause();
		gpu.render(); // Show the newly drawn scanline
	}
#endif
}
	
void SystemGBC::handleVBlankInterrupt(){ 
	rIF->setBit(0);
}

void SystemGBC::handleLcdInterrupt(){ 
	rIF->setBit(1);
}

void SystemGBC::handleTimerInterrupt(){ 
	rIF->setBit(2);
}

void SystemGBC::handleSerialInterrupt(){ 
	rIF->setBit(3); 
}

void SystemGBC::handleJoypadInterrupt(){ 
	rIF->setBit(4); 
}

void SystemGBC::enableInterrupts(){ 
	(*rIME) = 1; 
}

void SystemGBC::disableInterrupts(){ 
	(*rIME) = 0;
}

bool SystemGBC::write(const unsigned short &loc, const unsigned char &src){
	// Check for memory access watch
	if(loc >= memoryAccessWrite[0] && loc <= memoryAccessWrite[1]){
		LR35902::OpcodeData *op = cpu.getLastOpcode();
		std::cout << " (W) PC=" << getHex(op->pc) << " " << getHex(src) << "->[" << getHex(loc) << "] ";
		if(op->op->nBytes == 2)
			std::cout << "d8=" << getHex(op->getd8());
		else if(op->op->nBytes == 3)
			std::cout << "d16=" << getHex(op->getd16());
		std::cout << std::endl;
	}
	
	// Check for memory write breakpoint
	if(breakpointMemoryWrite.check(loc))
		pause();

	// Check for system registers
	if(loc >= REGISTER_LOW && loc < REGISTER_HIGH){
		// Write the register
		unsigned char wramBank;
		if(!writeRegister(loc, src))
			return false;
	}
	else{ // Attempt to write to memory
		switch(loc){
			case 0x0000 ... 0x7FFF: // Cartridge ROM 
				cart.writeRegister(loc, src); // Write to cartridge MBC (if present)
				break;
			case 0x8000 ... 0x9FFF: // Video RAM (VRAM)
				gpu.write(loc, src);
				break;
			case 0xA000 ... 0xBFFF: // External (cartridge) RAM
				cart.getRam()->write(loc, src);
				break;
			case 0xC000 ... 0xFDFF: // Work RAM (WRAM) 0-1 and ECHO
				wram.write(loc, src);
				break;
			case 0xFE00 ... 0xFE9F: // Sprite table (OAM)
				oam.write(loc, src);
				break;
			case 0xFF80 ... 0xFFFE: // High RAM (HRAM)
				hram.write(loc, src);
				break;
			case 0xFFFF: // Interrupt enable
				rIE->write(src);
				break;
			default:
				return false;
		}
	}

	return true; // Successfully wrote to memory location (loc)
}

bool SystemGBC::read(const unsigned short &loc, unsigned char &dest){
	// Check for memory read breakpoint
	if(breakpointMemoryRead.check(loc))
		pause();

	// Check for system registers
	if(loc >= REGISTER_LOW && loc < REGISTER_HIGH){
		// Read the register
		if(!readRegister(loc, dest))
			return false;
	}
	else{ // Attempt to read from memory
		switch(loc){
			case 0x0000 ... 0x7FFF: // Cartridge ROM 
				if(bootSequence && (loc < 0x100 || loc >= 0x200)){ // Startup sequence. 
					//Ignore bytes between 0x100 and 0x200 where the cartridge header lives.
					dest = bootROM[loc];
				}
				else
					cart.read(loc, dest);
				break;
			case 0x8000 ... 0x9FFF:
				gpu.read(loc, dest);
				break;
			case 0xA000 ... 0xBFFF:
				cart.getRam()->read(loc, dest);
				break;
			case 0xC000 ... 0xFDFF:
				wram.read(loc, dest);
				break;
			case 0xFE00 ... 0xFE9F:
				oam.read(loc, dest);
				break;
			case 0xFF80 ... 0xFFFE:
				hram.read(loc, dest);
				break;
			case 0xFFFF: // Interrupt enable
				dest = rIE->read();
				break;
			default:
				return false;
		}
	}

	// Check for memory access watch
	if(loc >= memoryAccessRead[0] && loc <= memoryAccessRead[1]){
		LR35902::OpcodeData *op = cpu.getLastOpcode();
		std::cout << " (R) PC=" << getHex(op->pc) << " [" << getHex(loc) << "]=" << getHex(dest) << "\n";
	}

	return true; // Successfully read from memory location (loc)
}

unsigned char SystemGBC::getValue(const unsigned short &loc){
	unsigned char retval;
	read(loc, retval);
	return retval;
}

unsigned char *SystemGBC::getPtr(const unsigned short &loc){
	// Note: Direct access to ROM banks is restricted. 
	// Use write() and read() methods to access instead.
	unsigned char *retval = 0x0;
	switch(loc){
		case 0x8000 ... 0x9FFF: // Video RAM (VRAM)
			retval = gpu.getPtr(loc);
			break;
		case 0xA000 ... 0xBFFF: // External (cartridge) RAM (if available)
			retval = cart.getRam()->getPtr(loc);
			break;
		case 0xC000 ... 0xFDFF: // Work RAM (WRAM) 0-1 and ECHO
			retval = wram.getPtr(loc);
			break;
		case 0xFE00 ... 0xFE9F: // Sprite table (OAM)
			retval = oam.getPtr(loc);
			break;
		case 0xFF00 ... 0xFF7F: // System registers
			retval = getPtrToRegisterValue(loc);
			break;
		case 0xFF80 ... 0xFFFE: // High RAM (HRAM)
			retval = hram.getPtr(loc);
			break;
		case 0xFFFF:
			retval = rIE->getPtr();
		default:
			break;
	}
	return retval;
}

const unsigned char *SystemGBC::getConstPtr(const unsigned short &loc){
	// Note: Direct access to ROM banks is restricted. 
	// Use write() and read() methods to access instead.
	const unsigned char *retval = 0x0;
	switch(loc){
		case 0x0000 ... 0x7FFF: // ROM
			retval = cart.getConstPtr(loc);
			break;
		case 0x8000 ... 0x9FFF: // Video RAM (VRAM)
			retval = gpu.getConstPtr(loc);
			break;
		case 0xA000 ... 0xBFFF: // External (cartridge) RAM (if available)
			retval = cart.getRam()->getConstPtr(loc);
			break;
		case 0xC000 ... 0xFDFF: // Work RAM (WRAM) 0-1 and ECHO
			retval = wram.getConstPtr(loc);
			break;
		case 0xFE00 ... 0xFE9F: // Sprite table (OAM)
			retval = oam.getConstPtr(loc);
			break;
		case 0xFF00 ... 0xFF7F: // System registers
			retval = getConstPtrToRegisterValue(loc);
			break;
		case 0xFF80 ... 0xFFFE: // High RAM (HRAM)
			retval = hram.getConstPtr(loc);
			break;
		case 0xFFFF:
			retval = rIE->getConstPtr();
		default:
			break;
	}
	return retval;
}

Register *SystemGBC::getPtrToRegister(const unsigned short &reg){
	if(reg < REGISTER_LOW || reg >= REGISTER_HIGH)
		return 0x0;
	return &registers[reg-0xFF00];
}

unsigned char *SystemGBC::getPtrToRegisterValue(const unsigned short &reg){
	if(reg < REGISTER_LOW || reg >= REGISTER_HIGH)
		return 0x0;
	return registers[reg-0xFF00].getPtr();
}

const unsigned char *SystemGBC::getConstPtrToRegisterValue(const unsigned short &reg){
	if(reg < REGISTER_LOW || reg >= REGISTER_HIGH)
		return 0x0;
	return registers[reg-0xFF00].getConstPtr();
}

void SystemGBC::setDebugMode(bool state/*=true*/){
	debugMode = state;
	for(auto comp = subsystems->list.begin(); comp != subsystems->list.end(); comp++)
		comp->second->setDebugMode(state);
#ifdef USE_QT_DEBUGGER
	if(debugMode){
		int dummyARGC = 1;
		char *dummyARGV[1] = {0};
		std::unique_ptr<ComponentList>(new ComponentList(this));
		app = std::unique_ptr<QApplication>(new QApplication(dummyARGC, dummyARGV));
		gui = std::unique_ptr<MainWindow>(new MainWindow(app.get()));
		gui->connectToSystem(this, subsystems.get());
	}
#endif
}

// Set CPU frequency multiplier
void SystemGBC::setCpuFrequency(const double &multiplier){
	clock.setFrequencyMultiplier(multiplier);
}

// Toggle verbose flag
void SystemGBC::setVerboseMode(bool state/*=true*/){
	verboseMode = state;
}

void SystemGBC::setMemoryWriteRegion(const unsigned short &locL, const unsigned short &locH/*=0*/){
	memoryAccessWrite[0] = locL;
	if(locH > locL){
		memoryAccessWrite[1] = locH;
		std::cout << " Watching writes to memory in range " << getHex(locL) << " to " << getHex(locH) << std::endl;
	}
	else{
		memoryAccessWrite[1] = locL;
		std::cout << " Watching writes to memory location " << getHex(locL) << std::endl;
	}
}

void SystemGBC::setMemoryReadRegion(const unsigned short &locL, const unsigned short &locH/*=0*/){
	memoryAccessRead[0] = locL;
	if(locH > locL){
		memoryAccessRead[1] = locH;
		std::cout << " Watching reads from memory in range " << getHex(locL) << " to " << getHex(locH) << std::endl;
	}
	else{
		memoryAccessRead[1] = locL;
		std::cout << " Watching reads from memory location " << getHex(locL) << std::endl;
	}
}

void SystemGBC::setBreakpoint(const unsigned short &pc){
	breakpointProgramCounter.enable(pc);
}

void SystemGBC::setMemWriteBreakpoint(const unsigned short &addr){
	breakpointMemoryWrite.enable(addr);
}

void SystemGBC::setMemReadBreakpoint(const unsigned short &addr){
	breakpointMemoryRead.enable(addr);
	std::cout << this << "\tread=" << getHex(addr) << std::endl;
}

void SystemGBC::setOpcodeBreakpoint(const unsigned char &op, bool cb/*=false*/){
	breakpointOpcode.enable(op);
}

void SystemGBC::clearBreakpoint(){
	breakpointProgramCounter.clear();
}

void SystemGBC::clearMemWriteBreakpoint(){
	breakpointMemoryWrite.clear();
}

void SystemGBC::clearMemReadBreakpoint(){
	breakpointMemoryRead.clear();
}

void SystemGBC::clearOpcodeBreakpoint(){
	breakpointOpcode.clear();
}

void SystemGBC::addSystemRegister(SystemComponent *comp, const unsigned char &reg, Register* &ptr, const std::string &name, const std::string &bits){
	registers[reg].setName(name);
	registers[reg].setMasks(bits);
	registers[reg].setAddress(0xFF00+reg);
	registers[reg].setSystemComponent(comp);
	ptr = &registers[reg];
}

void SystemGBC::addDummyRegister(SystemComponent *comp, const unsigned char &reg){
	registers[reg].setSystemComponent(comp);
}

void SystemGBC::clearRegister(const unsigned char &reg){ 
	registers[reg].clear(); 
}

bool SystemGBC::dumpMemory(const char *fname){
	std::cout << " Writing system memory to file \"" << fname << "\"... ";
	std::ofstream ofile(fname, std::ios::binary);
	if(!ofile.good()){
		std::cout << "FAILED!\n";
		return false;
	}
		
	unsigned char byte;
	for(unsigned int i = 0; i <= 0xFFFF; i++){
		// Read a byte from memory.
		if(i >= 0xFEA0 && i < 0xFF00) // Not accessible
			byte = 0x00;
		else if(!read((unsigned short)i, byte)){
			if(i >= REGISTER_LOW && i < REGISTER_HIGH) // System registers
				byte = registers[i-REGISTER_LOW].getValue();
			else{
				std::cout << " WARNING! Failed to read memory location " << getHex((unsigned short)i) << "!\n";
				byte = 0x00; // Write an empty byte
			}
		}
		ofile.write((char*)&byte, 1);
	}
	ofile.close();

	std::cout << "DONE\n";

	return true;
}

bool SystemGBC::dumpVRAM(const char *fname){
	std::cout << " Writing VRAM to file \"" << fname << "\"... ";
	std::ofstream ofile(fname, std::ios::binary);
	if(!ofile.good() || !gpu.writeMemoryToFile(ofile)){
		std::cout << "FAILED!\n";
		return false;
	}
	ofile.close();
	std::cout << " DONE\n";
	return true;
}

bool SystemGBC::dumpSRAM(const char *fname){
	if(!cart.getRam()->getSize()){
		std::cout << " WARNING! Cartridge has no SRAM.\n";
		return false;
	}
	std::cout << " Writing cartridge RAM to file \"" << fname << "\"... ";
	std::ofstream ofile(fname, std::ios::binary);
	if(!ofile.good() || !cart.getRam()->writeMemoryToFile(ofile)){
		std::cout << "FAILED!\n";
		return false;
	}
	ofile.close();
	std::cout << " DONE\n";
	return true;
}

void SystemGBC::resumeCPU(){ 
	cpuStopped = false;
	if(prepareSpeedSwitch){
		if(!currentClockSpeed){ // Normal speed
			currentClockSpeed = true;
			clock.setFrequencyMultiplier(2.0);
		}
		else{ // Double speed
			currentClockSpeed = false;
			clock.setFrequencyMultiplier(1.0);
		}
		(*rKEY1) = 0x0; // Zero register bits
		if(currentClockSpeed)
			(*rKEY1) |= 0x80;
		prepareSpeedSwitch = false;
	}
}

void SystemGBC::pause(){ 
	emulationPaused = true; 
#ifdef USE_QT_DEBUGGER
	if(debugMode){
		gui->updatePausedState(true);
		gui->processEvents();
		gui->update();
	}
#endif
}

void SystemGBC::unpause(){ 
	emulationPaused = false; 
#ifdef USE_QT_DEBUGGER
	if(debugMode)
		gui->updatePausedState(false);
#endif
}

bool SystemGBC::reset(){
	// Set default register values.
	cpu.reset();
	
	// Read the ROM into memory
	bool retval = cart.readRom(romFilename, verboseMode);

	// Check that the ROM is loaded and the window is open
	if(!retval || !gpu.getWindowStatus())
		return false;

	// Enable GBC features for original GB games.
	if(forceColor){
		if(!bGBCMODE)
			bGBCMODE = true;
		else // Disable force color for GBC games
			forceColor = false;
	}

	// Load the boot ROM (if available)
	bool loadBootROM = false;
	std::ifstream bootstrap;
	if(bGBCMODE){
		if(!gameboyColorBootRomPath.empty()){
			bootstrap.open(gameboyColorBootRomPath.c_str(), std::ios::binary);
			if(!bootstrap.good())
				std::cout << " Warning! Failed to load GBC boot ROM \"" << gameboyColorBootRomPath << "\".\n";
			else
				loadBootROM = true;
		}
	}
	else{
		if(!gameboyBootRomPath.empty()){
			bootstrap.open(gameboyBootRomPath.c_str(), std::ios::binary);
			if(!bootstrap.good())
				std::cout << " Warning! Failed to load GB boot ROM \"" << gameboyBootRomPath << "\".\n";
			else
				loadBootROM = true;
		}
	}
	
	if(loadBootROM){
		bootstrap.seekg(0, bootstrap.end);
		bootLength = bootstrap.tellg();
		bootstrap.seekg(0);
		bootROM.reserve(bootLength);
		bootstrap.read((char*)bootROM.data(), bootLength); // Read the entire boot ROM at once
		bootstrap.close();
		std::cout << " Successfully loaded " << bootLength << " B boot ROM.\n";
		cpu.setProgramCounter(0);
		bootSequence = true;
	}
	else{ // Initialize the system registers with default values.
		// Timer registers
		(*rTIMA)  = 0x00;
		(*rTMA)   = 0x00;
		(*rTAC)   = 0x00;
		
		// Sound processor registers
		(*rNR10)  = 0x80;
		(*rNR11)  = 0xBF;
		(*rNR12)  = 0xFE;
		(*rNR14)  = 0xBF;
		(*rNR21)  = 0x3F;
		(*rNR22)  = 0x00;
		(*rNR24)  = 0xBF;
		(*rNR30)  = 0x7F;
		(*rNR31)  = 0xFF;
		(*rNR32)  = 0x9F;
		(*rNR33)  = 0xBF;
		(*rNR41)  = 0xFF;
		(*rNR42)  = 0x00;
		(*rNR43)  = 0x00;
		(*rNR44)  = 0xBF;
		(*rNR50)  = 0x77;
		(*rNR51)  = 0xF3;
		(*rNR52)  = 0xF1;

		// GPU registers
		(*rLCDC)  = 0x91;
		(*rSCY)   = 0x00;
		(*rSCX)   = 0x00;
		(*rLYC)   = 0x00;
		(*rBGP)   = 0xFC;
		(*rOBP0)  = 0xFF;
		(*rOBP1)  = 0xFF;
		(*rWY)    = 0x00;
		(*rWX)    = 0x00;

		// Interrupt enable
		(*rIE)    = 0x00;

		// Undocumented registers (for completeness)
		(*rFF6C)  = 0xFE;
		(*rFF72)  = 0x00;
		(*rFF73)  = 0x00;
		(*rFF74)  = 0x00;
		(*rFF75)  = 0x8F;
		(*rFF76)  = 0x00;
		(*rFF77)  = 0x00;

		// Set the PC to the entry point of the program. Skip the boot sequence.
		cpu.setProgramCounter(cart.getProgramEntryPoint());
		
		// Disable the boot sequence
		bootSequence = false;
	}
	return true;
}

bool SystemGBC::screenshot(){
	std::cout << " Not implemented\n";
	return false;
}

bool SystemGBC::quicksave(){
	std::cout << " Quicksaving... ";
	std::ofstream ofile("quick.sav", std::ios::binary);
	if(!ofile.good()){
		std::cout << "FAILED!\n";
		return false;
	}

	unsigned int nBytesWritten = 0;

	//cart.writeSavestate(ofile); // Write the ROM
	nBytesWritten += gpu.writeSavestate(ofile); // Write VRAM  (16 kB)
	nBytesWritten += cart.getRam()->writeSavestate(ofile); // Write cartridge RAM (if present)
	nBytesWritten += wram.writeSavestate(ofile); // Write Work RAM (8 kB)
	nBytesWritten += oam.writeSavestate(ofile); // Write Object Attribute Memory (OAM, 160 B)
	
	// Write system registers (128 B)
	for(unsigned int i = REGISTER_LOW; i < REGISTER_HIGH; i++){
		ofile.write((char*)&registers[i-REGISTER_LOW], 1);
		nBytesWritten++;
	}
		
	// Write High RAM (127 B)
	nBytesWritten += hram.writeSavestate(ofile);
	ofile.write((char*)&bGBCMODE, 1); // Gameboy Color mode flag
	ofile.write((char*)rIE, 1); // Interrupt enable
	ofile.write((char*)rIME, 1); // Master interrupt enable
	ofile.write((char*)&cpuStopped, 1); // STOP flag
	ofile.write((char*)&cpuHalted, 1); // HALT flag
	nBytesWritten += 5;
	
	nBytesWritten += cpu.writeSavestate(ofile); // CPU registers
	nBytesWritten += timer.writeSavestate(ofile); // System timer status
	nBytesWritten += clock.writeSavestate(ofile); // System clock status
	nBytesWritten += sound.writeSavestate(ofile); // Sound processor
	nBytesWritten += joy.writeSavestate(ofile); // Joypad controller

	ofile.close();
	std::cout << "DONE! Wrote " << nBytesWritten << " B\n";
	
	return true;
}

bool SystemGBC::quickload(){
	std::cout << " Loading quicksave... ";
	std::ifstream ifile("quick.sav", std::ios::binary);
	if(!ifile.good()){
		std::cout << "FAILED!\n";
		return false;
	}

	unsigned int nBytesRead = 0;

	//cart.readSavestate(ifile); // Write the ROM
	nBytesRead += gpu.readSavestate(ifile); // Write VRAM  (16 kB)
	nBytesRead += cart.getRam()->readSavestate(ifile); // Write cartridge RAM (if present)
	nBytesRead += wram.readSavestate(ifile); // Write Work RAM (8 kB)
	nBytesRead += oam.readSavestate(ifile); // Write Object Attribute Memory (OAM, 160 B)
	
	// Write system registers (128 B)
	for(unsigned int i = REGISTER_LOW; i < REGISTER_HIGH; i++){
		ifile.read((char*)&registers[i-REGISTER_LOW], 1);
		nBytesRead++;
	}
		
	// Write High RAM (127 B)
	nBytesRead += hram.readSavestate(ifile);
	ifile.read((char*)&bGBCMODE, 1); // Gameboy Color mode flag
	ifile.read((char*)rIE, 1); // Interrupt enable
	ifile.read((char*)rIME, 1); // Master interrupt enable
	ifile.read((char*)&cpuStopped, 1); // STOP flag
	ifile.read((char*)&cpuHalted, 1); // HALT flag
	nBytesRead += 5;
	
	nBytesRead += cpu.readSavestate(ifile); // CPU registers
	nBytesRead += timer.readSavestate(ifile); // System timer status
	nBytesRead += clock.readSavestate(ifile); // System clock status
	nBytesRead += sound.readSavestate(ifile); // Sound processor
	nBytesRead += joy.readSavestate(ifile); // Joypad controller

	ifile.close();
	std::cout << "DONE! Read " << nBytesRead << " B\n";
	
	return true;
}

void SystemGBC::help(){
	std::cout << "HELP: Press escape to exit program.\n\n";
	
	std::cout << " Button Map-\n";
	std::cout << "  Start = Enter\n";
	std::cout << " Select = Tab\n";
	std::cout << "      B = j\n";
	std::cout << "      A = k\n";
	std::cout << "     Up = w (up)\n";
	std::cout << "   Down = s (down)\n";
	std::cout << "   Left = a (left)\n";
	std::cout << "  Right = d (right)\n\n";

	std::cout << " System Keys-\n";
	std::cout << "  F1 : Display this help screen\n";
	std::cout << "  F2 : Pause emulation\n";
	std::cout << "  F3 : Resume emulation\n";
	std::cout << "  F4 : Take a screenshot\n";
	std::cout << "  F5 : Quicksave state\n";
	std::cout << "  F6 : Dump system memory to \"memory.dat\"\n";
	std::cout << "  F7 : Dump VRAM to \"vram.dat\"\n";
	std::cout << "  F8 : Dump cartridge RAM to \"sram.dat\"\n";
	std::cout << "  F9 : Quickload state\n";
	std::cout << "   - : Decrease frame skip\n";
	std::cout << "   + : Increase frame skip\n";
	std::cout << "   f : Show/hide FPS counter on screen\n";
}

void SystemGBC::stepThrough(){
	unpause();
	pauseAfterNextInstruction = true;
}

void SystemGBC::resumeUntilNextHBlank(){
	unpause();
	pauseAfterNextHBlank = true;
}

void SystemGBC::resumeUntilNextVBlank(){
	unpause();
	pauseAfterNextVBlank = true;
}

bool SystemGBC::writeRegister(const unsigned short &reg, const unsigned char &val){
	if(reg < REGISTER_LOW || reg > REGISTER_HIGH)
		return false;
	Register *ptr = &registers[reg - REGISTER_LOW];
	if(ptr->getSystemComponent()){ // Registers with an associated system component
		if(!ptr->getSystemComponent()->checkRegister(reg))
			return false;
		ptr->write(val); // Write the new register value.
		ptr->getSystemComponent()->writeRegister(reg, val);
	}
	else{ // Registers with no associated system component
		ptr->write(); // Write the new register value.
		switch(reg){
			case 0xFF0F: // IF (Interrupt Flag)
				break;
			case 0xFF4D: // KEY1 (Speed switch register)
				prepareSpeedSwitch = rKEY1->getBit(0); // Prepare to switch speed [0:No, 1:Yes]
				break;
			case 0xFF50: // Enable/disable ROM boot sequence
				bootSequence = false;
				if(forceColor) // Disable GBC mode so that original GB games display correctly after boot.
					bGBCMODE = false;
				break;
			case 0xFF56: // RP (Infrared comms port (not used))
				break;
			default:
				return false;
		}
	}
	return true;
}

bool SystemGBC::readRegister(const unsigned short &reg, unsigned char &val){
	if(reg < REGISTER_LOW || reg > REGISTER_HIGH)
		return false;
	Register *ptr = &registers[reg - REGISTER_LOW];
	val = ptr->read();
	if(ptr->getSystemComponent()){ // Registers with an associated system component
		ptr->getSystemComponent()->readRegister(reg, val);	
	}
	else{ // Registers with no associated system component
		switch(reg){
			case 0xFF0F: // IF (Interrupt Flag)
				break;
			case 0xFF4D: // KEY1 (Speed switch register)
				break;
			case 0xFF56: // RP (Infrared comms port (not used))
				break;
			default:
				return false;
		}
	}
	return true; // Read register
}

void SystemGBC::checkSystemKeys(){
	KeyStates *keys = gpu.getWindow()->getKeypress();
	if(keys->empty()) return;
	
	// Function keys
	if(keys->poll(0xF1)) // Help
		help();	
	else if(keys->poll(0xF2)) // Pause emulation
		pause();
	else if(keys->poll(0xF3)) // Resume emulation
		unpause();
	else if(keys->poll(0xF4)) // Screenshot
		screenshot();
	else if(keys->poll(0xF5)) // Quicksave
		quicksave();
	else if(keys->poll(0xF6)) // Dump memory
		dumpMemory("memory.dat");
	else if(keys->poll(0xF7)) // Dump VRAM
		dumpVRAM("vram.dat");
	else if(keys->poll(0xF8)) // Dump cartridge RAM
		dumpSRAM("sram.dat");
	else if(keys->poll(0xF9)) // Quickload
		quickload();
	else if(keys->poll(0x2D)) // '-'    Decrease frame skip
		frameSkip = (frameSkip > 1 ? frameSkip-1 : 1);
	else if(keys->poll(0x3D)) // '=(+)' Increase frame skip
		frameSkip++;
	else if(keys->poll(0x66)) // 'f' Display framerate
		displayFramerate = !displayFramerate;
}
