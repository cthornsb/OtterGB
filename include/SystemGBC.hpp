#ifndef SYSTEM_GBC_HPP
#define SYSTEM_GBC_HPP

#include <vector>
#include <memory>
#include <map>

#include "SystemComponent.hpp"
#include "SystemRegisters.hpp"
#include "Cartridge.hpp"
#include "GPU.hpp"
#include "Sound.hpp"
#include "SystemTimer.hpp"
#include "Joystick.hpp"
#include "LR35902.hpp"
#include "WorkRam.hpp"
#include "DmaController.hpp"
#include "SerialController.hpp"

#define REGISTER_LOW  0xFF00
#define REGISTER_HIGH 0xFF80

#ifdef USE_QT_DEBUGGER
	class QApplication;
	class MainWindow;
#endif

class ComponentList{
public:
	SerialController   *serial;
	DmaController      *dma;
	Cartridge          *cart;
	GPU                *gpu;
	SoundProcessor     *apu;
	SpriteHandler      *oam;
	JoystickController *joy;
	WorkRam            *wram; // 8 4kB banks of RAM
	SystemComponent    *hram; // 127 bytes of high RAM
	SystemClock        *sclk;
	SystemTimer        *timer;
	LR35902            *cpu;
	
	std::map<std::string, SystemComponent*> list;
	
	ComponentList(){ }
	
	ComponentList(SystemGBC *sys);
};

class Breakpoint{
public:
	Breakpoint() : d8(0), d16(0), d32(0), enabled(0) { }

	void enable(const unsigned char &value){ d8 = value; enabled |= 0x1; }
	
	void enable(const unsigned short &value){ d16 = value; enabled |= 0x2; }
	
	void enable(const unsigned int &value){ d32 = value; enabled |= 0x4; }

	void clear(){ enabled = 0; }
	
	bool check(const unsigned char &value) const { return ((enabled & 0x1) && value == d8); }
	
	bool check(const unsigned short &value) const { return ((enabled & 0x2) && value == d16); }
	
	bool check(const unsigned int &value) const { return ((enabled & 0x4) && value == d32); }
	
private:
	unsigned char d8;
	
	unsigned short d16;
	
	unsigned int d32;
	
	unsigned char enabled;
};

class SystemGBC{
	friend class ComponentList;
public:
	SystemGBC();
	
	~SystemGBC();

	void initialize();

	bool execute();
	
	bool write(const unsigned short &loc, unsigned char *src){ return write(loc, (*src)); }
	
	bool write(const unsigned short &loc, const unsigned char &src);

	bool read(const unsigned short &loc, unsigned char *dest){ return read(loc, (*dest)); }
	
	bool read(const unsigned short &loc, unsigned char &dest);

	std::string getRomFilename() const { return romFilename; }
	
	bool getEmulationPaused() const { return emulationPaused; }
	
	unsigned char getValue(const unsigned short &loc);
	
	unsigned char *getPtr(const unsigned short &loc);
	
	const unsigned char *getConstPtr(const unsigned short &loc);
	
	Register *getPtrToRegister(const unsigned short &reg);

	unsigned char *getPtrToRegisterValue(const unsigned short &reg);

	const unsigned char *getConstPtrToRegisterValue(const unsigned short &reg);

	LR35902 *getCPU(){ return &cpu; }
	
	GPU *getGPU(){ return &gpu; }
	
	SpriteHandler *getOAM(){ return &oam; }
	
	SystemClock *getClock(){ return &clock; }
	
	Cartridge *getCartridge(){ return &cart; }
	
	WorkRam *getWRAM(){ return &wram; }
	
	ComponentList *getListOfComponents(){ return subsystems.get(); }
	
	 // Toggle CPU debug flag
	void setDebugMode(bool state=true);

	// Toggle framerate output
	void setDisplayFramerate(bool state=true){ displayFramerate = state; }

	// Set CPU frequency multiplier
	void setCpuFrequency(const double &multiplier);

	// Toggle verbose flag
	void setVerboseMode(bool state=true);
	
