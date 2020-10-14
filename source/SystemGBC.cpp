
#include <unistd.h>
#include <iostream>

#include "Support.hpp"
#include "SystemGBC.hpp"
#include "SystemRegisters.hpp"

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

/** INTERRUPTS:
	0x40 - VBlank interrupt (triggered at beginning of VBlank period [~59.7 Hz])
	0x48 - LCDC status interrupt
	0x50 - Timer interrupt (triggered every time the timer overflows)
	0x58 - Serial interrupt (triggered after sending/receiving 8 bits)
	0x60 - Joypad interrupt (triggered any time a joypad button is pressed)
**/

SystemGBC::SystemGBC() : nFrames(0), frameSkip(1), verboseMode(false), debugMode(false), masterInterruptEnable(0x1), interruptEnable(0), 
                         dmaSourceH(0), dmaSourceL(0), dmaDestinationH(0), dmaDestinationL(0) { 
	// Disable memory reagion monitor
	memoryAccessWrite[0] = 1; 
	memoryAccessWrite[1] = 0;
	memoryAccessRead[0] = 1; 
	memoryAccessRead[1] = 0;
}

bool SystemGBC::initialize(const std::string &fname){ 
	hram.initialize(127);

	// Set pointers to joypad registers
	rJOYP = getPtrToRegister(0xFF00); // JOYP (Joypad register)

	// Set pointers to serial data registers
	rSB   = getPtrToRegister(0xFF01); // SB (Serial transfer data byte)
	rSC   = getPtrToRegister(0xFF02); // SC (Serial transfer control)

	// Set pointers to timer registers
	rDIV  = getPtrToRegister(0xFF04); // DIV (Divider register)
	rTIMA = getPtrToRegister(0xFF05); // TIMA (Timer counter)
	rTMA  = getPtrToRegister(0xFF06); // TMA (Timer modulo)
	rTAC  = getPtrToRegister(0xFF07); // TAC (Timer control)
	
	// Set pointers to sound processor registers
	rNR10 = getPtrToRegister(0xFF10); // NR10 ([TONE] Channel 1 sweep register)
	rNR11 = getPtrToRegister(0xFF11); // NR11 ([TONE] Channel 1 sound length / wave pattern duty)
	rNR12 = getPtrToRegister(0xFF12); // NR12 ([TONE] Channel 1 volume envelope)
	rNR13 = getPtrToRegister(0xFF13); // NR13 ([TONE] Channel 1 frequency low)
	rNR14 = getPtrToRegister(0xFF14); // NR14 ([TONE] Channel 1 frequency high)
	rNR21 = getPtrToRegister(0xFF16); // NR21 ([TONE] Channel 2 sound length / wave pattern duty)
	rNR22 = getPtrToRegister(0xFF17); // NR22 ([TONE] Channel 2 volume envelope
	rNR23 = getPtrToRegister(0xFF18); // NR23 ([TONE] Channel 2 frequency low)
	rNR24 = getPtrToRegister(0xFF19); // NR24 ([TONE] Channel 2 frequency high)
	rNR30 = getPtrToRegister(0xFF1A); // NR30 ([TONE] Channel 3 sound on/off)
	rNR31 = getPtrToRegister(0xFF1B); // NR31 ([WAVE] Channel 3 sound length)
	rNR32 = getPtrToRegister(0xFF1C); // NR32 ([WAVE] Channel 3 select output level)
	rNR33 = getPtrToRegister(0xFF1D); // NR33 ([WAVE] Channel 3 frequency low)
	rNR34 = getPtrToRegister(0xFF1E); // NR34 ([WAVE] Channel 3 frequency high)
	rNR41 = getPtrToRegister(0xFF20); // NR41 ([NOISE] Channel 4 sound length)
	rNR42 = getPtrToRegister(0xFF21); // NR42 ([NOISE] Channel 4 volume envelope)
	rNR43 = getPtrToRegister(0xFF22); // NR43 ([NOISE] Channel 4 polynomial counter)
	rNR44 = getPtrToRegister(0xFF23); // NR44 ([NOISE] Channel 4 counter / consecutive, initial)
	rNR50 = getPtrToRegister(0xFF24); // NR50 (Channel control / ON-OFF / volume)
	rNR51 = getPtrToRegister(0xFF25); // NR51 (Select sound output)
	rNR52 = getPtrToRegister(0xFF26); // NR52 (Sound ON-OFF)
	rWAVE = getPtrToRegister(0xFF30); // [Wave] pattern RAM (FF30-FF3F)

	// Set pointers to GPU registers	
	rLCDC = getPtrToRegister(0xFF40); // LCDC (LCD Control Register)
	rSTAT = getPtrToRegister(0xFF41); // STAT (LCDC Status Register)
	rSCY  = getPtrToRegister(0xFF42); // SCY (Scroll Y)
	rSCX  = getPtrToRegister(0xFF43); // SCX (Scroll X)
	rLY   = getPtrToRegister(0xFF44); // LY (LCDC Y-coordinate) [read-only]
	rLYC  = getPtrToRegister(0xFF45); // LYC (LY Compare)
	rDMA  = getPtrToRegister(0xFF46); // DMA transfer from ROM/RAM to OAM
	rBGP  = getPtrToRegister(0xFF47); // BGP (BG palette data, non-gbc mode only)
	rOBP0 = getPtrToRegister(0xFF48); // OBP0 (Object palette 0 data, non-gbc mode only)
	rOBP1 = getPtrToRegister(0xFF49); // OBP1 (Object palette 1 data, non-gbc mode only)
	rWY   = getPtrToRegister(0xFF4A); // WY (Window Y Position)
	rWX   = getPtrToRegister(0xFF4B); // WX (Window X Position (minus 7))
	rVBK  = getPtrToRegister(0xFF4F); // VBK (VRAM bank select, gbc mode)
	rBGPI = getPtrToRegister(0xFF68); // BCPS/BGPI (Background palette index, gbc mode)
	rBGPD = getPtrToRegister(0xFF69); // BCPD/BGPD (Background palette data, gbc mode)
	rOBPI = getPtrToRegister(0xFF6A); // OCPS/OBPI (Sprite palette index, gbc mode)
	rOBPD = getPtrToRegister(0xFF6B); // OCPD/OBPD (Sprite palette index, gbc mode)
	
	// Set pointers to system registers
	rIF    = getPtrToRegister(0xFF0F); // Interrupt Flag
	rDMA   = getPtrToRegister(0xFF46); // DMA transfer from ROM/RAM to OAM
	rKEY1  = getPtrToRegister(0xFF4D); // Speed switch register
	rHDMA1 = getPtrToRegister(0xFF51); // New DMA source, high byte (GBC only)
	rHDMA2 = getPtrToRegister(0xFF52); // New DMA source, low byte (GBC only)
	rHDMA3 = getPtrToRegister(0xFF53); // New DMA destination, high byte (GBC only)
	rHDMA4 = getPtrToRegister(0xFF54); // New DMA destination, low byte (GBC only)
	rHDMA5 = getPtrToRegister(0xFF55); // New DMA source, length/mode/start (GBC only)
	rRP    = getPtrToRegister(0xFF56); // Infrared comms port (not used)
	rSVBK  = getPtrToRegister(0xFF70); // WRAM bank select
	rIE    = &interruptEnable;       // Interrupt enable (ffff)
	rIME   = &masterInterruptEnable; // Master interrupt enable

	// Set pointers to undocumented registers
	rFF6C = getPtrToRegister(0xFF6C);
	rFF72 = getPtrToRegister(0xFF72);
	rFF73 = getPtrToRegister(0xFF73);
	rFF74 = getPtrToRegister(0xFF74);
	rFF75 = getPtrToRegister(0xFF75);
	rFF76 = getPtrToRegister(0xFF76);
	rFF77 = getPtrToRegister(0xFF77);

	// Set memory offsets for all components	
	gpu.setOffset(VRAM_SWAP_START);
	cart.getRam()->setOffset(CART_RAM_START);
	wram.setOffset(WRAM_ZERO_START);
	oam.setOffset(OAM_TABLE_START);
	joy.setOffset(IO_PORTS_START);
	hram.setOffset(HIGH_RAM_START);

	// Connect system components to the system bus.
	cpu.connectSystemBus(this);
	timer.connectSystemBus(this);
	clock.connectSystemBus(this);
	hram.connectSystemBus(this);
	wram.connectSystemBus(this);
	joy.connectSystemBus(this);
	oam.connectSystemBus(this);
	sound.connectSystemBus(this);
	gpu.connectSystemBus(this);
	cart.connectSystemBus(this);
	
	// Must connect the system bus BEFORE initializing the CPU.
	cpu.initialize();
	
	// Disable the system timer.
	timer.disable();
	
	// Initialize the system registers with default values.
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
	
	// Read the ROM into memory
	bool retval = cart.readRom(fname, verboseMode);
	if(retval) // Initialize the SDL window
		gpu.initialize();
	
	return retval;
}

