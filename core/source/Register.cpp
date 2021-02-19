#include "Register.hpp"
#include "Support.hpp"

Register::Register(const std::string &name, const std::string &bits) : 
	value(0), 
	readBits(0), 
	writeBits(0), 
	sName(name), 
	address(0), 
	comp(0x0) 
{ 
	setMasks(bits);
}

unsigned char Register::getBits(const unsigned char &lowBit, const unsigned char &highBit) const {
	return ((value & getBitmask(lowBit, highBit)) >> lowBit);
}

void Register::setMasks(const std::string &masks){
	for(unsigned char i = 0; i < masks.length(); i++){
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

