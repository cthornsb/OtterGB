#ifndef SYSTEM_CLOCK_HPP
#define SYSTEM_CLOCK_HPP

#include <OTTFrameTimer.hpp>

#include "SystemComponent.hpp"

class SystemClock : public OTTFrameTimer, public SystemComponent {
public:
	/** Default constructor
	  */
	SystemClock();

	/** Set the target framerate multiplier
	  * @param freq Framerate multiplier where a multiplier of 1.0 corresponds to normal output framerate of 59.7 fps
	  */
	void setFramerateMultiplier(const float &freq);

	/** Set system clock to CGB double speed mode (~2 MHz), effectively doubling the number of instructions per second
	  * The CPU, system timer, serial controller, and DMA controller will operate at double speed while
	  * the PPU, HDMA controller, and APU will all operate at normal speed.
	  * Note: Changing clock speed does directly affect output framerate
	  */
	void setDoubleSpeedMode();
	
	/** Set system clock to normal speed mode (~1 MHz)
	  * Note: Changing clock speed does not directly affect output framerate
	  */
	void setNormalSpeedMode();

	/** Get the current system clock speed in cycles per second
	  */
	double getFrequency() const { 
		return currentClockSpeed; 
	}

	/** Get the current number of clock cycles per second
	  */
	double getCyclesPerSecond() const;
	
	/** Get the number of clock cycles since the previous vertical blanking (VBlank) interval ended
	  */
	unsigned int getCyclesSinceVBlank() const { 
		return cyclesSinceLastVSync; 
	}
	
	/** Get the number of clock cycles since the previous horizontal blanking (HBlank) interval ended
	  */
	unsigned int getCyclesSinceHBlank() const { 
		return cyclesSinceLastHSync; 
	}
	
	/** Get the number of clock cycles remaining until the start of the next frame
	  */
	unsigned int getCyclesUntilNextFrame() const {
		return (cyclesPerVSync - cyclesSinceLastVSync);
	}

	/** Get the number of clock cycles remaining until the start of the next scanline
	  */
	unsigned int getCyclesUntilNextScanline() const {
		return (cyclesPerHSync - cyclesSinceLastHSync);
	}

	/** Get the current LCD driver mode
	  *  0: Horizontal blank (HBlank) interval
	  *  1: Vertical blank (VBlank) interval
	  *  2: PPU is searching OAM for sprite coordinates (OAM inaccessible)
	  *  3: PPU is drawing the current scanline (VRAM and OAM inaccessible)
	  */
	unsigned char getDriverMode() const { 
		return lcdDriverMode; 
	}
	
	/** Return true if the LCD driver is in vertical blank (VBlank) interval
	  */
	bool getVSync() const { 
		return bVSync; 
	}
	
	/** Set the number of pixel clock (~4 MHz) ticks to delay start of HBlank interval (mode 0)
	  * This delay is due to several factors, the primary one being the number of sprites the PPU drew on the current scanline
	  */
	void setPixelClockPause(const unsigned int& ticks){
		nClockPause = ticks;
	}
	
	/** Poll the VSync (vertical blank interval) flag and reset it.
	  */
	bool pollVSync(){ 
		return (bVSync ? !(bVSync = false) : false); 
	}
	
	/** Perform one system clock tick
	  * @return True if the system has entered vertical blank (VBlank) interval, and false otherwise
	  */
	bool onClockUpdate() override ;

	/** Read settings from an input user configuration file
	  */
	void readConfigFile(ConfigFile* config) override;
	
	/** Reset cycle counters, zero LY register, and handle LY - LYC coincidence check
	  * If LCDC register bit 7 is set (LCD enabled) LCD driver mode is set to 2, otherwise driver mode is set to 1
	  */
	void resetScanline();

private:
	bool bDoubleSpeedMode; ///< Set if emulator is in CGB double speed mode (~2 MHz)

	bool bVSync; ///< Set if LCD driver is in vertical blank interval

	unsigned int cyclesSinceLastVSync; ///< Number of pixel clock ticks since the last vertical blank interval
	
	unsigned int cyclesSinceLastHSync; ///< Number of pixel clock ticks since the last horizontal blank interval

	unsigned int currentClockSpeed; ///< Current number of system clock cycles per second
	
	unsigned int cyclesPerVSync; ///< Current number of pixel clock ticks per vertical sync
	
	unsigned int cyclesPerHSync; ///< Current number of pixel clock ticks per horizontal sync

	unsigned char lcdDriverMode; ///< Current LCD driver mode (0-3)

	unsigned int cycleCounter; ///< System clock cycle counter

	double cyclesPerSecond; ///< Average number of system clock cycles per second over a ten second period

	unsigned int nClockPause; ///< Number of pixel clock ticks to pause before entering HBlank period (mode 0)

	unsigned int modeStart[4]; ///< Number of pixel clock ticks to elapse before starting the next LCD driver mode

	/** Increment the current scanline (register LY)
	  */
	void incrementScanline();

	/** Handle coincidence between LY and LYC registers
	  * Updates STAT register coincidence bit and issues LCD STAT interrupt request if enabled.
	  * @return True if LY == LYC, and return false otherwise
	  */
	bool compareScanline();

	/** Begin mode 0 (horizontal blank period)
	  * Request mode 0 STAT interrupt (INT 48, if enabled)
	  */
	void startMode0();

	/** Begin mode 1 (vertical blank period)
	  * Request mode 1 STAT interrupt (INT 48, if enabled)
	  * Request VBlank interrupt (INT 40)
	  */
	void startMode1();

	/** Begin mode 2 (OAM search)
	  * OAM inaccessible
	  * Request mode 2 STAT interrupt (INT 48, if enabled)
	  */
	void startMode2();
	
	/** Begin mode 3 (drawing scanline)
	  * VRAM and OAM inaccessible
	  * Trigger PPU to start drawing scanline
	  */
	void startMode3();
	
	/** Add elements to a list of values which will be written to / read from an emulator savestate
	  */
	void userAddSavestateValues() override;

	/** Reset system clock
	  */
	void onUserReset() override;
};

#endif