bool SystemGBC::execute(){
	// Set the PC to the entry point of the program.
	cpu.setProgramCounter(cart.getProgramEntryPoint());

	// Run the ROM. Main loop.
	unsigned short nCycles = 1;
	while(true){
		// Check the status of the GPU and LCD screen
		if(!gpu.getWindowStatus())
			break;

		// Check for pending interrupts.
		if(masterInterruptEnable && (*rIF) != 0x0){
			if(((*rIF) & 0x1) != 0) // VBlank
				acknowledgeVBlankInterrupt();
			if(((*rIF) & 0x2) != 0) // LCDC STAT
				acknowledgeLcdInterrupt();
			if(((*rIF) & 0x4) != 0) // Timer
				acknowledgeTimerInterrupt();
			if(((*rIF) & 0x8) != 0) // Serial
				acknowledgeSerialInterrupt();
			if(((*rIF) & 0x10) != 0) // Joypad
				acknowledgeJoypadInterrupt();
		}
			
		// Perform one instruction.
		if((nCycles = cpu.execute(&cart)) == 0)
			break;

		// Update system timer.
		timer.onClockUpdate(nCycles);

		// Update sound processor.
		sound.onClockUpdate(nCycles);

		// Sync with the GBC system clock.
		// Wait a certain number of cycles based on the opcode executed		
		if(clock.sync(nCycles))
			if(nFrames++ % frameSkip == 0)
				gpu.render(); // Render the current frame
	}
}