	// Force GBC features for original GB games.
	void setForceColorMode(bool state=true){ forceColor = state; }
	
	void setMemoryWriteRegion(const unsigned short &locL, const unsigned short &locH=0);
	
	void setMemoryReadRegion(const unsigned short &locL, const unsigned short &locH=0);
	
	void setFrameSkip(const unsigned short &frames){ frameSkip = frames; }

	void setRomFilename(const std::string &fname){ romFilename = fname; }

	void setBreakpoint(const unsigned short &pc);
	
	void setMemWriteBreakpoint(const unsigned short &addr);
	
	void setMemReadBreakpoint(const unsigned short &addr);
	
	void setOpcodeBreakpoint(const unsigned char &op, bool cb=false);

	void clearBreakpoint();
	
	void clearMemWriteBreakpoint();
	
	void clearMemReadBreakpoint();
	
	void clearOpcodeBreakpoint();

	void addSystemRegister(SystemComponent *comp, const unsigned char &reg, Register* &ptr, const std::string &name, const std::string &bits);
	
	void addDummyRegister(SystemComponent *comp, const unsigned char &reg);

	void clearRegister(const unsigned char &reg);
	
	bool dumpMemory(const char *fname);
	
	bool dumpVRAM(const char *fname);
	
	bool dumpSRAM(const char *fname);
	
	void handleHBlankPeriod();
	
	void handleVBlankInterrupt();
	
	void handleLcdInterrupt();
	
	void handleTimerInterrupt();
	
	void handleSerialInterrupt();
	
	void handleJoypadInterrupt();

	void enableInterrupts();
	
	void disableInterrupts();

	void haltCPU(){ cpuHalted = true; }
	
	void stopCPU(){ cpuStopped = true; }
	
	void resumeCPU();
	
	void pause();
	
	void unpause();

	bool reset();

	bool screenshot();

	bool quicksave();
	
	bool quickload();
	
	void help();
	
	void quit(){ userQuitting = true; }
	
private:
	unsigned short nFrames;
	unsigned short frameSkip;

	bool verboseMode; ///< Verbosity flag
	bool debugMode; ///< Debug flag

	bool cpuStopped; ///< 
	bool cpuHalted; ///<
	bool emulationPaused; ///<
	bool bootSequence; ///< The gameboy boot ROM is still running
	bool forceColor; 
	bool prepareSpeedSwitch;
	bool currentClockSpeed;
	bool displayFramerate;
	bool userQuitting;

	unsigned char dmaSourceH; ///< DMA source MSB
	unsigned char dmaSourceL; ///< DMA source LSB
	unsigned char dmaDestinationH; ///< DMA destination MSB
	unsigned char dmaDestinationL; ///< DMA destination LSB

	std::string romFilename; ///< Path to input ROM file

#ifdef USE_QT_DEBUGGER
	std::unique_ptr<QApplication> app;

	std::unique_ptr<MainWindow> gui;
#endif

	Breakpoint breakpointProgramCounter;
	Breakpoint breakpointMemoryWrite;
	Breakpoint breakpointMemoryRead;
	Breakpoint breakpointOpcode;

	unsigned short memoryAccessWrite[2]; ///< User-set memory 
	unsigned short memoryAccessRead[2]; ///< 

	Register registers[REGISTER_HIGH-REGISTER_LOW]; ///< System control registers
	
	unsigned short bootLength; ///< Size of the boot ROM
	std::vector<unsigned char> bootROM; ///< Variable length gameboy/gameboy color boot ROM

	SerialController serial;
	DmaController dma;
	Cartridge cart;
	GPU gpu;
	SoundProcessor sound;
	SpriteHandler oam;
	JoystickController joy;
	WorkRam wram; // 8 4kB banks of RAM
	SystemComponent hram; // 127 bytes of high RAM
	SystemClock clock;
	SystemTimer timer;
	LR35902 cpu;

	std::unique_ptr<ComponentList> subsystems;

	bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	bool readRegister(const unsigned short &reg, unsigned char &val);
	
	void checkSystemKeys();
};

#endif
