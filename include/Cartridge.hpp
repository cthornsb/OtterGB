#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <fstream>

#include "SystemComponent.hpp"

class Cartridge : public SystemComponent {
public:
	Cartridge() : SystemComponent("Cartridge"), ramSelect(false), extRamEnabled(false) { }

	// ROM is read-only, so return false to prevent writing to it.
	virtual bool preWriteAction(){ return false; }
	
	virtual bool preReadAction();

	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	virtual bool readRegister(const unsigned short &reg, unsigned char &val);

	SystemComponent *getRam(){ return &ram; }

	std::string getTitleString() const { return std::string(titleString); }

	std::string getLanguage() const { return (language == 0x0 ? "Japanese" : "English"); }
	
	unsigned short getRomSize() const { return size/1024; }
	
	unsigned short getRamSize() const { return ram.getSize()/1024; }
	
	unsigned char getCartridgeType() const { return cartridgeType; }

	unsigned short getProgramEntryPoint() const { return programStart; }	
	
	bool getExternalRamEnabled() const { return extRamEnabled; }
	
	bool readRom(const std::string &fname, bool verbose=false);

	void print();

private:
	bool ramSelect;
	bool extRamEnabled;

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
	
	unsigned int readHeader(std::ifstream &f);
};

#endif