void SystemGBC::handleHBlankPeriod(){
	if(nFrames % frameSkip == 0)
		gpu.drawNextScanline(&oam);
}
	
void SystemGBC::handleVBlankInterrupt(){ (*rIF) |= 0x1; }

void SystemGBC::handleLcdInterrupt(){ (*rIF) |= 0x2; }

void SystemGBC::handleTimerInterrupt(){ (*rIF) |= 0x4; }

void SystemGBC::handleSerialInterrupt(){ (*rIF) |= 0x8; }

void SystemGBC::handleJoypadInterrupt(){ (*rIF) |= 0x10; }

bool SystemGBC::write(const unsigned short &loc, const unsigned char &src){
	// Check for system registers
	if(loc >= REGISTER_LOW && loc < REGISTER_HIGH){
		// Write the register
		bool prepareSpeedSwitch;
		unsigned char wramBank;
		unsigned short srcStart, destStart;
		unsigned short transferLength;
		bool transferMode;
		if(!writeRegister(loc, src)){
			switch(loc){
				case 0xFF0F: // IF (Interrupt Flag)
					(*rIF) = src;
					break;
				case 0xFF46: // DMA transfer from ROM/RAM to OAM
					// Source:      XX00-XX9F with XX in range [00,F1]
					// Destination: FE00-FE9F
					// DMA transfer takes 160 us (80 us in double speed) and CPU may only access HRAM during this interval
					(*rDMA) = src;
					srcStart = ((src & 0x00F1) << 8);
					startDmaTransfer(0xFE00, srcStart, 160);
					//clock.wait(160);
					break;
				case 0xFF4D: // KEY1 (Speed switch register)
					(*rKEY1) = src;
					//currentSpeed = (src & 0x80) != 0; // Current speed [0:normal, 1:double] (read-only)
					prepareSpeedSwitch = (src & 0x1) != 0; // Prepare to switch speed [0:No, 1:Yes]
					// Technincally we should wait for a STOP command, but we'll
					// go ahead and do it now for simplicity. Also, this should
					// toggle the clock speed, not just double it.
					clock.setFrequencyMultiplier(2.0);
					break;
				case 0xFF51: // HDMA1 - new DMA source, high byte (GBC only)
					(*rHDMA1) = src;
					dmaSourceH = src;
					break;
				case 0xFF52: // HDMA2 - new DMA source, low byte (GBC only)
					(*rHDMA2) = src;
					dmaSourceL = (src & 0xF0); // Bits (4-7) of LSB of source address
					break;
				case 0xFF53: // HDMA3 - new DMA destination, high byte (GBC only)
					(*rHDMA3) = src;
					dmaDestinationH = (src & 0x1F); // Bits (0-4) of MSB of destination address
					break;
				case 0xFF54: // HDMA4 - new DMA destination, low byte (GBC only)
					(*rHDMA4) = src;
					dmaDestinationL = (src & 0xF0); // Bits (4-7) of LSB of destination address
					break;
				case 0xFF55: // HDMA5 - new DMA source, length/mode/start (GBC only)
					// Start a VRAM DMA transfer
					(*rHDMA5) = src;
					srcStart = ((dmaSourceH & 0x00FF) << 8) + dmaSourceL;
					destStart = ((dmaDestinationH & 0x00FF) << 8) + dmaDestinationL;
					transferLength = ((src & 0x7F) * + 1) * 0x10; // Specifies the number of bytes to transfer
					transferMode = (src & 0x80) != 0; // 0: General DMA, 1: H-Blank DMA
					// source:      0000-7FF0 or A000-DFF0
					// destination: 8000-9FF0 (VRAM)
					// DMA takes ~1 us per two bytes transferred
					if(((srcStart >= 0x000 && srcStart <= 0x7FF0) || (srcStart >= 0xA000 && srcStart <= 0xDFF0)) && (destStart >= 0x8000 && destStart <= 0x9FF0)){
						startDmaTransfer(destStart, srcStart, transferLength);		
						//clock.wait(transferLength/2);
					}
					break;
				case 0xFF56: // RP (Infrared comms port (not used))
					(*rRP) = src;
					break;
				case 0xFF70: // SVBK (WRAM bank select)
					(*rSVBK) = src;
					wramBank = (src & 0x7); // Select WRAM bank (1-7)
					if(wramBank == 0x0) wramBank = 0x1; // Select bank 1 instead
					wram.setBank(wramBank);
					break;
				default:
					return false;
			}
		}
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
			case 0xFFFF: // Master interrupt enable
				interruptEnable = src;
				break;
			default:
				return false;
		}
	}

	// Check for memory access watch
	if(loc >= memoryAccessWrite[0] && loc <= memoryAccessWrite[1]){
		std::cout << " (W) PC=" << getHex((unsigned short)(cpu.getProgramCounter()-cpu.getLength())) << " " << getHex(src) << "->[" << getHex(loc) << "] ";
		if(cpu.getLength() == 2)
			std::cout << "d8=" << getHex(cpu.getd8());
		else if(cpu.getLength() == 3)
			std::cout << "d16=" << getHex(cpu.getd16());
		std::cout << std::endl;
	}
	
	return true; // Successfully wrote to memory location (loc)
}

