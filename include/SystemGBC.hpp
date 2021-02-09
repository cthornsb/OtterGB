#ifndef SYSTEM_GBC_HPP
#define SYSTEM_GBC_HPP

#include <vector>
#include <memory>
#include <map>

#include "SystemComponent.hpp"
#include "SystemRegisters.hpp"

#ifdef USE_QT_DEBUGGER
	class MainWindow;
#endif

class SoundManager;
class SerialController;
class DmaController;
class Cartridge;
class GPU;
class SoundProcessor;
class SpriteHandler;
class JoystickController;
class WorkRam;
class SystemClock;
class SystemTimer;
class LR35902;

class ComponentList{
public:
	SerialController   *serial; ///< Serial I/O controller
	DmaController      *dma;    ///< Direct memory access (DMA) controller
	Cartridge          *cart;   ///< Cartridge (ROM) controller
	GPU                *gpu;    ///< Pixel processing unit (PPU) controller
	SoundProcessor     *apu;    ///< Audio processing unit (APU) controller
	SpriteHandler      *oam;    ///< Object attribute memory (OAM) controller
	JoystickController *joy;    ///< Joypad input controller
	WorkRam            *wram;   ///< 32 kB work ram (WRAM) controller
	SystemComponent    *hram;   ///< 127 B high ram (HRAM) controller
	SystemClock        *sclk;   ///< System clock controller
	SystemTimer        *timer;  ///< System timer controller
	LR35902            *cpu;    ///< LR35902 CPU controller
	
	std::map<std::string, SystemComponent*> list; ///< Map of all named system components
	
	/** Default constructor
	  */
	ComponentList() :
		serial(0x0),
		dma(0x0),
		cart(0x0),
		gpu(0x0),
		apu(0x0),
		oam(0x0),
		joy(0x0),
		wram(0x0),
		hram(0x0),
		sclk(0x0),
		timer(0x0),
		cpu(0x0)
	{
	}
	
	/** Emulator system constructor
	  */ 
	ComponentList(SystemGBC* sys);
};

class Breakpoint{
public:
	/** Default constructor
	  */
	Breakpoint() : 
		d8(0), 
		d16(0), 
		d32(0), 
		enabled(0) 
	{ 
	}

	/** Enable 8-bit integer breakpoint
	  */
	void enable(const unsigned char &value){
		d8 = value; enabled |= 0x1;
	}

	/** Enable 16-bit integer breakpoint
	  */	
	void enable(const unsigned short &value){
		d16 = value; enabled |= 0x2;
	}
	
	/** Enable 32-bit integer breakpoint
	  */
	void enable(const unsigned int &value){
		d32 = value; enabled |= 0x4;
	}

	/** Disable breakpoint
	  */ 
	void clear(){
		enabled = 0;
	}
	
	/** Check input value against the 8-bit breakpoint (if enabled)
	  */
	bool check(const unsigned char &value) const {
		return ((enabled & 0x1) && value == d8);
	}
	
	/** Check input value against the 16-bit breakpoint (if enabled)
	  */
	bool check(const unsigned short &value) const {
		return ((enabled & 0x2) && value == d16);
	}
	
	/** Check input value against the 32-bit breakpoint (if enabled)
	  */
	bool check(const unsigned int &value) const {
		return ((enabled & 0x4) && value == d32);
	}
	
private:
	unsigned char d8; ///< 8-bit breakpoint
	
	unsigned short d16; ///< 16-bit breakpoint
	
	unsigned int d32; ///< 32-bit breakpoint
	
	unsigned char enabled; /** Enabled flag
	                         *  Bit 0: 8-bit breakpoint enabled
	                         *  Bit 1: 16-bit breakpoint enabled
	                         *  Bit 2: 32-bit breakpoint enabled
	                         */
};

class SystemGBC{
	friend class ComponentList;
	
public:
	/** Default constructor (deleted)
	  */
	SystemGBC() = delete;
	
	/** Command line arguments constructor
	  */
	SystemGBC(int &argc, char* argv[]);
	
	/** Destructor
	  * Currently does nothing
	  */
	~SystemGBC();

	/** Execute main emulation loop
	  * @return True if loop exits successfully and return false in the event of an error.
	  */
	void operator () (int) {
		execute();
	}

	/** Initialize all emulator system components and add a few remaining system registers
	  * May only be called once, otherwise does nothing.
	  */
	void initialize();

