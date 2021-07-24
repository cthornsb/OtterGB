#include <iostream>
#include <cstdlib>
#include <cinttypes>
#include <cfloat>

#include "Support.hpp"
#include "SystemComponent.hpp"

void SystemComponent::connectSystemBus(SystemGBC *bus){ 
	sys = bus; 
	this->defineRegisters();
	this->userAddSavestateValues();
}

void SystemComponent::initialize(const unsigned short &nB, const unsigned short &N/*=1*/){
	mem = std::vector<std::vector<unsigned char> >(N, std::vector<unsigned char>(nB, 0x0));
	nBytes = nB;
	nBanks = N;
	bs = 0;
	size = nB*N;
}

SystemComponent::~SystemComponent(){
	mem.clear();
}

bool SystemComponent::write(const unsigned short &loc, const unsigned char *src){ 
	return write(loc, bs, (*src));
}

bool SystemComponent::write(const unsigned short &loc, const unsigned char &src){ 
	return write(loc, bs, src);
}

bool SystemComponent::write(const unsigned short &loc, const unsigned short &bank, const unsigned char *src){ 
	return write(loc, bank, (*src));
}

bool SystemComponent::write(const unsigned short &loc, const unsigned short &bank, const unsigned char &src){ 
	writeLoc = loc; 
	writeBank = bank;
	writeVal = src;
	if(readOnly || !preWriteAction())
		return false;
#ifdef DEBUG_OUTPUT
	if (nBytes == 0 || (writeLoc - offset) >= nBytes || writeBank >= nBanks) {
		if (verboseMode)
			std::cout << " [" << sName << "] ERROR! Illegal memory write attempt: addr=" << getHex(readLoc) << ", bank=" << readBank << std::endl;
		return false;
	}
#endif // ifdef DEBUG_OUTPUT
	mem[writeBank][writeLoc-offset] = writeVal;
	return true;		
}	

void SystemComponent::writeFast(const unsigned short &loc, const unsigned char &src){
	if(readOnly)
		return;
	mem[bs][loc-offset] = src;
}

void SystemComponent::writeFastBank0(const unsigned short &loc, const unsigned char &src){
	if(readOnly)
		return;
	mem[0][loc-offset] = src;
}

bool SystemComponent::read(const unsigned short &loc, unsigned char *dest){
	return read(loc, bs, (*dest));
}

bool SystemComponent::read(const unsigned short &loc, unsigned char &dest){ 
	return read(loc, bs, dest);
}

bool SystemComponent::read(const unsigned short &loc, const unsigned short &bank, unsigned char *dest){ 
	return read(loc, bank, (*dest));
}

bool SystemComponent::read(const unsigned short &loc, const unsigned short &bank, unsigned char &dest){ 
	readLoc = loc;
	readBank = bank;
	if(!preReadAction())
		return false;
#ifdef DEBUG_OUTPUT
	if (nBytes == 0 || (readLoc - offset) >= nBytes || readBank >= nBanks) {
		std::cout << " [" << sName << "] ERROR! Illegal memory read attempt: addr=" << getHex(readLoc) << ", bank=" << readBank << std::endl;
		return false;
	}
#endif // ifdef DEBUG_OUTPUT
	dest = mem[readBank][readLoc-offset];
	return true;
}

void SystemComponent::readFast(const unsigned short &loc, unsigned char &dest){ 
#ifdef DEBUG_OUTPUT
	if (nBytes == 0 || (loc - offset) >= nBytes || bs >= nBanks) {
		std::cout << " [" << sName << "] ERROR! Illegal fast write attempt: addr=" << getHex(loc) << ", bank=" << getHex(bs) << std::endl;
		return;
	}
#endif // ifdef DEBUG_OUTPUT
	dest = mem[bs][loc - offset];
}

void SystemComponent::readFastBank0(const unsigned short &loc, unsigned char &dest){ 
#ifdef DEBUG_OUTPUT
	if (nBytes == 0 || (loc - offset) >= nBytes || bs >= nBanks) {
		std::cout << " [" << sName << "] ERROR! Illegal bank-0 fast write attempt: addr=" << getHex(loc) << std::endl;
		return;
	}
#endif // ifdef DEBUG_OUTPUT
	dest = mem[0][loc - offset];
}

void SystemComponent::setBank(const unsigned short& b) {
#ifdef DEBUG_OUTPUT
	if (nBytes == 0 || (writeLoc - offset) >= nBytes || writeBank >= nBanks) {
		if (verboseMode) {
			std::cout << " [" << sName << "] ERROR! Illegal swap bank selected: " << getHex(b) << std::endl;
		}
		return;
	}
#endif // ifdef DEBUG_OUTPUT
	if (b < nBanks)
		bs = b;
}

void SystemComponent::print(const unsigned short bytesPerRow/*=10*/){
}

void SystemComponent::reset(){
	resetMemory();
	this->onUserReset();
}

unsigned int SystemComponent::writeMemoryToFile(std::ofstream &f){
	if(!size)
		return 0;

	// Write memory contents to the output file.	
	unsigned int nBytesWritten = 0;
	for (unsigned short i = 0; i < nBanks; i++) {
		f.write((char*)&mem[i][0], nBytes);
		nBytesWritten += nBytes;
	}

	return nBytesWritten;
}

