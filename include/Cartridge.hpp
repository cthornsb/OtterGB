#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <fstream>

#include "SystemComponent.hpp"

class Cartridge : public SystemComponent {
public:
	Cartridge() : SystemComponent(), ramSelect(false), extRamEnabled(false) { }

	// ROM is read-only, so return false to prevent writing to it.
	virtual bool preWriteAction(){ return false; }
	
	virtual bool preReadAction();

	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	virtual bool readRegister(const unsigned short &reg, unsigned char &val);

	SystemComponent *getRam(){ return &ram; }

	bool readRom(const std::string &fname);
	
	unsigned short getProgramEntryPoint() const { return programStart; }

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
	
	void print();
};

#endif