	/** Execute main emulation loop
	  * @return True if loop exits successfully and return false in the event of an error.
	  */
	bool execute();
	
	/** Attempt to write a value to system memory
	  * @param loc 16-bit system memory address
	  * @param src Pointer to value to write to the specified address
	  * @return True if the value was written successfully and return false otherwise
	  */
	bool write(const unsigned short &loc, const unsigned char* src){ 
		return write(loc, (*src)); 
	}
	
	/** Attempt to write a value to system memory
	  * @param loc 16-bit system memory address
	  * @param src Value to write to the specified address
	  * @return True if the value was written successfully and return false otherwise
	  */
	bool write(const unsigned short &loc, const unsigned char &src);

	/** Attempt to read a value from system memory
	  * @param loc 16-bit system memory address
	  * @param dest Pointer to value which specified address will be read into
	  * @return True if the value was read successfully and return false otherwise.
	  *         Upon failure to read value, destination value will not be modified.
	  */
	bool read(const unsigned short &loc, unsigned char* dest){ 
		return read(loc, (*dest)); 
	}
	
	/** Attempt to read a value from system memory
	  * @param loc 16-bit system memory address
	  * @param dest Value reference which specified address will be read into
	  * @return True if the value was read successfully and return false otherwise.
	  *         Upon failure to read value, destination value will not be modified.
	  */
	bool read(const unsigned short &loc, unsigned char &dest);

	/** Return the current system path to directory containing ROM files
	  */
	std::string getRomPath() const { 
		return romPath; 
	}
	
	/** Return the current ROM filename
	  */
	std::string getRomFilename() const { 
		return romFilename; 
	}

	/** Return the current ROM file extension
	  */
	std::string getRomExtension() const { 
		return romExtension; 
	}
	
	/** Return true if emulation is currently paused and return false otherwise
	  */
	bool getEmulationPaused() const { 
		return emulationPaused; 
	}
	
	/** Return true if debug mode is enabled and return false otherwise
	  */
	bool debugModeEnabled() const {
		return debugMode;
	}
	
	/** Attempt to read a byte from system memory and return the result
	  * If address is not readable, behavior is undefined.
	  * @param loc 16-bit system memory address
	  * @return The byte read from system memory 
	  */
	unsigned char getValue(const unsigned short &loc);
	
	/** Attempt to get a pointer to a byte in system memory
	  * @param loc 16-bit system memory address
	  * @return Pointer to the byte in system memory, or null in the event the address was not modifiable
	  */
	unsigned char* getPtr(const unsigned short &loc);
	
	/** Attempt to get a const pointer to a byte in system memory
	  * @param loc 16-bit system memory address
	  * @return Const pointer to the byte in system memory, or null in the event the address was not readable
	  */
	const unsigned char* getConstPtr(const unsigned short &loc);
	
	/** Get pointer to a system register
	  * @param reg Register memory address in range [0xff00, 0xff80]
	  * @return Pointer to register or return null if specified address was outside of the range of registers
	  */
	Register* getPtrToRegister(const unsigned short &reg);

	/** Get pointer to the value of a system register
	  * @param reg Register memory address in range [0xff00, 0xff80]
	  * @return Pointer to register value or return null if specified address was outside of the range of registers
	  */
	unsigned char* getPtrToRegisterValue(const unsigned short &reg);

	/** Get const pointer to the value of a system register
	  * @param reg Register memory address in range [0xff00, 0xff80]
	  * @return Pointer to register value or return null if specified address was outside of the range of registers
	  */
	const unsigned char* getConstPtrToRegisterValue(const unsigned short &reg);

	/** Get pointer to the LR35902 CPU emulator
	  */
	LR35902* getCPU(){
		return cpu.get();
	}
	
	/** Get pointer to the pixel processing unit (PPU)
	  */
	GPU* getGPU(){
		return gpu.get();
	}
	
	/** Get pointer to the audio processing unit (APU)
	  */
	SoundProcessor* getSound(){
		return sound.get();
	}
	
	/** Get pointer to the object attribute memory (OAM) handler
	  */
	SpriteHandler* getOAM(){
		return oam.get();
	}
	
	/** Get pointer to the system clock
	  */
	SystemClock* getClock() {
		return sclk.get();
	}