unsigned int SystemComponent::readMemoryFromFile(std::ifstream &f){
	if(!size)
		return 0;

	// Write memory contents to the output file.
	unsigned int nBytesRead = 0;
	for (unsigned short i = 0; i < nBanks; i++) {
		f.read((char*)&mem[i][0], nBytes);
		nBytesRead += nBytes;
	}

	return nBytesRead;
}

unsigned int SystemComponent::writeSavestate(std::ofstream &f){
	size_t nWritten = 0; 
	nWritten += writeSavestateHeader(f); // Write the component header
	for(auto val = userValues.cbegin(); val != userValues.cend(); val++){
		f.write(static_cast<char*>(val->first), val->second);
		nWritten += val->second;
	}
	if(bSaveRAM)
		nWritten += writeMemoryToFile(f); // Write associated component RAM
	return (unsigned int)nWritten;
}

unsigned int SystemComponent::readSavestate(std::ifstream &f){
	size_t nRead = 0;
	nRead += readSavestateHeader(f); // Read component header
	for(auto val = userValues.cbegin(); val != userValues.cend(); val++){
		f.read(static_cast<char*>(val->first), val->second);
		nRead += val->second;
	}
	if(bSaveRAM)
		nRead += readMemoryFromFile(f); // Read associated component RAM
	return (unsigned int)nRead;
}

void SystemComponent::addSavestateValue(void* ptr, const SavestateType& type, const unsigned int& N/* = 1*/) {
	switch (type) {
	case SavestateType::BOOL:
		userValues.push_back(std::make_pair(ptr, N * sizeof(bool)));
		break;
	case SavestateType::CHAR:
		userValues.push_back(std::make_pair(ptr, N * sizeof(int8_t)));
		break;
	case SavestateType::SHORT:
		userValues.push_back(std::make_pair(ptr, N * sizeof(int16_t)));
		break;
	case SavestateType::LONG:
		userValues.push_back(std::make_pair(ptr, N * sizeof(int32_t)));
		break;
	case SavestateType::LLONG:
		userValues.push_back(std::make_pair(ptr, N * sizeof(int64_t)));
		break;
	case SavestateType::BYTE:
		userValues.push_back(std::make_pair(ptr, N * sizeof(uint8_t)));
		break;
	case SavestateType::USHORT:
		userValues.push_back(std::make_pair(ptr, N * sizeof(uint16_t)));
		break;
	case SavestateType::ULONG:
		userValues.push_back(std::make_pair(ptr, N * sizeof(uint32_t)));
		break;
	case SavestateType::ULLONG:
		userValues.push_back(std::make_pair(ptr, N * sizeof(uint64_t)));
		break;
	case SavestateType::FLOAT:
		userValues.push_back(std::make_pair(ptr, N * sizeof(float_t)));
		break;
	case SavestateType::DOUBLE:
		userValues.push_back(std::make_pair(ptr, N * sizeof(double_t)));
		break;
	default:
		break;
	}
}

unsigned int SystemComponent::writeSavestateHeader(std::ofstream &f){
	f.write((char*)&nComponentID, 4);
	f.write((char*)&readOnly, 1);
	f.write((char*)&offset, 2);
	f.write((char*)&nBytes, 2);
	f.write((char*)&nBanks, 2);
	f.write((char*)&bs, 2);
	return 13;
}

unsigned int SystemComponent::readSavestateHeader(std::ifstream &f){
	bool readBackReadOnly;
	unsigned int readBackComponentID;
	unsigned short readBackOffset;
	unsigned short readBackBytes;
	unsigned short readBackBanks;
	f.read((char*)&readBackComponentID, 4);
	f.read((char*)&readBackReadOnly, 1);
	f.read((char*)&readBackOffset, 2);
	f.read((char*)&readBackBytes, 2);
	f.read((char*)&readBackBanks, 2);
	if(
		(readBackComponentID != nComponentID) ||
		(readBackReadOnly != readOnly) ||
		(readBackOffset != offset) ||
		(readBackBytes != nBytes) ||
		(readBackBanks != nBanks))
	{
		std::cout << " [" << sName << "] Warning! Signature of savestate does not match signature for component name=" << sName << std::endl;
		std::cout << " [" << sName << "]  Unstable behavior will likely occur" << std::endl;
	}
	f.read((char*)&bs, 2); // Read the bank select
	return 13;
}

void SystemComponent::resetMemory(const unsigned char& val/* = 0*/){
	if(!size)
		return;
	for(std::vector<std::vector<unsigned char> >::iterator iter = mem.begin(); iter != mem.end(); iter++){
		std::fill(iter->begin(), iter->end(), val);
	}
	bs = 0; // Reset bank select
}

void SystemComponent::fillRandom() {
	for(std::vector<std::vector<unsigned char> >::iterator iter = mem.begin(); iter != mem.end(); iter++){
		for(std::vector<unsigned char>::iterator addr = iter->begin(); addr != iter->end(); addr++){
			*addr = (unsigned char)(rand() % 256);
		}
	}
}