bool SystemGBC::read(const unsigned short &loc, unsigned char &dest){
	// Check for system registers
	if(loc >= REGISTER_LOW && loc < REGISTER_HIGH){
		// Read the register
		if(!readRegister(loc, dest)){
			switch(loc){
				case 0xFF0F: // IF (Interrupt Flag)
					dest = (*rIF);
					break;
				case 0xFF46: // DMA transfer from ROM/RAM to OAM
					dest = (*rDMA);
					break;
				case 0xFF4D: // Speed switch register
					dest = (*rKEY1);
					break;
				case 0xFF51: // HDMA1 (New DMA source, high byte (GBC only))
					dest = (*rHDMA1);
					break;
				case 0xFF52: // HDMA2 (New DMA source, low byte (GBC only))
					dest = (*rHDMA2);
					break;
				case 0xFF53: // HDMA3 (New DMA destination, high byte (GBC only))
					dest = (*rHDMA3);
					break;
				case 0xFF54: // HDMA4 (New DMA destination, low byte (GBC only))
					dest = (*rHDMA4);
					break;
				case 0xFF55: // HDMA5 (New DMA source, length/mode/start (GBC only))
					dest = (*rHDMA5);
					break;
				case 0xFF56: // RP (Infrared comms port (not used))
					dest = (*rRP);
					break;
				case 0xFF70: // WRAM bank select
					dest = (*rSVBK);
					break;
				default:
					return false;
			}
		}
	}
	else{ // Attempt to read from memory
		switch(loc){
			case 0x0000 ... 0x7FFF: // Cartridge ROM 
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
			case 0xFFFF: // Master interrupt enable
				dest = interruptEnable;
				break;
			default:
				return false;
		}
	}

	// Check for memory access watch
	if(loc >= memoryAccessRead[0] && loc <= memoryAccessRead[1])
		std::cout << " (R) PC=" << getHex((unsigned short)(cpu.getProgramCounter()-cpu.getLength())) << " [" << getHex(loc) << "] dest=" << getHex(dest) << "\n";
	
	return true; // Successfully read from memory location (loc)
}

unsigned char SystemGBC::getValue(const unsigned short &loc){
	unsigned char retval;
	read(loc, retval);
	return retval;
}

