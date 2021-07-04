#ifndef MEMORY_CONTROLLER_HPP
#define MEMORY_CONTROLLER_HPP

#include <vector>
#include <string>

#include "Register.hpp"
#include "ComponentTimer.hpp"

class Cartridge;
class SystemComponent;

namespace mbcs {

enum class CartMBC {
	UNKNOWN, 
	ROMONLY, 
	MBC1, 
	MBC2, 
	MBC3, 
	MBC5
};

class MemoryController {
public:
	/** Default constructor (invalid MBC)
	  */
	MemoryController() :
		bValid(false),
		bRamEnabled(false),
		bRamSupport(false),
		bBatterySupport(false),
		bTimerSupport(false),
		bRumbleSupport(false),
		mbcType(CartMBC::UNKNOWN),
		sTypeString("UNKNOWN"),
		cart(0x0),
		ram(0x0)
	{
	}

	/** MBC type constructor
	  */
	MemoryController(const CartMBC& type, const std::string& typeStr) :
		bValid(true),
		bRamEnabled(false),
		bRamSupport(false),
		bBatterySupport(false),
		bTimerSupport(false),
		bRumbleSupport(false),
		mbcType(type),
		sTypeString(typeStr),
		cart(0x0),
		ram(0x0)
	{
	}

	/** Set which cartridge features are present
	  */
	void setCartridgeFeatures(const bool& bRam, const bool& bBattery, const bool& bTimer, const bool& bRumble) {
		bRamSupport = bRam;
		bBatterySupport = bBattery;
		bTimerSupport = bTimer;
		bRumbleSupport = bRumble;
	}

	/** Set pointers to cartridge ROM and RAM (if present)
	  */
	void setMemory(Cartridge* ROM, SystemComponent* RAM){
		cart = ROM;
		ram = RAM;
	}

	/** Return true if cartridge RAM is present and is currently enabled
	  */
	bool getRamEnabled() const { 
		return (bRamSupport && bRamEnabled);
	}

	/** Get cartridge MBC type
	  */
	CartMBC getType() const {
		return mbcType;
	}

	/** Get human-readable cartridge MBC type string
	  */
	std::string getTypeString() const {
		return sTypeString;
	}

	/** Write cartridge MBC register
	  * @return True if MBC register is written successfully and return false otherwise
	  */
	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val) {
		return false;
	}
	
	/** Read cartridge MBC register
	  * @return True if MBC register is read successfully and return false otherwise
	  */
	virtual bool readRegister(const unsigned short &reg, unsigned char &val) {
		return false;
	}
	
	/** Request a value to be written to cartridge RAM
	  * Cartridge MBC may intercept request and prevent RAM access by returning true.
	  * @return True if write was handled by MBC and no further RAM access should occur
	  */
	virtual bool writeToRam(const unsigned short& addr, const unsigned char& value) {
		return false;
	}

	/** Request a value to be read from cartridge RAM
	  * Cartridge MBC may intercept request and prevent RAM access by returning true.
	  * @return True if write was handled by MBC and no further RAM access should occur
	  */
	virtual bool readFromRam(const unsigned short& addr, unsigned char& value) {
		return false;
	}
	
	/** Method called once per 1 MHz system clock tick.
	  * Clock the real-time-clock, if MBC contains one.
	  * @return True if clocking the MBC timer caused it to rollover
	  */
	virtual bool onClockUpdate() {
		return false;
	}

protected:
	bool bValid; ///< Set if this object is a valid MBC

	bool bRamEnabled; ///< Set if cartridge ram is enabled
	
	bool bRamSupport; ///< Cartridge contains internal RAM bank(s)

	bool bBatterySupport; ///< Cartridge supports battery-backup saves

	bool bTimerSupport; ///< Cartridge supports internal timer

	bool bRumbleSupport; ///< Catridge supports rumble feature

	CartMBC mbcType; ///< ROM cartridge type
	
	std::string sTypeString; ///< Human readable MBC type string
	
	Cartridge* cart; ///< Pointer to cartridge object
	
	SystemComponent* ram; ///< Pointer to cartridge RAM (if present)
};

class NoMBC : public MemoryController {
public:
	NoMBC() : 
		MemoryController(CartMBC::ROMONLY, "ROM ONLY")
	{
	}
};

class MBC1 : public MemoryController {
public:
	MBC1() : 
		MemoryController(CartMBC::MBC1, "MBC1"),
		rRamEnable("MBC1_ENABLE", "22220000"),
		rRomBankLow("MBC1_BANKLO", "22222000"),
		rRomBankHigh("MBC1_BANKHI", "22000000"),
		rBankModeSelect("MBC1_SELECT", "20000000")
	{
	}
	
