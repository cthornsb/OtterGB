#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <fstream>
#include <string>
#include <vector>
#include <memory>

#include "SystemComponent.hpp"
#include "MemoryController.hpp"

class Cartridge : public SystemComponent {
public:
	/** Default constructor
	  */
	Cartridge();
	
	/** Write to cartridge RAM (if available)
	  * @param addr 16-bit system memory address
	  * @param value 8-bit value to write to RAM
	  * @return True if value is written successfully
	  */
	bool writeToRam(const unsigned short& addr, const unsigned char& value);

	/** Read from cartridge RAM (if available)
	  * @param addr 16-bit system memory address
	  * @param value 8-bit value reference to read from RAM
	  * @return True if value is read successfully
	  */
	bool readFromRam(const unsigned short& addr, unsigned char& value);

	/** ROM is read-only, so return false to prevent writing to it
	  */
	bool preWriteAction() override { 
		return false; 
	}
	
	/** Preare to read from ROM memory
	  * @return True if address being written to resides in ROM bank 0 or ROM swap bank and false otherwise
	  */
	bool preReadAction() override ;

	/** Write cartridge MBC register
	  * @return True if MBC register is written successfully and return false otherwise
	  */
	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;
	
	/** Read cartridge MBC register
	  * @return True if MBC register is read successfully and return false otherwise
	  */
	bool readRegister(const unsigned short &reg, unsigned char &val) override ;

	/** Return true if the cartridge has an internal RAM bank
	  */
	bool hasRam() const { 
		return !ram.empty(); 
	}

	/** Return true if a ROM is loaded in memory
	  */
	bool isLoaded() const {
		return bLoaded; 
	}

	/** Get pointer to internal RAM bank
	  */
	SystemComponent *getRam(){ 
		return &ram; 
	}

	/** Get cartridge title c-string
	  */
	char *getRawTitleString(){ 
		return titleString; 
	}

	/** Get cartridge title string
	  */
	std::string getTitleString() const { 
		return std::string(titleString); 
	}
	
	/** Get cartridge language string
	  */
	std::string getLanguage() const { 
		return (language == 0x0 ? "Japanese" : "English"); 
	}

	/** Get size of catridge ROM (in kB)
	  */
	unsigned short getRomSize() const { 
		return size/1024; 
	}
	
	/** Get size of internal cartridge RAM (in kB)
	  */
	unsigned short getRamSize() const { 
		return ram.getSize()/1024; 
	}
	
	/** Get the cartridge type ID
	  */
	unsigned char getCartridgeTypeID() const { 
		return cartridgeType; 
	}

	/** Get 16-bit program start address
	  */
	unsigned short getProgramEntryPoint() const { 
		return programStart; 
	}	
	
	/** Return true if cartridge RAM is present and is enabled
	  */
	bool getExternalRamEnabled() const {
		return (!ram.empty() && mbc->getRamEnabled());
	}
	
	/** Return true if cartridge supports battery-backup saves (SRAM)
	  */
	bool getSaveSupport() const { 
		return (!ram.empty() && batterySupport); 
	}
	
	/** Return true if cartridge has internal timer support
	  */
	bool getTimerSupport() const { 
		return timerSupport; 
	}
	
	/** Return true if cartridge has rumble feature support
	  */
	bool getRumbleSupport() const { 
		return rumbleSupport; 
	}
	
	 /** Return true if cartridge supports CGB features
	   */
	 bool getSupportCGB() const {
	 	return ((gbcFlag & 0x80) == 0x80);
	 }
	
	/** Read input ROM file and load entire ROM into memory
	  * @param fname Path to input ROM file
	  * @param verbose If set to true, ROM header will be printed to stdout
	  * @return True if file is read successfully and return false otherwise
	  */
	bool readRom(const std::string &fname, bool verbose=false);

	/** Unload ROM from memory
	  */
	void unload();

	/** Print cartridge ROM header information
	  */
	void print();

	/** Method called once per 1 MHz system clock tick.
	  * Clock the real-time-clock, if the cartridge contains an MBC which uses it.
	  */
	bool onClockUpdate() override;
	
	/** Get cartridge ROM MBC type string
	  */
	std::string getCartridgeType() const {
		return mbc->getTypeString();
	}

protected:
	bool bLoaded; ///< Set to true if a ROM is loaded in memory

	bool extRamSupport; ///< Cartridge contains internal RAM bank(s)

	bool batterySupport; ///< Cartridge supports battery-backup saves

	bool timerSupport; ///< Cartridge supports internal timer

	bool rumbleSupport; ///< Catridge supports rumble feature

	unsigned char leader; ///< Header block leader opcode (usually a JP)

	unsigned short programStart; ///< Program entry point address

	unsigned char bootBitmapString[48]; ///< Boot bitmap

	char titleString[12]; ///< Cartridge title string

	char manufacturer[5]; ///< 4 character manufacturer code

	unsigned char gbcFlag; ///< CGB flag

	char licensee[3]; ///< 2 character licensee code

	unsigned char sgbFlag; ///< SGB flag

	unsigned char cartridgeType; ///< Catridge type ID

	unsigned char romSize; ///< Cartridge ROM size ID

	unsigned char ramSize; ///< Cartridge internal RAM size ID

	unsigned char language; ///< Cartridge language

	unsigned char oldLicensee; ///< Licensee ID number

	unsigned char versionNumber; ///< Cartridge version number

	unsigned char headerChecksum; ///< Checksum of header

	unsigned short globalChecksum; ///< Checksum of ROM

	SystemComponent ram; ///< Internal RAM bank(s)
	
	std::unique_ptr<mbcs::MemoryController> mbc; ///< Cartridge memory bank controller (MBC)
	
	/** Read input ROM header
	  * Initialize ROM memory (and RAM if enabled) and set cartridge feature flags
	  * @return The number of header bytes read from input stream
	  */
	unsigned int readHeader(std::ifstream &f);

	/** Create internal MBC registers and add them to the register vector
	  */	
	virtual void createRegisters() {
	}
	
	/** Called whenever a cartridge RAM write is requested.
	  * Cartridge MBC may intercept request and prevent RAM access by returning true.
	  * @return True if write was handled by MBC and no further RAM access should occur
	  */
	virtual bool onUserWriteToRam(const unsigned short& addr, const unsigned char& value) {
		return false;
	}

	/** Called whenever a cartridge RAM read is requested.
	  * Cartridge MBC may intercept request and prevent RAM access by returning true.
	  * @return True if write was handled by MBC and no further RAM access should occur
	  */
	virtual bool onUserReadFromRam(const unsigned short& addr, unsigned char& value) {
		return false;
	}
};

#endif
