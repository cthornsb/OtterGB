#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <fstream>

#include "SystemComponent.hpp"

class Cartridge : public SystemComponent {
public:
	enum class CartMBC {UNKNOWN, ROMONLY, MBC1, MBC2, MMM01, MBC3, MBC4, MBC5};

	Cartridge();

	// ROM is read-only, so return false to prevent writing to it.
	bool preWriteAction() override { 
		return false; 
	}
	
	bool preReadAction() override ;

	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;
	
	bool readRegister(const unsigned short &reg, unsigned char &val) override ;

	bool hasRam(){ return !ram.empty(); }

	SystemComponent *getRam(){ return &ram; }

	char *getRawTitleString(){ return titleString; }

	std::string getTitleString() const { return std::string(titleString); }

	std::string getLanguage() const { return (language == 0x0 ? "Japanese" : "English"); }

	std::string getCartridgeType() const;
	
	unsigned short getRomSize() const { return size/1024; }
	
	unsigned short getRamSize() const { return ram.getSize()/1024; }
	
	unsigned char getCartridgeTypeID() const { return cartridgeType; }

	unsigned short getProgramEntryPoint() const { return programStart; }	
	
	bool getExternalRamEnabled() const { return extRamEnabled; }
	
	bool getSaveSupport() const { return (!ram.empty() && batterySupport); }
	
	bool getTimerSupport() const { return timerSupport; }
	
	bool getRumbleSupport() const { return rumbleSupport; }
	
	bool readRom(const std::string &fname, bool verbose=false);

	void print();

private:
	bool ramSelect;
	bool extRamEnabled;
	bool extRamSupport;
	bool batterySupport;
	bool timerSupport;
	bool rumbleSupport;

	unsigned char leader;
	unsigned short programStart;
	unsigned char nintendoString[48];
	char titleString[12];
	char manufacturer[5];
	unsigned char gbcFlag;
	char licensee[3];
	unsigned char sgbFlag;
	unsigned char cartridgeType;
	unsigned char romSize;
	unsigned char ramSize;
	unsigned char language;
	unsigned char oldLicensee;
	unsigned char versionNumber;
	unsigned char headerChecksum;
	unsigned short globalChecksum;

	SystemComponent ram;
	
	CartMBC mbcType;
	
	unsigned int readHeader(std::ifstream &f);
};

#endif