unsigned char *SystemGBC::getPtr(const unsigned short &loc){
	unsigned char *retval;
	read(loc, retval);
	return retval;
}

unsigned char *SystemGBC::getPtrToRegister(const unsigned short &reg){
	if(reg < REGISTER_LOW || reg >= REGISTER_HIGH)
		return 0x0;
	return &registers[(reg & 0x00FF)];
}

void SystemGBC::setDebugMode(bool state/*=true*/){
	debugMode = state;
	cpu.setDebugMode(state);
	timer.setDebugMode(state);
	clock.setDebugMode(state);
	hram.setDebugMode(state);
	wram.setDebugMode(state);
	joy.setDebugMode(state);
	oam.setDebugMode(state);
	sound.setDebugMode(state);
	gpu.setDebugMode(state);
}

// Toggle framerate output
void SystemGBC::setDisplayFramerate(bool state/*=true*/){
	clock.setDisplayFramerate(state);
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

bool SystemGBC::dumpMemory(const char *fname){
	std::ofstream ofile(fname, std::ios::binary);
	if(!ofile.good())
		return false;
		
	unsigned char byte;
	for(unsigned int i = 0; i <= 0xFFFF; i++){
		// Read a byte from memory.
		if(!read((unsigned short)i, byte)){
			std::cout << " WARING! Failed to read memory location " << getHex((unsigned short)i) << "!\n";
			byte = 0x00; // Write an empty byte
		}
		ofile.write((char*)&byte, 1);
	}
	ofile.close();

	return true;
}

void SystemGBC::startDmaTransfer(const unsigned short &dest, const unsigned short &src, const unsigned short &N){
	unsigned char byte;
	for(unsigned short i = 0; i < N; i++){
		// Read a byte from memory.
		this->read(src+i, byte);
		// Write it to a different location.
		this->write(dest+i, byte);
	}
}

bool SystemGBC::writeRegister(const unsigned short &reg, const unsigned char &val){
	switch(reg){
		case 0xFF00: // Joypad
			return joy.writeRegister(reg, val);
		case 0xFF04 ... 0xFF07: // SystemTimer
			return timer.writeRegister(reg, val);
		case 0xFF10 ... 0xFF30: // SoundProcessor
			return sound.writeRegister(reg, val);
		case 0xFF40 ... 0xFF6B: // GPU
			return gpu.writeRegister(reg, val);
		default:
			break;
	}
	
	return false; // Failed to write register
}

bool SystemGBC::readRegister(const unsigned short &reg, unsigned char &val){
	switch(reg){
		case 0xFF00: // Joypad
			return joy.readRegister(reg, val);
		case 0xFF04 ... 0xFF07: // SystemTimer
			return timer.readRegister(reg, val);
		case 0xFF10 ... 0xFF30: // SoundProcessor
			return sound.readRegister(reg, val);
		case 0xFF40 ... 0xFF6B: // GPU
			return gpu.readRegister(reg, val);
		default:
			break;
	}
	
	return false; // Failed to read register
}

void SystemGBC::acknowledgeVBlankInterrupt(){
	(*rIF) &= 0xFE;
	if((interruptEnable & 0x1) != 0){ // Execute interrupt
		masterInterruptEnable = 0;
		cpu.callInterruptVector(0x40);
	}
}

void SystemGBC::acknowledgeLcdInterrupt(){
	(*rIF) &= 0xFD;
	if((interruptEnable & 0x2) != 0){ // Execute interrupt
		masterInterruptEnable = 0;
		cpu.callInterruptVector(0x48);
	}
}

void SystemGBC::acknowledgeTimerInterrupt(){
	(*rIF) &= 0xFB;
	if((interruptEnable & 0x4) != 0){ // Execute interrupt
		std::cout << " HERE!\n";
		masterInterruptEnable = 0;
		cpu.callInterruptVector(0x50);
	}
}

void SystemGBC::acknowledgeSerialInterrupt(){
	(*rIF) &= 0xF7;
	if((interruptEnable & 0x8) != 0){ // Execute interrupt
		masterInterruptEnable = 0;
		cpu.callInterruptVector(0x58);
	}
}

void SystemGBC::acknowledgeJoypadInterrupt(){
	(*rIF) &= 0xEF;
	if((interruptEnable & 0x10) != 0){ // Execute interrupt
		std::cout << " JOYP\n";
		masterInterruptEnable = 0;
		cpu.callInterruptVector(0x60);
	}
}