	/** Get pointer to the cartridge (ROM) handler
	  */	
	Cartridge* getCartridge(){
		return cart.get();
	}
	
	/** Get pointer to the work ram (WRAM) handler
	  */
	WorkRam* getWRAM(){
		return wram.get();
	}
	
	/** Get pointer to the list of system components
	  */
	ComponentList* getListOfComponents(){
		return subsystems.get();
	}
	
	/** Get pointer to the vector of all system registers
	  */
	std::vector<Register>* getRegisters(){
		return &registers;
	}
	
	/** Search for a system register by name and return a pointer to it if found, otherwise return null
	  * @param name Human-readable name of system register (not case-sensitive)
	  * @return A pointer to a system register if a match is found, otherwise null is returned
	  */
	Register* getRegisterByName(const std::string& name);

	/** Return true if the cpu is currently in a halted state (HALT)
	  */	
	bool cpuIsHalted() const {
		return cpuHalted;
	}
	
	/** Return true if the cpu is currently in a stopped state (STOP 0)
	  */
	bool cpuIsStopped() const {
		return cpuStopped;
	}
	
	/** Toggle CPU debug flag (disabled by default)
	  */
	void setDebugMode(bool state=true);

	/** Toggle on-screen framerate display (disabled by default)
	  */
	void setDisplayFramerate(bool state=true){
		displayFramerate = state;
	}

	/** Toggle verbosity flag (disabled by default)
	  */
	void setVerboseMode(bool state=true);
	
	/** Force CGB features for original DMG games (disabled by default)
	  * Equivalent to playing a DMG game on a CGB.
	  */
	void setForceColorMode(bool state=true){
		forceColor = state;
	}

	/** Set system memory write monitor region to the inclusive range [locL, locH]
	  */	
	void setMemoryWriteRegion(const unsigned short &locL, const unsigned short &locH=0);
	
	/** Set system memory read monitor region to the inclusive range [locL, locH]
	  */
	void setMemoryReadRegion(const unsigned short &locL, const unsigned short &locH=0);
	
	/** Set the number of concurrent frames to render (default is 1 i.e. no frames skipped)
	  * Larger frame skip values will yield higher frame-rates at the cost of decreased smoothness.
	  */
	void setFrameSkip(const unsigned short &frames){
		frameSkip = frames;
	}

	/** Set the system path directory to use for loading ROM files
	  */
	void setRomPath(const std::string &path){
		romPath = path;
	}

	/** Set program counter break-point
	  * Execution will pause when the specified point in the program is reached.
	  */
	void setBreakpoint(const unsigned short &pc);
	
	/** Set system memory write break-point
	  * Execution will pause when the specified address is accessed for writing.
	  */
	void setMemWriteBreakpoint(const unsigned short &addr);
	
	/** Set system memory read break-point
	  * Execution will pause when the specified address is accessed for reading.
	  */
	void setMemReadBreakpoint(const unsigned short &addr);
	
	/** Set opcode break-point
	  * Execution will pause when the specified opcode is reached.
	  * @param op Opcode to break on
	  * @param cb Flag specifying op is a CB-prefix opcode (defaults to false)
	  */
	void setOpcodeBreakpoint(const unsigned char &op, bool cb=false);

	/** Set the pointer to the external audio interface manager
	  */
	void setAudioInterface(SoundManager* ptr);

#ifdef USE_QT_DEBUGGER
	/** Set the pointer to the external Qt debugger window and connect it to emulator system
	  */
	void setQtDebugger(MainWindow* ptr);
#endif

	/** Set target emulation framerate
	  */
	void setFramerateMultiplier(const float& freq);

	void clearBreakpoint();
	
	void clearMemWriteBreakpoint();
	
	void clearMemReadBreakpoint();
	
	void clearOpcodeBreakpoint();

	void addSystemRegister(SystemComponent* comp, const unsigned char &reg, Register* &ptr, const std::string &name, const std::string &bits);

	void addSystemRegister(const unsigned char &reg, Register* &ptr, const std::string &name, const std::string &bits);
	
	void addDummyRegister(SystemComponent* comp, const unsigned char &reg);

	void clearRegister(const unsigned char &reg);
	
	bool dumpMemory(const std::string &fname);
	
	bool dumpVRAM(const std::string &fname);
	
	bool saveSRAM(const std::string &fname);
	