	/** Write cartridge MBC register
	  * @return True if MBC register is written successfully and return false otherwise
	  */
	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;
	
protected:
	Register rRamEnable; ///< Cartridge RAM enable register
	
	Register rRomBankLow; ///< Lower 5 bits of cartridge ROM bank register
	
	Register rRomBankHigh; ///< Upper 2 bits of cartridge ROM bank or RAM bank number register
	
	Register rBankModeSelect; ///< Cartridge ROM / RAM bank mode select register
};

class MBC2 : public MemoryController {
public:
	MBC2() : 
		MemoryController(CartMBC::MBC2, "MBC2"),
		rRomBank("MBC2_BANK", "22220000")
	{
	}
	
	/** Write cartridge MBC register
	  * @return True if MBC register is written successfully and return false otherwise
	  */
	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;

protected:
	Register rRomBank; ///< ROM bank register
};

class MBC3 : public MemoryController {
public:
	MBC3() : 
		MemoryController(CartMBC::MBC3, "MBC3"),
		bLatchState(false),
		nRtcTimerSeconds(0),
		rtcTimer(32), // 32.768 kHz clock
		rRamEnable("MBC3_ENABLE", "22220000"),
		rRomBank("MBC3_ROMBANK", "22222220"),
		rRamBank("MBC3_RAMBANK", "22220000"),
		rLatch("MBC3_LATCH", "20000000"),
		rSeconds("MBC3_SECONDS", "33333333"),
		rMinutes("MBC3_MINUTES", "33333333"),
		rHours("MBC3_HOURS", "33333000"),
		rDayLow("MBC3_DAYLOW", "33333333"),
		rDayHigh("MBC3_DAYHIGH", "30000033"),
		registerSelect(0x0)
		
	{
	}
	
	/** Write cartridge MBC register
	  * @return True if MBC register is written successfully and return false otherwise
	  */
	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;
	
	/** Method called once per 1 MHz system clock tick.
	  * Clock the MBC's 32.768 kHz timer.
	  * @return True if clocking the timer caused it to rollover
	  */
	bool onClockUpdate() override ;

protected:
	bool bLatchState; ///< Cartridge real-time-clock (RTC) latch state

	unsigned int nRtcTimerSeconds; ///< The number of seconds elapsed according to the RTC

	ComponentTimer rtcTimer; ///< Real-time-clock (RTC) timer

	Register rRamEnable; ///< Cartridge RAM enable register
	
	Register rRomBank; ///< Cartridge ROM bank number register
	
	Register rRamBank; ///< Cartridge RAM bank number or RTC register select register
	
	Register rLatch; ///< Clock data latch register register
	
	Register rSeconds; ///< RTC seconds counter register
	
	Register rMinutes; ///< RTC minutes counter register
	
	Register rHours; ///< RTC hours counter register
	
	Register rDayLow; ///< Lower 8 bits of RTC day counter
	
	Register rDayHigh; ///< Upper bit of RTC day counter

	Register* registerSelect; ///< Pointer to currently selected RTC register

	/** Request a value to be written to cartridge RAM
	  * Cartridge MBC may intercept request and prevent RAM access by returning true.
	  * @return True if write was handled by MBC and no further RAM access should occur
	  */
	bool writeToRam(const unsigned short& addr, const unsigned char& value) override ;
	
	/** Request a value to be read from cartridge RAM
	  * Cartridge MBC may intercept request and prevent RAM access by returning true.
	  * @return True if write was handled by MBC and no further RAM access should occur
	  */
	bool readFromRam(const unsigned short& addr, unsigned char& value) override ;
};

class MBC5 : public MemoryController {
public:
	MBC5() : 
		MemoryController(CartMBC::MBC5, "MBC5"),
		rRamEnable("MBC5_ENABLE", "22220000"),
		rRomBankLow("MBC5_BANKLO", "22222222"),
		rRomBankHigh("MBC5_BANKHI", "20000000"),
		rRamBank("MBC5_BANKRAM", "22220000")
	{
	}
	
	/** Write cartridge MBC register
	  * @return True if MBC register is written successfully and return false otherwise
	  */
	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;

protected:
	Register rRamEnable; ///< Cartridge RAM enable register
	
	Register rRomBankLow; ///< Lower 8 bits of cartridge ROM bank register
	
	Register rRomBankHigh; ///< Upper 1 bit of cartridge ROM bank register
	
	Register rRamBank; ///< Cartridge RAM bank register
};

} // namespace mbcs

#endif // define MEMORY_CONTROLLER_HPP
