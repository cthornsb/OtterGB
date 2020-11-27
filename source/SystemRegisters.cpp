#include "Support.hpp"
#include "SystemRegisters.hpp"

Register::Register(const std::string &name, const std::string &bits) : value(0), readBits(0), writeBits(0), sName(name), address(0), comp(0x0) { 
	// Read/Write bits
	// 0: Not readable or writeable
	// 1: Read-only
	// 2: Write-only
	// 3: Readable and writeable
	setMasks(bits);
}

unsigned char Register::getBits(const unsigned char &lowBit, const unsigned char &highBit) const {
	return ((value & getBitmask(lowBit, highBit)) >> lowBit);
}

void Register::setMasks(const std::string &masks){
	for(unsigned short i = 0; i < masks.length(); i++){
		switch(masks[i]){
			case '0':
				break;
			case '1':
				bitSet(readBits, i);
				break;
			case '2':
				bitSet(writeBits, i);
				break;
			case '3':
				bitSet(readBits, i);
				bitSet(writeBits, i);
				break;
			default:
				break;
		}
	}
}

void Register::setBit(const unsigned char &bit){ 
	bitSet(value, bit); 
}

void Register::setBits(const unsigned char &lowBit, const unsigned char &highBit){
	for(unsigned char i = lowBit; i <= highBit; i++)
		setBit(i);
}

void Register::setBits(const unsigned char &lowBit, const unsigned char &highBit, const unsigned char &v){
	value = (value & ~getBitmask(lowBit, highBit)) | (v << lowBit);
}

void Register::resetBit(const unsigned char &bit){ 
	bitReset(value, bit); 
}

void Register::resetBits(const unsigned char &lowBit, const unsigned char &highBit){
	for(unsigned char i = lowBit; i <= highBit; i++)
		resetBit(i);
}

std::string Register::dump() const {
	return (getHex(address)+"  "+getHex(value)+"  "+getBinary(value)+"  "+sName);
}

// Pointer to joypad register
Register *rJOYP = 0x0;

// Pointers to serial registers
Register *rSB = 0x0;
Register *rSC = 0x0;

// Pointers to timer registers
Register *rDIV  = 0x0;
Register *rTIMA = 0x0;
Register *rTMA  = 0x0;
Register *rTAC  = 0x0;

// Pointers to DMA registers
Register *rDMA   = 0x0;
Register *rHDMA1 = 0x0;
Register *rHDMA2 = 0x0;
Register *rHDMA3 = 0x0;
Register *rHDMA4 = 0x0;
Register *rHDMA5 = 0x0;

// Pointers to GPU registers
Register *rLCDC = 0x0;
Register *rSTAT = 0x0;
Register *rSCY  = 0x0;
Register *rSCX  = 0x0;
Register *rLY   = 0x0;
Register *rLYC  = 0x0;
Register *rBGP  = 0x0;
Register *rOBP0 = 0x0;
Register *rOBP1 = 0x0;
Register *rWY   = 0x0;
Register *rWX   = 0x0;
Register *rVBK  = 0x0;
Register *rBGPI = 0x0;
Register *rBGPD = 0x0;
Register *rOBPI = 0x0;
Register *rOBPD = 0x0;

// Pointers to sound processor registers
Register *rNR10 = 0x0;
Register *rNR11 = 0x0;
Register *rNR12 = 0x0;
Register *rNR13 = 0x0;
Register *rNR14 = 0x0;
Register *rNR20 = 0x0;
Register *rNR21 = 0x0;
Register *rNR22 = 0x0;
Register *rNR23 = 0x0;
Register *rNR24 = 0x0;
Register *rNR30 = 0x0;
Register *rNR31 = 0x0;
Register *rNR32 = 0x0;
Register *rNR33 = 0x0;
Register *rNR34 = 0x0;
Register *rNR40 = 0x0;
Register *rNR41 = 0x0;
Register *rNR42 = 0x0;
Register *rNR43 = 0x0;
Register *rNR44 = 0x0;
Register *rNR50 = 0x0;
Register *rNR51 = 0x0;
Register *rNR52 = 0x0;
Register *rWAVE[16];

// Pointers to system registers	
Register *rIF   = 0x0;
Register *rKEY1 = 0x0;
Register *rRP   = 0x0;
Register *rIE   = 0x0;
Register *rIME  = 0x0;
Register *rSVBK = 0x0;

// Pointers to undocumented registers
Register *rFF6C = 0x0;
Register *rFF72 = 0x0;
Register *rFF73 = 0x0;
Register *rFF74 = 0x0;
Register *rFF75 = 0x0;
Register *rFF76 = 0x0;
Register *rFF77 = 0x0;

// Gameboy Color flag
bool bGBCMODE = false;
