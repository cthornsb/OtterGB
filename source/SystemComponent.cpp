#include <iostream>
#include <fstream>

#include "Support.hpp"
#include "SystemComponent.hpp"

void SystemComponent::initialize(const unsigned int &nB, const unsigned int &N/*=1*/){
	mem = std::vector<std::vector<unsigned char> >(N, std::vector<unsigned char>(nB, 0x0));
	nBytes = nB;
	nBanks = N;
	bs = 0;
	labs = 0;
	lrel = 0;
	size = nB*N;
}

SystemComponent::~SystemComponent(){
	mem.clear();
}

bool SystemComponent::write(const unsigned int &loc, unsigned char *src){ 
	return write(loc, bs, (*src));
}

bool SystemComponent::write(const unsigned int &loc, const unsigned char &src){ 
	return write(loc, bs, src);
}

bool SystemComponent::write(const unsigned int &loc, const unsigned int &bank, const unsigned char *src){ 
	return write(loc, bank, (*src));
}

bool SystemComponent::write(const unsigned int &loc, const unsigned int &bank, const unsigned char &src){ 
	writeLoc = loc; 
	writeBank = bank;
	writeVal = src;
	if(!preWriteAction())
		return false;
	if(readOnly || nBytes == 0 || (writeLoc-offset) >= nBytes || writeBank >= nBanks)
		return false;
	mem[writeBank][writeLoc-offset] = writeVal;
	postWriteAction();
	return true;		
}	

void SystemComponent::writeNext(unsigned char *src){
	writeNext((*src));
}

void SystemComponent::writeNext(const unsigned char &src){
	writeLoc = lrel;
	writeBank = bs;
	writeVal = src;
	if(!preWriteAction() || readOnly)
		return;
	mem[writeBank][writeLoc] = writeVal;
	incL();
	postWriteAction();
}

bool SystemComponent::read(const unsigned int &loc, unsigned char *dest){
	return read(loc, bs, (*dest));
}

bool SystemComponent::read(const unsigned int &loc, unsigned char &dest){ 
	return read(loc, bs, dest);
}

bool SystemComponent::read(const unsigned int &loc, const unsigned int &bank, unsigned char *dest){ 
	return read(loc, bank, (*dest));
}

bool SystemComponent::read(const unsigned int &loc, const unsigned int &bank, unsigned char &dest){ 
	readLoc = loc;
	readBank = bank;
	if(!preReadAction())
		return false;
	if(nBytes == 0 || (readLoc-offset) >= nBytes || readBank >= nBanks) 
		return false;
	dest = mem[readBank][readLoc-offset];
	postReadAction();
	return true;
}

void SystemComponent::readNext(unsigned char *src){
	readNext((*src));
}

void SystemComponent::readNext(unsigned char &src){
	readLoc = lrel;
	readBank = bs;
	if(!preReadAction())
		return;
	src = mem[readBank][readLoc];
	incL();
	postReadAction();
}

bool SystemComponent::readABS(const unsigned int &loc, unsigned char *dest){
	readABS(loc, (*dest));
}

bool SystemComponent::readABS(const unsigned int &loc, unsigned char &dest){
	/*readLoc = lrel;
	readBank = bs;
	if(!preReadAction())
		return false;
	if(nBytes == 0 || loc >= size) return false;
	dest = mem[readBank][readLoc-offset];
	postReadAction();
	return true;	*/
	return false;
}

void SystemComponent::incL(){
	labs++;
	if(++lrel >= nBytes){ // Switch to the next bank
		if(++bs >= nBanks){ // Wrap around to the 0th bank
			bs = 0;
			labs = 0;
		}
		lrel = 0;
	}
}

void SystemComponent::decL(){
	if(lrel == 0){ // Switch to the next bank
		if(bs == 0){ // Wrap around to the Nth bank
			bs = nBanks-1;
			labs = size-1;
		}
		else labs--;
		lrel = nBytes-1;
	}
	else{
		labs--;
		lrel--;
	}
}
