#ifndef SYSTEM_GBC_HPP
#define SYSTEM_GBC_HPP

#include <vector>

#include "SystemComponent.hpp"
#include "Cartridge.hpp"
#include "GPU.hpp"
#include "Sound.hpp"
#include "SystemTimer.hpp"
#include "Joystick.hpp"
#include "LR35902.hpp"
#include "WorkRam.hpp"
#include "DmaController.hpp"

#define REGISTER_LOW  0xFF00
#define REGISTER_HIGH 0xFF80

class SystemGBC{
public:
	SystemGBC();

	bool initialize(const std::string &fname);

	bool execute();
	
	bool write(const unsigned short &loc, unsigned char *src){ return write(loc, (*src)); }
	
	bool write(const unsigned short &loc, const unsigned char &src);

	bool read(const unsigned short &loc, unsigned char *dest){ return read(loc, (*dest)); }
	
	bool read(const unsigned short &loc, unsigned char &dest);
	
	unsigned char getValue(const unsigned short &loc);
	
	unsigned char *getPtr(const unsigned short &loc);
	
	unsigned char *getPtrToRegister(const unsigned short &reg);

	LR35902 *getCPU(){ return &cpu; }
	
	GPU *getGPU(){ return &gpu; }
	
	SystemClock *getClock(){ return &clock; }
	
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
	
	bool dumpMemory(const char *fname);
	
	bool dumpVRAM(const char *fname);
	
	bool dumpSRAM(const char *fname);
	
	void handleHBlankPeriod();
	
	void handleVBlankInterrupt();
	
	void handleLcdInterrupt();
	
	void handleTimerInterrupt();
	
	void handleSerialInterrupt();
	
	void handleJoypadInterrupt();

	void enableInterrupts(){ masterInterruptEnable = 0x1; }
	
	void disableInterrupts(){ masterInterruptEnable = 0x0; }

	void haltCPU(){ cpuHalted = true; }
	
	void stopCPU(){ cpuStopped = true; }
	
	void resumeCPU();
	
	void pause(){ emulationPaused = true; }
	
	void unpause(){ emulationPaused = false; }

	bool screenshot();

	bool quicksave();
	
	bool quickload();
	
	void help();
	
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

	unsigned char masterInterruptEnable; ///< Master interrupt enable
	unsigned char interruptEnable; ///< Interrupt enable register (FFFF)
	unsigned char dmaSourceH; ///< DMA source MSB
	unsigned char dmaSourceL; ///< DMA source LSB
	unsigned char dmaDestinationH; ///< DMA destination MSB
	unsigned char dmaDestinationL; ///< DMA destination LSB

	unsigned short memoryAccessWrite[2]; ///< User-set memory 
	unsigned short memoryAccessRead[2]; ///< 

	unsigned char registers[REGISTER_HIGH-REGISTER_LOW]; ///< System control registers
	
	unsigned short bootLength; ///< Size of the boot ROM
	std::vector<unsigned char> bootROM; ///< Variable length gameboy/gameboy color boot ROM

	DmaController dma;
	Cartridge cart;
	GPU gpu;
	SoundProcessor sound;
	SpriteAttHandler oam;
	JoystickController joy;
	WorkRam wram; // 8 4kB banks of RAM
	SystemComponent hram; // 127 bytes of high RAM
	SystemClock clock;
	SystemTimer timer;
	LR35902 cpu;

	bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	bool readRegister(const unsigned short &reg, unsigned char &val);
	
	void checkSystemKeys();
};

#endif
