#ifndef SYSTEM_GBC_HPP
#define SYSTEM_GBC_HPP

#include <vector>
#include <memory>
#include <map>

#include "SystemComponent.hpp"
#include "SystemRegisters.hpp"
#include "ComponentThread.hpp"

#ifdef USE_QT_DEBUGGER
	class MainWindow;
#endif

struct BackgroundWindowSettings {
	unsigned char nscx;
	unsigned char nscy;
	unsigned char nwx;
	unsigned char nwy;
};

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
class OTTWindow;

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

class SystemGBC : public ThreadObject {
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
	
	/** Return the current ROM directory
	  */
	std::string getRomDirectory() const {
		return romDirectory;
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
	
	/** Get the OtterGB version number string
	  */
	static std::string getVersionString();
	
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
	
	/** Get pointer to the object attribute memory (OAM) controller
	  */
	SpriteHandler* getOAM(){
		return oam.get();
	}
	
	/** Get pointer to the system clock
	  */
	SystemClock* getClock() {
		return sclk.get();
	}

	/** Get pointer to the cartridge (ROM) controller
	  */	
	Cartridge* getCartridge(){
		return cart.get();
	}
	
	/** Get pointer to the work ram (WRAM) controller
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

	/** Return true if the CPU is currently in a halted state (HALT)
	  */	
	bool cpuIsHalted() const {
		return cpuHalted;
	}
	
	/** Return true if the CPU is currently in a stopped state (STOP 0)
	  */
	bool cpuIsStopped() const {
		return cpuStopped;
	}
	
	/** Toggle debug flag for all system components (disabled by default)
	  */
	void setDebugMode(bool state=true);

	/** Toggle on-screen framerate display (disabled by default)
	  */
	void setDisplayFramerate(bool state=true){
		displayFramerate = state;
	}

	/** Toggle verbosity flag for all system components (disabled by default)
	  */
	void setVerboseMode(bool state=true);
	
	/** Force CGB features for original DMG games (disabled by default)
	  * Equivalent to playing a DMG game on a CGB. Has no effect for CGB games.
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

	/** Set the explicit input ROM filename
	  */
	void setRomPath(const std::string &path){
		romPath = path;
		bNeedsReloaded = true;
	}
	
	/** Set the system path directory to use for loading ROM files
	  */
	void setRomDirectory(const std::string& path){
		romDirectory = path;
	}
	
	/** Set the input ROM filename (excluding the ROM directory)
	  * Not to be confused with setRomPath() which explicitly sets the ROM path.
	  */
	void setRomFilename(const std::string& fname);

	/** Set program counter breakpoint
	  * Execution will pause when the specified point in the program is reached.
	  */
	void setBreakpoint(const unsigned short &pc);
	
	/** Set system memory write access breakpoint
	  * Execution will pause when the specified address is accessed for writing.
	  */
	void setMemWriteBreakpoint(const unsigned short &addr);
	
	/** Set system memory read access breakpoint
	  * Execution will pause when the specified address is accessed for reading.
	  */
	void setMemReadBreakpoint(const unsigned short &addr);
	
	/** Set LR35902 opcode breakpoint
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

	/** Open tile viewer debugger window
	  */
	void openTileViewer();

	/** Open draw layer viewer debugger window
	  */
	void openLayerViewer();

	/** Set target emulation framerate
	  */
	void setFramerateMultiplier(const float& freq);

	/** Clear the current program counter breakpoint
	  */
	void clearBreakpoint();
	
	/** Clear the current memory write access breakpoint
	  */
	void clearMemWriteBreakpoint();
	
	/** Clear the current memory read access breakpoint
	  */
	void clearMemReadBreakpoint();
	
	/** Clear the current LR35902 opcode breakpoint
	  */
	void clearOpcodeBreakpoint();

	/** Add a system register
	  * @param comp Associated system component
	  * @param reg Register index (as addr = 0xff00 + reg)
	  * @param ptr Pointer reference where new register pointer will be returned
	  * @param name Human readable name of register
	  * @param bits Read / write bit access string (see Register class)
	  */
	void addSystemRegister(SystemComponent* comp, const unsigned char &reg, Register* &ptr, const std::string &name, const std::string &bits);

	/** Add a system register (without an associated system component)
	  * @param reg Register index (as addr = 0xff00 + reg)
	  * @param ptr Pointer reference where new register pointer will be returned
	  * @param name Human readable name of register
	  * @param bits Read / write bit access string (see Register class)
	  */
	void addSystemRegister(const unsigned char &reg, Register* &ptr, const std::string &name, const std::string &bits);
	
	/** Add a dummy register to the list of system registers
	  * Dummy registers are used to make a system component sensitive to a register address, even though the register does not physically exist.
	  * @param comp Associated system component
	  * @param reg Register index (as addr = 0xff00 + reg)
	  */
	void addDummyRegister(SystemComponent* comp, const unsigned char &reg);

	/** Zero the specified system register
	  * @param reg Register index (as addr = 0xff00 + reg)
	  */
	void clearRegister(const unsigned char &reg);
	
	/** Dump current contents of memory to output file (64 kB)
	  */
	bool dumpMemory(const std::string &fname);
	
	/** Dump current contents of VRAM to output file (16 kB)
	  */
	bool dumpVRAM(const std::string &fname);
	
	/** Write cartridge save RAM to a file
	  * @return True if cartridge supports save RAM and it is written successfully
	  */	
	bool saveSRAM(const std::string &fname);
	
	/** Copy cartridge save RAM from a file
	  * @return True if cartridge supports save RAM and it is copied successfully
	  */
	bool loadSRAM(const std::string &fname);
	
	/** Handle system horizontal blank (HBlank) period
	  * Draw current LCD scanline and call DMA controller to handle any active HBlank transfers.
	  */
	void handleHBlankPeriod();
	
	/** Request vertical blanking (VBlank) period interrupt (INT 40)
	  */
	void handleVBlankInterrupt();
	
	/** Request LCD STAT interrupt (INT 48)
	  */
	void handleLcdInterrupt();
	
	/** Request system timer interrupt (INT 50)
	  */
	void handleTimerInterrupt();
	
	/** Request serial controller interrupt (INT 58)
	  */
	void handleSerialInterrupt();
	
	/** Request joypad controller interrupt (INT 60)
	  */
	void handleJoypadInterrupt();

	/** Enable all interrupts
	  */
	void enableInterrupts();
	
	/** Disable all interrupts
	  */
	void disableInterrupts();

	/** Put CPU into HALTED state
	  */
	void haltCPU(){
		cpuHalted = true;
	}
	
	/** Put CPU into STOPPED state
	  */
	void stopCPU(){
		cpuStopped = true;
	}
	
	/** Resume CPU from HALTED or STOPPED state
	  */
	void resumeCPU();
	
	/** Update debugger gui and process Qt events
	  */
	void updateDebuggers();

	/** Pause emulation
	  * Audio output interface is also disabled.
	  */	
	void pause();
	
	/** Resume emulation
	  * @param resumeAudio If set, audio output interface is also resumed, otherwise it remains disabled
	  */
	void unpause(bool resumeAudio=true);

	/** Reset emulator to beginning of input ROM program
	  */
	bool reset();

	/** Save the current frame buffer as an image
	  * Not implemented.
	  */
	bool screenshot();

	/** Write a savestate file
	  * If filename not specified, the current input ROM filename plus extension ".sav" is used.
	  * @param fname Savestate filename
	  * @return True if savestate is written successfully
	  */
	bool quicksave(const std::string& fname="");
	
	/** Load a savestate file
	  * If filename not specified, the current input ROM filename plus extension ".sav" is used.
	  * @param fname Savestate filename
	  * @return True if savestate is loaded successfully
	  */
	bool quickload(const std::string& fname="");

	/** Write cartridge save RAM to a file
	  * The current input ROM filename plus extension ".sram" is used.
	  * @return True if cartridge supports save RAM and it is written successfully
	  */	
	bool writeExternalRam();

	/** Copy cartridge save RAM from a file
	  * The current input ROM filename plus extension ".sram" is used.
	  * @return True if cartridge supports save RAM and it is copied successfully
	  */
	bool readExternalRam();
	
	/** Print help message to stdout
	  */
	void help();
	
	/** Open LR35902 interpretr console
	  */
	void openDebugConsole();
	
	/** Close LR35902 interpreter console
	  */
	void closeDebugConsole();
	
	/** Stop execution and close all windows
	  */
	void quit(){
		userQuitting = true;
	}

	/** Resume emulation until just after the next CPU instruction finishes execution
	  * Does not resume audio output.
	  */	
	void stepThrough();
	
	/** Resume emulation until just after the next system clock tick
	  * Does not resume audio output.
	  */	
	void advanceClock();
	
	/** Resume emulation until just after the next scanline is drawn
	  * Does not resume audio output.
	  */
	void resumeUntilNextHBlank();
	
	/** Resume emulation until just after the next frame is drawn
	  * Does not resume audio output.
	  */
	void resumeUntilNextVBlank();
	
	/** Lock read / write access to VRAM and / or OAM
	  */
	void lockMemory(bool lockVRAM, bool lockOAM);
	
	/** Enable vertical syncronization (VSync).
	  * VSync may not be available on all platforms and hardware.
	  */
	void enableVSync();

	/** Disable vertical syncronization (VSync)
	  */
	void disableVSync();

	/** Switch from windowed mode to full screen, or vice versa.
	  * VSync will be enabled when switching from windowed to full screen, unless VSync is disabled (and is not forced on).
	  * VSync will be disabled when switching from full screen back to windowed, unless VSync is forced on.
	  */
	void toggleFullScreenMode();

private:
	SystemComponent dummyComponent; ///< Dummy system component used to organize system registers

	unsigned short nFrames; ///< Drawn frames counter 
	
	unsigned short frameSkip; ///< The value N for drawing every 1 out of N frames (or the number of frames to skip between rendered frames plus 1)

	unsigned short bootLength; ///< Size of the boot ROM (bytes)

	bool verboseMode; ///< Verbosity flag
	
	bool debugMode; ///< Debug flag

	bool cpuStopped; ///< Set if CPU is in a stopped state (STOP 0)
	
	bool cpuHalted; ///< Set if CPU is in a halted state (HALT)
	
	bool emulationPaused; ///< Set if emulator is paused
	
	bool bootSequence; ///< Set if the boot ROM is still executing
	
	bool forceColor; ///< Set if CGB features will be forced for DMG games (has no effect for CGB games)
	
	bool displayFramerate; ///< Set if framerate will be displayed on screen
	
	bool userQuitting; ///< Set if user has issued the command to quit
	
	bool autoLoadExtRam; ///< Set if external cartridge RAM (SRAM) will not be loaded at boot
	
	bool initSuccessful; ///< Set if all components were initialized successfully
	
	bool fatalError; ///< Set if a fatal error was encountered
	
	bool consoleIsOpen; ///< Set if interpreter console is currently open
	
	bool bLockedVRAM; ///< Set if VRAM is locked while being read by PPU 
	
	bool bLockedOAM; ///< Set if OAM is locked while being read by PPU

	bool bNeedsReloaded; ///< Set if ROM file should be reloaded on call to reset()

	bool bUseTileViewer; ///< Set if user has requested tile bitmap viewer window
	
	bool bUseLayerViewer; ///< Set if user has requested render layer viewer window

	bool bAudioOutputEnabled; ///< Set if audio output interface is enabled

	bool bVSyncEnabled; ///< Set if VSync will be used when in fullscreen mode
	
	bool bForceVSync; ///< Set if VSync will always be used, regardless of windowed / fullscreen mode

	bool bLayerViewerSelect; ///< 

	bool bHardPaused; ///< Set if user has paused emulator manually, or if breakpoint reached

	unsigned char dmaSourceH; ///< DMA source MSB
	
	unsigned char dmaSourceL; ///< DMA source LSB
	
	unsigned char dmaDestinationH; ///< DMA destination MSB
	
	unsigned char dmaDestinationL; ///< DMA destination LSB

	std::string romPath; ///< Full path to input ROM file
	
	std::string romDirectory; ///< Input ROM directory path (no filename)
	
	std::string romFilename; ///< Input ROM filename (with no path or extension)
	
	std::string romExtension; ///< Input ROM file extension

	SoundManager* audioInterface; ///< Pointer to sound output interface

	OTTWindow* window; ///< Pointer to output graphical window

	std::unique_ptr<SerialController> serial; ///< Pointer to serial I/O controller
	
	std::unique_ptr<DmaController> dma; ///< Pointer to Direct Memory Access (DMA) controller
	
	std::unique_ptr<Cartridge> cart; ///< Pointer to cartridge ROM
	
	std::unique_ptr<GPU> gpu; ///< Pointer to Pixel Processing Unit (PPU)
	
	std::unique_ptr<SoundProcessor> sound; ///< Pointer to Audio Processing Unit (APU)
	
	std::unique_ptr<SpriteHandler> oam; ///< Pointer to Object Attribute Memory (OAM) controller 
	
	std::unique_ptr<JoystickController> joy; ///< Pointer to joypad controller
	
	std::unique_ptr<WorkRam> wram; ///< Pointer to Work RAM (WRAM) controller
	
	std::unique_ptr<SystemComponent> hram; ///< Pointer to High RAM (HRAM) controller
	
	std::unique_ptr<SystemClock> sclk; ///< Pointer to system clock
	
	std::unique_ptr<SystemTimer> timer; ///< Pointer to system timer
	
	std::unique_ptr<LR35902> cpu; ///< Pointer to LR35902 emulator
	
	std::unique_ptr<OTTWindow> tileViewer; ///< Pointer to tile viewer window
	
	std::unique_ptr<OTTWindow> layerViewer; ///< Pointer to layer viewer window

#ifdef USE_QT_DEBUGGER
	MainWindow* gui; ///< Pointer to Qt gui debugger (if available)
#endif

	Breakpoint breakpointProgramCounter; ///< Program counter breakpoint
	
	Breakpoint breakpointMemoryWrite; ///< Memory address write access breakpoint
	
	Breakpoint breakpointMemoryRead; ///< Memory address read access breakpoint
	
	Breakpoint breakpointOpcode; ///< LR35902 opcode read breakpoint

	unsigned int nBreakCycles; ///< Number of clock cycles remaining before next break

	unsigned short memoryAccessWrite[2]; ///< Memory address write access region
	
	unsigned short memoryAccessRead[2]; ///< Memory address read access region

	std::vector<Register> registers; ///< System control registers
	
	std::vector<unsigned char> bootROM; ///< Variable length DMG / CGB boot ROM

	std::vector<BackgroundWindowSettings> winScrollPositions; ///< Vector of window and scroll coordinates for each scanline

	std::unique_ptr<ComponentList> subsystems; ///< List of all system component pointers 

	/** Write to a system register 
	  * Note: The true register value will be AND-ed together with its writable bit bitmask
	  * @param reg 16-bit register address (ff00 to ff80)
	  * @param val Value to attempt to write to register
	  * @return True if register is written to successfully, otherwise return false
	  */
	bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	/** Read from a system register 
	  * Note: The true register value will be AND-ed together with its readable bit bitmask
	  * @param reg 16-bit register address (ff00 to ff80)
	  * @param val Value reference to read register value into
	  * @return True if register is read from successfully, otherwise return false
	  */
	bool readRegister(const unsigned short &reg, unsigned char &val);
	
	/** Check for pressed / held keyboard keys
	  */
	void checkSystemKeys();
};

#endif