	bool loadSRAM(const std::string &fname);
	
	void handleHBlankPeriod();
	
	void handleVBlankInterrupt();
	
	void handleLcdInterrupt();
	
	void handleTimerInterrupt();
	
	void handleSerialInterrupt();
	
	void handleJoypadInterrupt();

	void enableInterrupts();
	
	void disableInterrupts();

	void haltCPU(){
		cpuHalted = true;
	}
	
	void stopCPU(){
		cpuStopped = true;
	}
	
	void resumeCPU();
	
#ifdef USE_QT_DEBUGGER
	/** Update debugger gui and process Qt events
	  */
	void updateDebugger();
#endif
	
	void pause();
	
	void unpause();

	bool reset();

	bool screenshot();

	bool quicksave(const std::string& fname="");
	
	bool quickload(const std::string& fname="");
	
	bool writeExternalRam();

	bool readExternalRam();
	
	void help();
	
	void openDebugConsole();
	
	void closeDebugConsole();
	
	void quit(){
		userQuitting = true;
	}

	/** Resume emulation until the next instruction is executed.
	  */	
	void stepThrough();
	
	/** Resume emulation until the next clock cycle.
	  */	
	void advanceClock();
	
	/** Resume emulation until the next HBlank period.
	  */
	void resumeUntilNextHBlank();
	
	/** Resume emulation until the next VBlank period.
	  */
	void resumeUntilNextVBlank();
	
private:
	SystemComponent dummyComponent; ///< Dummy system component used to organize system registers

	unsigned short nFrames;
	unsigned short frameSkip;

	bool verboseMode; ///< Verbosity flag
	bool debugMode; ///< Debug flag

	bool cpuStopped; ///< 
	bool cpuHalted; ///<
	bool emulationPaused; ///<
	bool bootSequence; ///< The gameboy boot ROM is still running
	bool forceColor; 
	bool displayFramerate;
	bool userQuitting;
	bool autoLoadExtRam; ///< If set, external cartridge RAM (SRAM) will not be loaded at boot
	bool initSuccessful; ///< Set if all components were initialized successfully
	bool fatalError;
	bool consoleIsOpen;

	unsigned char dmaSourceH; ///< DMA source MSB
	unsigned char dmaSourceL; ///< DMA source LSB
	unsigned char dmaDestinationH; ///< DMA destination MSB
	unsigned char dmaDestinationL; ///< DMA destination LSB

	std::string romPath; ///< Full path to input ROM file
	std::string romFilename; ///< Input ROM filename (with no path or extension)
	std::string romExtension; ///< Input ROM file extension

	bool pauseAfterNextInstruction;
	bool pauseAfterNextClock;
	bool pauseAfterNextHBlank;
	bool pauseAfterNextVBlank;

	SoundManager* audioInterface; ///< Pointer to sound output interface

	std::unique_ptr<SerialController> serial;
	std::unique_ptr<DmaController> dma;
	std::unique_ptr<Cartridge> cart;
	std::unique_ptr<GPU> gpu;
	std::unique_ptr<SoundProcessor> sound;
	std::unique_ptr<SpriteHandler> oam;
	std::unique_ptr<JoystickController> joy;
	std::unique_ptr<WorkRam> wram; // 8 4kB banks of RAM
	std::unique_ptr<SystemComponent> hram; // 127 bytes of high RAM
	std::unique_ptr<SystemClock> sclk;
	std::unique_ptr<SystemTimer> timer;
	std::unique_ptr<LR35902> cpu;
	
#ifdef USE_QT_DEBUGGER
	MainWindow* gui; ///< Pointer to Qt gui debugger (if available)
#endif

	Breakpoint breakpointProgramCounter;
	Breakpoint breakpointMemoryWrite;
	Breakpoint breakpointMemoryRead;
	Breakpoint breakpointOpcode;

	unsigned short memoryAccessWrite[2]; ///< User-set memory 
	unsigned short memoryAccessRead[2]; ///< 

	std::vector<Register> registers; ///< System control registers
	
	unsigned short bootLength; ///< Size of the boot ROM
	
	std::vector<unsigned char> bootROM; ///< Variable length gameboy/gameboy color boot ROM

	std::unique_ptr<ComponentList> subsystems;

	bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	bool readRegister(const unsigned short &reg, unsigned char &val);
	
	void checkSystemKeys();
};

#endif
