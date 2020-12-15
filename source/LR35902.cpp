#include <iostream>
#include <stdlib.h>

#include "Support.hpp"
#include "LR35902.hpp"
#include "SystemGBC.hpp"
#include "SystemRegisters.hpp"

const unsigned char FLAG_Z_BIT = 7;
const unsigned char FLAG_S_BIT = 6;
const unsigned char FLAG_H_BIT = 5;
const unsigned char FLAG_C_BIT = 4;

const unsigned char FLAG_Z_MASK = 0x80;
const unsigned char FLAG_S_MASK = 0x40;
const unsigned char FLAG_H_MASK = 0x20;
const unsigned char FLAG_C_MASK = 0x10;

const unsigned char IMMEDIATE_LEFT_BIT  = 0;
const unsigned char IMMEDIATE_RIGHT_BIT = 1;
const unsigned char ADDRESS_LEFT_BIT    = 2;
const unsigned char ADDRESS_RIGHT_BIT   = 3;

/** Read the next instruction from memory and return the number of clock cycles. 
  */
unsigned short LR35902::evaluate(){
	// Read an opcode
	unsigned char op;
	if(!sys->read(PC++, &op))
		std::cout << " Opcode read failed! PC=" << getHex((unsigned short)(PC-1)) << std::endl;
	
	if(op != 0xCB) // Normal opcodes
		lastOpcode.set(opcodes.getOpcodes(), op, PC-1);
	else{ // CB prefix opcodes
		sys->read(PC++, &op);
		lastOpcode.set(opcodes.getOpcodesCB(), op, PC-2);
	}

	d8 = 0x0;
	d16h = 0x0;
	d16l = 0x0;

	// Read the opcode's accompanying value (if any)
	if(lastOpcode()->nBytes == 2){ // Read 8 bits (valid targets: d8, d8, d8)
		sys->read(PC++, d8);
		lastOpcode.setImmediateData(d8);
	}
	else if(lastOpcode()->nBytes == 3){ // Read 16 bits (valid targets: d16, d16)
		// Low byte read first!
		sys->read(PC++, d16l);
		sys->read(PC++, d16h);
		lastOpcode.setImmediateData(getUShort(d16h, d16l));
	}
	
	// Set the memory read/write address (if any)
	if(lastOpcode()->addrptr) // Set memory address
		memoryAddress = (this->*lastOpcode()->addrptr)();

	return lastOpcode.nCycles;
}

/** Perform one CPU (machine) cycle of the current instruction.
  * @return True if the current instruction has completed execution (i.e. nCyclesRemaining==0).
  */
bool LR35902::onClockUpdate(){
	if(!lastOpcode.executing()){ // Previous instruction finished executing, read the next one.
		// Check for pending interrupts.
		if(!rIME->zero() && ((*rIE) & (*rIF))){
			if(rIF->getBit(0)) // VBlank
				acknowledgeVBlankInterrupt();
			if(rIF->getBit(1)) // LCDC STAT
				acknowledgeLcdInterrupt();
			if(rIF->getBit(2)) // Timer
				acknowledgeTimerInterrupt();
			if(rIF->getBit(3)) // Serial
				acknowledgeSerialInterrupt();
			if(rIF->getBit(4)) // Joypad
				acknowledgeJoypadInterrupt();
		}
		evaluate();
	}
	return lastOpcode.clock(this); // Execute the instruction on the last cycle
}

void LR35902::acknowledgeVBlankInterrupt(){
	rIF->resetBit(0);
	if(rIE->getBit(0)){ // Execute interrupt
		(*rIME) = 0;
		callInterruptVector(0x40);
	}
}

void LR35902::acknowledgeLcdInterrupt(){
	rIF->resetBit(1);
	if(rIE->getBit(1)){ // Execute interrupt
		(*rIME) = 0;
		callInterruptVector(0x48);
	}
}

void LR35902::acknowledgeTimerInterrupt(){
	rIF->resetBit(2);
	if(rIE->getBit(2)){ // Execute interrupt
		(*rIME) = 0;
		callInterruptVector(0x50);
	}
}

void LR35902::acknowledgeSerialInterrupt(){
	rIF->resetBit(3);
	if(rIE->getBit(3)){ // Execute interrupt
		(*rIME) = 0;
		callInterruptVector(0x58);
	}
}

void LR35902::acknowledgeJoypadInterrupt(){
	rIF->resetBit(4);
	if(rIE->getBit(4)){ // Execute interrupt
		(*rIME) = 0;
		callInterruptVector(0x60);
	}
}

void LR35902::setFlag(const unsigned char &bit, bool state/*=true*/){
	if(state) set_d8(&F, bit);
	else      res_d8(&F, bit);
}

void LR35902::setFlags(bool zflag, bool sflag, bool hflag, bool cflag){
	setFlag(FLAG_Z_BIT, zflag);
	setFlag(FLAG_S_BIT, sflag);
	setFlag(FLAG_H_BIT, hflag);
	setFlag(FLAG_C_BIT, cflag);
}

unsigned short LR35902::getd16() const { return getUShort(d16h, d16l); }

unsigned short LR35902::getAF() const { return getUShort(A, F); }

unsigned short LR35902::getBC() const { return getUShort(B, C); }

unsigned short LR35902::getDE() const { return getUShort(D, E); }

unsigned short LR35902::getHL() const { return getUShort(H, L); }

void LR35902::setd16(const unsigned short &val){
	d16h = (0xFF00 & val) >> 8;
	d16l = 0x00FF & val;
}

void LR35902::setAF(const unsigned short &val){
	A = (0xFF00 & val) >> 8;
	F = 0x00FF & val;
	F &= 0xF0; // Bottom 4 bits of F are always zero
}

void LR35902::setBC(const unsigned short &val){
	B = (0xFF00 & val) >> 8;
	C = 0x00FF & val;
}

void LR35902::setDE(const unsigned short &val){
	D = (0xFF00 & val) >> 8;
	E = 0x00FF & val;
}

void LR35902::setHL(const unsigned short &val){
	H = (0xFF00 & val) >> 8;
	L = 0x00FF & val;
}

bool LR35902::getRegister8bit(const std::string& name, unsigned char& val){
	auto reg = rget8.find(name);
	if(reg == rget8.end())
		return false;
	val = (this->*reg->second)();
	return true;
}

bool LR35902::getRegister16bit(const std::string& name, unsigned short& val){
	auto reg = rget16.find(name);
	if(reg == rget16.end())
		return false;
	val = (this->*reg->second)();
	return true;
}

unsigned char* LR35902::getPointerToRegister8bit(const std::string& name) {
	if (name == "a") {
		return &A;
	}
	else if (name == "b") {
		return &B;
	}
	else if (name == "c") {
		return &C;
	}
	else if (name == "d") {
		return &D;
	}
	else if (name == "e") {
		return &E;
	}
	else if (name == "f") {
		return &F;
	}
	else if (name == "h") {
		return &H;
	}
	else if (name == "l") {
		return &L;
	}
	return 0x0;
}

unsigned short* LR35902::getPointerToRegister16bit(const std::string& name) {
	if (name == "af") {
		return 0x0;
	}
	else if (name == "bc") {
		return 0x0;
	}
	else if (name == "de") {
		return 0x0;
	}
	else if (name == "hl") {
		return 0x0;
	}
	else if (name == "pc") {
		return &PC;
	}
	else if (name == "sp") {
		return &SP;
	}
	return 0x0;
}

bool LR35902::setRegister8bit(const std::string& name, const unsigned char& val){
	auto reg = rset8.find(name);
	if(reg == rset8.end())
		return false;
	(this->*reg->second)(val);
	return true;
}

bool LR35902::setRegister16bit(const std::string& name, const unsigned short& val){
	auto reg = rset16.find(name);
	if(reg == rset16.end())
		return false;
	(this->*reg->second)(val);
	return true;
}

void LR35902::readMemory(){
	sys->read(memoryAddress, memoryValue);
}

void LR35902::writeMemory(){
	sys->write(memoryAddress, memoryValue);
}

addrGetFunc LR35902::getMemoryAddressFunction(const std::string &target){
	if(target.find("bc") != std::string::npos)
		return &LR35902::getBC;
	else if(target.find("de") != std::string::npos)
		return &LR35902::getDE;
	else if(target.find("hl") != std::string::npos)
		return &LR35902::getHL;
	else if(target.find("c") != std::string::npos)
		return &LR35902::getAddress_C;
	else if(target.find("a8") != std::string::npos)
		return &LR35902::getAddress_d8;
	else if(target.find("a16") != std::string::npos)
		return &LR35902::getd16;
	else
		std::cout << " Warning! Unrecognized memory target (" << target << ")\n";
	return 0x0;
}

bool LR35902::findOpcode(const std::string& mnemonic, OpcodeData& data) {
	if (opcodes.findOpcode(mnemonic, data)) {
		// Set data access memory address (if present)
		if (data.memoryAccess())
			setMemoryAddress((this->*data()->addrptr)());
		return true;
	}
	return false;
}

void LR35902::rlc_d8(unsigned char *arg){
	// Rotate (arg) left, copy old 7th bit into the carry.
	bool highBit = ((*arg) & 0x80) == 0x80;
	(*arg) = (*arg) << 1;
	if(highBit) set_d8(arg, 0);
	else        res_d8(arg, 0);
	setFlags((*arg == 0), 0, 0, highBit);
}

void LR35902::rrc_d8(unsigned char *arg){
	// Rotate (arg) right, copy old 0th bit into the carry.
	bool lowBit = ((*arg) & 0x1) == 0x1;
	(*arg) = (*arg) >> 1;
	if(lowBit) set_d8(arg, 7);
	else       res_d8(arg, 7);
	setFlags((*arg == 0), 0, 0, lowBit);
}

void LR35902::rl_d8(unsigned char *arg){
	// Rotate (arg) left through carry.
	bool highBit = ((*arg) & 0x80) == 0x80;
	(*arg) = (*arg) << 1;
	if(getFlagC()) set_d8(arg, 0);
	else           res_d8(arg, 0);
	setFlags((*arg == 0), 0, 0, highBit);
}

void LR35902::rr_d8(unsigned char *arg){
	// Rotate (arg) right through carry.
	bool lowBit = ((*arg) & 0x1) == 0x1;
	(*arg) = (*arg) >> 1;
	if(getFlagC()) set_d8(arg, 7);
	else           res_d8(arg, 7);
	setFlags((*arg == 0), 0, 0, lowBit);
}

void LR35902::res_d8(unsigned char *arg, const unsigned char &bit){
	// Clear a bit of (arg).
	(*arg) &= (~(0x1 << bit));
}

void LR35902::set_d8(unsigned char *arg, const unsigned char &bit){
	// Set a bit of (arg).
	(*arg) |= (0x1 << bit);
}

void LR35902::bit_d8(const unsigned char &arg, const unsigned char &bit){
	// Copy a bit of (arg) to the zero bit.
	setFlag(FLAG_Z_BIT, (arg & (0x1 << bit)) == 0);
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, 1);
}

void LR35902::inc_d16(unsigned char *addrH, unsigned char *addrL){
	if(++(*addrL) == 0x0) // Carry from 7th bit (full)
		(*addrH)++;
}

void LR35902::dec_d16(unsigned char *addrH, unsigned char *addrL){
	if(--(*addrL) == 0xFF) // Carry from 7th bit (full)
		(*addrH)--;
}

void LR35902::inc_d8(unsigned char *arg){
	(*arg) = getCarriesAdd(*arg, 1);
	setFlag(FLAG_Z_BIT, (*arg == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, halfCarry);
}

void LR35902::dec_d8(unsigned char *arg){
	(*arg) = getCarriesSub(*arg, 1);
	setFlag(FLAG_Z_BIT, (*arg == 0));
	setFlag(FLAG_S_BIT, 1);
	setFlag(FLAG_H_BIT, halfCarry);
}

void LR35902::jr_n(const unsigned char &n){
	PC += twosComp(n);
}

void LR35902::jr_cc_n(const unsigned char &n){
	lastOpcode.addCycles(1); // Conditional JR takes 4 additional cycles if true (8->12)
	jr_n(n);
}

void LR35902::ld_d8(unsigned char *dest, const unsigned char &src){
	(*dest) = src;
}

void LR35902::ld_SP_d16(const unsigned char &addrH, const unsigned char &addrL){
	// Load immediate 16-bit value into the stack pointer, SP.
	SP = getUShort(addrH, addrL);
}

void LR35902::add_HL_d16(const unsigned char &addrH, const unsigned char &addrL){
	// Add d16 to HL.
	unsigned short HL = getHL();
	unsigned short dd = getUShort(addrH, addrL);
	unsigned int result = HL + dd;
	// Note: (arg1 ^ arg2) ^ result = carry bits
	halfCarry = (((HL ^ dd) ^ result) & 0x1000) == 0x1000; // Carry from bit 11
	fullCarry = (((HL ^ dd) ^ result) & 0x10000) == 0x10000; // Carry from bit 15	
	setHL(result & 0xFFFF);
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

void LR35902::add_A_d8(const unsigned char &arg){
	A = getCarriesAdd(A, arg);
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

void LR35902::adc_A_d8(const unsigned char &arg){
	A = getCarriesAdd(A, arg, true); // Add WITH carry
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

void LR35902::sub_A_d8(const unsigned char &arg){
	A = getCarriesSub(A, arg);
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 1);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

void LR35902::sbc_A_d8(const unsigned char &arg){
	A = getCarriesSub(A, arg, true); // Subtract WITH carry
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 1);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

void LR35902::and_d8(const unsigned char &arg){
	A &= arg;
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, 1);
	setFlag(FLAG_C_BIT, 0);		
}

void LR35902::xor_d8(const unsigned char &arg){
	A ^= arg;
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, 0);
	setFlag(FLAG_C_BIT, 0);	
}

void LR35902::or_d8(const unsigned char &arg){
	A |= arg;
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, 0);
	setFlag(FLAG_C_BIT, 0);
}

void LR35902::cp_d8(const unsigned char &arg){
	getCarriesSub(A, arg);
	setFlag(FLAG_Z_BIT, (A == arg)); // A == arg
	setFlag(FLAG_S_BIT, 1);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry); // A < arg
}

void LR35902::push_d16(const unsigned char &addrH, const unsigned char &addrL){
	sys->write(SP-1, addrH);
	sys->write(SP-2, addrL);
	DEC_SP();
	DEC_SP();
}

void LR35902::push_d16(const unsigned short &addr){
	unsigned char addrH = (addr & 0xFF00) >> 8;
	unsigned char addrL = (addr & 0x00FF);
	push_d16(addrH, addrL);
}

void LR35902::pop_d16(unsigned char *addrH, unsigned char *addrL){
	sys->read(SP, addrL);
	sys->read(SP+1, addrH);
	INC_SP();
	INC_SP();
}

void LR35902::pop_d16(unsigned short *addr){
	unsigned char addrH = ((*addr) & 0xFF00) >> 8;
	unsigned char addrL = ((*addr) & 0x00FF);
	pop_d16(&addrH, &addrL);
	(*addr) = getUShort(addrH, addrL);
}

void LR35902::jp_d16(const unsigned char &addrH, const unsigned char &addrL){
	// Jump to address
	PC = getUShort(addrH, addrL);
}

void LR35902::jp_cc_d16(const unsigned char &addrH, const unsigned char &addrL){
	lastOpcode.addCycles(1); // Conditional JP takes 4 additional cycles if true (12->16)
	jp_d16(addrH, addrL);
}

void LR35902::call_a16(const unsigned char &addrH, const unsigned char &addrL){
	push_d16(PC); // Push the program counter onto the stack
	jp_d16(addrH, addrL); // Jump to the called address
}

void LR35902::call_cc_a16(const unsigned char &addrH, const unsigned char &addrL){
	lastOpcode.addCycles(3); // Conditional CALL takes 12 additional cycles if true (12->24)
	call_a16(addrH, addrL);
}

void LR35902::rst_n(const unsigned char &n){
	push_d16(PC); // Push the program counter onto the stack
	PC = n & 0x00FF; // Zero the high bits
}

void LR35902::ret(){
	pop_d16(&PC); // Pop the program counter off the stack
}

void LR35902::ret_cc(){
	lastOpcode.addCycles(3); // Conditional RET takes 12 additional cycles if true (8->20)
	ret();
}

unsigned char LR35902::getCarriesAdd(const unsigned char &arg1, const unsigned char &arg2, bool adc/*=false*/){ // ADD - ADC
	// Note: (arg1 ^ arg2) ^ result = carry bits
	unsigned short result = arg1 + (arg2 + (adc & getFlagC() ? 1: 0));
	halfCarry = (((arg1 ^ arg2) ^ result) & 0x10) == 0x10; // Carry from bit 3
	fullCarry = (((arg1 ^ arg2) ^ result) & 0x100) == 0x100; // Carry from bit 7
	return (result & 0x00FF); // Return the lower 8 bits of the result.
}

unsigned char LR35902::getCarriesSub(const unsigned char &arg1, const unsigned char &arg2, bool sbc/*=false*/){ // SUB - SBC
	// Note: (arg1 ^ arg2) ^ result = borrow bits
	unsigned short result = arg1 - (arg2 + (sbc & getFlagC() ? 1 : 0));
	halfCarry = (((arg1 ^ arg2) ^ result) & 0x10) == 0x10; // Borrow from bit 4
	fullCarry = (((arg1 ^ arg2) ^ result) & 0x100) == 0x100; // Borrow from bit 8
	return (result & 0x00FF); // Return the lower 8 bits of the result.
}

void LR35902::sla_d8(unsigned char *arg){
	// Left bitshift (arg), setting its 0th bit to 0 and moving
	// its 7th bit into the carry bit.
	bool highBit = ((*arg) & 0x80) == 0x80;
	(*arg) = (*arg) << 1;
	setFlags((*arg == 0), 0, 0, highBit);
}

void LR35902::sra_d8(unsigned char *arg){
	// Right bitshift (arg), its 0th bit is moved to the carry
	// and its 7th bit retains its original value.
	bool lowBit = ((*arg) & 0x1) == 0x1;
	bool highBit = ((*arg) & 0x80) == 0x80;
	(*arg) = (*arg) >> 1;
	if(highBit)	set_d8(arg, 7);
	setFlags((*arg == 0), 0, 0, lowBit);
}

void LR35902::srl_d8(unsigned char *arg){
	// Right bitshift (arg), setting its 7th bit to 0 and moving
	// its 0th bit into the carry bit.
	bool lowBit = ((*arg) & 0x1) == 0x1;
	(*arg) = (*arg) >> 1;
	setFlags((*arg == 0), 0, 0, lowBit);
}

void LR35902::swap_d8(unsigned char *arg){
	// Swap the low and high nibbles (4-bits) of (arg).
	unsigned char lowNibble = (*arg) & 0xF;
	unsigned char highNibble = (*arg) & 0xF0;
	(*arg) = (lowNibble << 4) + (highNibble >> 4);
	setFlags((*arg == 0), 0, 0, 0);
}

void LR35902::callInterruptVector(const unsigned char &offset){
	// Interrupt vectors behave the same way that RST vectors do
	rst_n(offset);
}

unsigned int LR35902::writeSavestate(std::ofstream &f){
	f.write((char*)&A, 1);
	f.write((char*)&B, 1);
	f.write((char*)&C, 1);
	f.write((char*)&D, 1);
	f.write((char*)&E, 1);
	f.write((char*)&H, 1);
	f.write((char*)&L, 1);
	f.write((char*)&F, 1);
	f.write((char*)&SP, 2);
	f.write((char*)&PC, 2);
	return 12;
}

unsigned int LR35902::readSavestate(std::ifstream &f){
	f.read((char*)&A, 1);
	f.read((char*)&B, 1);
	f.read((char*)&C, 1);
	f.read((char*)&D, 1);
	f.read((char*)&E, 1);
	f.read((char*)&H, 1);
	f.read((char*)&L, 1);
	f.read((char*)&F, 1);
	f.read((char*)&SP, 2);
	f.read((char*)&PC, 2);
	return 12;
}

/////////////////////////////////////////////////////////////////////
// OPCODES
/////////////////////////////////////////////////////////////////////

// RLCA | RLCA | RRA | RRCA

void LR35902::RLA(){ 
	rl_d8(&A); 
	setFlag(FLAG_Z_BIT, 0); // RLA clears Z regardless of result.
}

void LR35902::RLCA(){ 
	rlc_d8(&A);
	setFlag(FLAG_Z_BIT, 0); // RLCA clears Z regardless of result.
}

void LR35902::RRA(){ 
	rr_d8(&A); 
	setFlag(FLAG_Z_BIT, 0); // RRA clears Z regardless of result.
}

void LR35902::RRCA(){ 
	rrc_d8(&A); 
	setFlag(FLAG_Z_BIT, 0); // RRCA clears Z regardless of result.
}

// DAA

void LR35902::DAA(){
	// Decimal adjust register A.
	// Convert register A into packed Binary Coded Decimal (BCD) representation.
	// One byte in packed BCD may represent values between 0 and 99 (inclusive).	
	if(!getFlagS()){ // After addition
		if(getFlagC() || (A > 0x99)){ // Value overflow (>= 0x100)
			A += 0x60;
			setFlag(FLAG_C_BIT, 1);
		}
		if(getFlagH() || ((A & 0x0F) > 0x9)) // 
			A += 0x6;
	}
	else{ // After subtraction
		if(getFlagC()){
			A -= 0x60;
			setFlag(FLAG_C_BIT, 1);
		}
		if(getFlagH())
			A -= 0x6;
	}
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_H_BIT, 0);
}

// CPL

void LR35902::CPL(){ 
	A = ~A; 
	setFlag(FLAG_S_BIT, 1);
	setFlag(FLAG_H_BIT, 1);
}

// SCF

void LR35902::SCF(){ 
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, 0);
	setFlag(FLAG_C_BIT, 1);
}

// CCF

void LR35902::CCF(){ 
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, 0);
	setFlag(FLAG_C_BIT, !getFlagC()); 
}

// ADD SP,d8

void LR35902::ADD_SP_r8(){
	// Add d8 (signed) to the stack pointer
	short r8 = twosComp(d8);
	unsigned short res = SP + r8;
	halfCarry = (((SP ^ r8) ^ res) & 0x10) == 0x10;
	fullCarry = (((SP ^ r8) ^ res) & 0x100) == 0x100;
	SP = res;
	setFlags(0, 0, halfCarry, fullCarry);
}

// LD HL,SP+r8 or LDHL SP,r8

void LR35902::LD_HL_SP_r8(){
	// Load the address (SP+r8) into HL, note: r8 is signed.
	short r8 = twosComp(d8);
	unsigned short res = SP + r8;
	halfCarry = (((SP ^ r8) ^ res) & 0x10) == 0x10;
	fullCarry = (((SP ^ r8) ^ res) & 0x100) == 0x100;
	setHL(SP + r8);
	setFlags(0, 0, halfCarry, fullCarry);
}

// LD HL,d16

void LR35902::LD_HL_d16(){
	// Load 16-bit immediate value into register HL.
	H = d16h;
	L = d16l;
}	

// LD d16 SP

void LR35902::LD_a16_SP(){
	// Write stack pointer to memory location (a16).
	unsigned short target = getd16();
	sys->write(target, (0x00FF & SP)); // Write the low byte first
	sys->write(target+1, (0xFF00 &SP) >> 8); // Write the high byte last
}

// ADD HL,BC[DE|HL|SP]

void LR35902::ADD_HL_SP(){
	// Add SP to HL.
	unsigned short HL = getHL();
	unsigned int result = HL + SP;
	// Note: (arg1 ^ arg2) ^ result = carry bits
	halfCarry = (((HL ^ SP) ^ result) & 0x1000) == 0x1000; // Carry from bit 11
	fullCarry = (((HL ^ SP) ^ result) & 0x10000) == 0x10000; // Carry from bit 15	
	setHL(result & 0xFFFF);
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

// LD HL-,A or LDD HL,A

void LR35902::LDD_aHL_A(){
	// Write register A to memory address (HL) and decrement HL.
	memoryValue = A;
	DEC_HL();
}

// LD A,HL- or LDD A,HL

void LR35902::LDD_A_aHL(){
	// Write memory address (HL) into register A and decrement HL.
	A = memoryValue;
	DEC_HL();
}

// LD HL+,A or LDI HL,A

void LR35902::LDI_aHL_A(){
	// Write register A to memory address (HL) and increment HL.
	memoryValue = A;
	INC_HL();
}

// LD A,HL+ or LDI A,HL

void LR35902::LDI_A_aHL(){
	// Write memory address (HL) into register A and increment HL.
	A = memoryValue;
	INC_HL();
}

/*void LR35902::LD_aHL_d16(){
	// Write d16 into memory location (HL).
	unsigned short val = getd16();
	unsigned short HL = getHL();
	sys->write(HL, (0x00FF & val)); // Write the low byte first
	sys->write(HL+1, (0xFF00 & val) >> 8); // Write the high byte last		
}*/

// LD BC,A[d16]

void LR35902::LD_BC_d16(){
	// Load 16-bit immediate value into register BC.
	B = d16h;
	C = d16l;
}

// LD DE,A[d16]

void LR35902::LD_DE_d16(){
	// Load 16-bit immediate value into register DE.
	D = d16h;
	E = d16l;
}

// POP AF

void LR35902::POP_AF(){ 
	pop_d16(&A, &F);
	F &= 0xF0; // Bottom 4 bits of F are always zero
}

// RET NZ[Z|NC|C]

void LR35902::RETI(){
	EI();
	ret();
}

// DI

void LR35902::DI(){ 
	sys->disableInterrupts(); 
}

// EI

void LR35902::EI(){ 
	sys->enableInterrupts(); 
}

// STOP 0

void LR35902::STOP_0(){ 
	sys->stopCPU();
}

// HALT

void LR35902::HALT(){ 
	sys->haltCPU();
}

void LR35902::initialize(){
	// 8 bit register getters
	rget8["a"] = &LR35902::getA;
	rget8["b"] = &LR35902::getB;
	rget8["c"] = &LR35902::getC;
	rget8["d"] = &LR35902::getD;
	rget8["e"] = &LR35902::getE;
	rget8["f"] = &LR35902::getF;
	rget8["h"] = &LR35902::getH;
	rget8["l"] = &LR35902::getL;
	rget8["d8"] = &LR35902::getd8;
	
	// 16 bit register getters
	rget16["af"] = &LR35902::getAF;
	rget16["bc"] = &LR35902::getBC;
	rget16["de"] = &LR35902::getDE;
	rget16["hl"] = &LR35902::getHL;
	rget16["pc"] = &LR35902::getProgramCounter;
	rget16["sp"] = &LR35902::getStackPointer;
	rget16["d16"] = &LR35902::getd16;
	
	// 8 bit register setters
	rset8["a"] = &LR35902::setA;
	rset8["b"] = &LR35902::setB;
	rset8["c"] = &LR35902::setC;
	rset8["d"] = &LR35902::setD;
	rset8["e"] = &LR35902::setE;
	rset8["f"] = &LR35902::setF;
	rset8["h"] = &LR35902::setH;
	rset8["l"] = &LR35902::setL;
	rset8["d8"] = &LR35902::setd8;
	
	// 16 bit register setters
	rset16["af"] = &LR35902::setAF;
	rset16["bc"] = &LR35902::setBC;
	rset16["de"] = &LR35902::setDE;
	rset16["hl"] = &LR35902::setHL;
	rset16["pc"] = &LR35902::setProgramCounter;
	rset16["sp"] = &LR35902::setStackPointer;
	rset16["d16"] = &LR35902::setd16;

	// Standard opcodes
	opcodes.setOpcodePointer(0, &LR35902::NOP);
	opcodes.setOpcodePointer(1, &LR35902::LD_BC_d16);
	opcodes.setOpcodePointer(2, &LR35902::LD_aBC_A);
	opcodes.setOpcodePointer(3, &LR35902::INC_BC);
	opcodes.setOpcodePointer(4, &LR35902::INC_B);
	opcodes.setOpcodePointer(5, &LR35902::DEC_B);
	opcodes.setOpcodePointer(6, &LR35902::LD_B_d8);
	opcodes.setOpcodePointer(7, &LR35902::RLCA);
	opcodes.setOpcodePointer(8, &LR35902::LD_a16_SP);
	opcodes.setOpcodePointer(9, &LR35902::ADD_HL_BC);
	opcodes.setOpcodePointer(10, &LR35902::LD_A_aBC);
	opcodes.setOpcodePointer(11, &LR35902::DEC_BC);
	opcodes.setOpcodePointer(12, &LR35902::INC_C);
	opcodes.setOpcodePointer(13, &LR35902::DEC_C);
	opcodes.setOpcodePointer(14, &LR35902::LD_C_d8);
	opcodes.setOpcodePointer(15, &LR35902::RRCA);
	opcodes.setOpcodePointer(16, &LR35902::STOP_0);
	opcodes.setOpcodePointer(17, &LR35902::LD_DE_d16);
	opcodes.setOpcodePointer(18, &LR35902::LD_aDE_A);
	opcodes.setOpcodePointer(19, &LR35902::INC_DE);
	opcodes.setOpcodePointer(20, &LR35902::INC_D);
	opcodes.setOpcodePointer(21, &LR35902::DEC_D);
	opcodes.setOpcodePointer(22, &LR35902::LD_D_d8);
	opcodes.setOpcodePointer(23, &LR35902::RLA);
	opcodes.setOpcodePointer(24, &LR35902::JR_r8);
	opcodes.setOpcodePointer(25, &LR35902::ADD_HL_DE);
	opcodes.setOpcodePointer(26, &LR35902::LD_A_aDE);
	opcodes.setOpcodePointer(27, &LR35902::DEC_DE);
	opcodes.setOpcodePointer(28, &LR35902::INC_E);
	opcodes.setOpcodePointer(29, &LR35902::DEC_E);
	opcodes.setOpcodePointer(30, &LR35902::LD_E_d8);
	opcodes.setOpcodePointer(31, &LR35902::RRA);
	opcodes.setOpcodePointer(32, &LR35902::JR_NZ_r8);
	opcodes.setOpcodePointer(33, &LR35902::LD_HL_d16);
	opcodes.setOpcodePointer(34, &LR35902::LDI_aHL_A);
	opcodes.setOpcodePointer(35, &LR35902::INC_HL);
	opcodes.setOpcodePointer(36, &LR35902::INC_H);
	opcodes.setOpcodePointer(37, &LR35902::DEC_H);
	opcodes.setOpcodePointer(38, &LR35902::LD_H_d8);
	opcodes.setOpcodePointer(39, &LR35902::DAA);
	opcodes.setOpcodePointer(40, &LR35902::JR_Z_r8);
	opcodes.setOpcodePointer(41, &LR35902::ADD_HL_HL);
	opcodes.setOpcodePointer(42, &LR35902::LDI_A_aHL);
	opcodes.setOpcodePointer(43, &LR35902::DEC_HL);
	opcodes.setOpcodePointer(44, &LR35902::INC_L);
	opcodes.setOpcodePointer(45, &LR35902::DEC_L);
	opcodes.setOpcodePointer(46, &LR35902::LD_L_d8);
	opcodes.setOpcodePointer(47, &LR35902::CPL);
	opcodes.setOpcodePointer(48, &LR35902::JR_NC_r8);
	opcodes.setOpcodePointer(49, &LR35902::LD_SP_d16);
	opcodes.setOpcodePointer(50, &LR35902::LDD_aHL_A);
	opcodes.setOpcodePointer(51, &LR35902::INC_SP);
	opcodes.setOpcodePointer(52, &LR35902::INC_aHL);
	opcodes.setOpcodePointer(53, &LR35902::DEC_aHL);
	opcodes.setOpcodePointer(54, &LR35902::LD_aHL_d8);
	opcodes.setOpcodePointer(55, &LR35902::SCF);
	opcodes.setOpcodePointer(56, &LR35902::JR_C_r8);
	opcodes.setOpcodePointer(57, &LR35902::ADD_HL_SP);
	opcodes.setOpcodePointer(58, &LR35902::LDD_A_aHL);
	opcodes.setOpcodePointer(59, &LR35902::DEC_SP);
	opcodes.setOpcodePointer(60, &LR35902::INC_A);
	opcodes.setOpcodePointer(61, &LR35902::DEC_A);
	opcodes.setOpcodePointer(62, &LR35902::LD_A_d8);
	opcodes.setOpcodePointer(63, &LR35902::CCF);
	opcodes.setOpcodePointer(64, &LR35902::LD_B_B);
	opcodes.setOpcodePointer(65, &LR35902::LD_B_C);
	opcodes.setOpcodePointer(66, &LR35902::LD_B_D);
	opcodes.setOpcodePointer(67, &LR35902::LD_B_E);
	opcodes.setOpcodePointer(68, &LR35902::LD_B_H);
	opcodes.setOpcodePointer(69, &LR35902::LD_B_L);
	opcodes.setOpcodePointer(70, &LR35902::LD_B_aHL);
	opcodes.setOpcodePointer(71, &LR35902::LD_B_A);
	opcodes.setOpcodePointer(72, &LR35902::LD_C_B);
	opcodes.setOpcodePointer(73, &LR35902::LD_C_C);
	opcodes.setOpcodePointer(74, &LR35902::LD_C_D);
	opcodes.setOpcodePointer(75, &LR35902::LD_C_E);
	opcodes.setOpcodePointer(76, &LR35902::LD_C_H);
	opcodes.setOpcodePointer(77, &LR35902::LD_C_L);
	opcodes.setOpcodePointer(78, &LR35902::LD_C_aHL);
	opcodes.setOpcodePointer(79, &LR35902::LD_C_A);
	opcodes.setOpcodePointer(80, &LR35902::LD_D_B);
	opcodes.setOpcodePointer(81, &LR35902::LD_D_C);
	opcodes.setOpcodePointer(82, &LR35902::LD_D_D);
	opcodes.setOpcodePointer(83, &LR35902::LD_D_E);
	opcodes.setOpcodePointer(84, &LR35902::LD_D_H);
	opcodes.setOpcodePointer(85, &LR35902::LD_D_L);
	opcodes.setOpcodePointer(86, &LR35902::LD_D_aHL);
	opcodes.setOpcodePointer(87, &LR35902::LD_D_A);
	opcodes.setOpcodePointer(88, &LR35902::LD_E_B);
	opcodes.setOpcodePointer(89, &LR35902::LD_E_C);
	opcodes.setOpcodePointer(90, &LR35902::LD_E_D);
	opcodes.setOpcodePointer(91, &LR35902::LD_E_E);
	opcodes.setOpcodePointer(92, &LR35902::LD_E_H);
	opcodes.setOpcodePointer(93, &LR35902::LD_E_L);
	opcodes.setOpcodePointer(94, &LR35902::LD_E_aHL);
	opcodes.setOpcodePointer(95, &LR35902::LD_E_A);
	opcodes.setOpcodePointer(96, &LR35902::LD_H_B);
	opcodes.setOpcodePointer(97, &LR35902::LD_H_C);
	opcodes.setOpcodePointer(98, &LR35902::LD_H_D);
	opcodes.setOpcodePointer(99, &LR35902::LD_H_E);
	opcodes.setOpcodePointer(100, &LR35902::LD_H_H);
	opcodes.setOpcodePointer(101, &LR35902::LD_H_L);
	opcodes.setOpcodePointer(102, &LR35902::LD_H_aHL);
	opcodes.setOpcodePointer(103, &LR35902::LD_H_A);
	opcodes.setOpcodePointer(104, &LR35902::LD_L_B);
	opcodes.setOpcodePointer(105, &LR35902::LD_L_C);
	opcodes.setOpcodePointer(106, &LR35902::LD_L_D);
	opcodes.setOpcodePointer(107, &LR35902::LD_L_E);
	opcodes.setOpcodePointer(108, &LR35902::LD_L_H);
	opcodes.setOpcodePointer(109, &LR35902::LD_L_L);
	opcodes.setOpcodePointer(110, &LR35902::LD_L_aHL);
	opcodes.setOpcodePointer(111, &LR35902::LD_L_A);
	opcodes.setOpcodePointer(112, &LR35902::LD_aHL_B);
	opcodes.setOpcodePointer(113, &LR35902::LD_aHL_C);
	opcodes.setOpcodePointer(114, &LR35902::LD_aHL_D);
	opcodes.setOpcodePointer(115, &LR35902::LD_aHL_E);
	opcodes.setOpcodePointer(116, &LR35902::LD_aHL_H);
	opcodes.setOpcodePointer(117, &LR35902::LD_aHL_L);
	opcodes.setOpcodePointer(118, &LR35902::HALT);
	opcodes.setOpcodePointer(119, &LR35902::LD_aHL_A);
	opcodes.setOpcodePointer(120, &LR35902::LD_A_B);
	opcodes.setOpcodePointer(121, &LR35902::LD_A_C);
	opcodes.setOpcodePointer(122, &LR35902::LD_A_D);
	opcodes.setOpcodePointer(123, &LR35902::LD_A_E);
	opcodes.setOpcodePointer(124, &LR35902::LD_A_H);
	opcodes.setOpcodePointer(125, &LR35902::LD_A_L);
	opcodes.setOpcodePointer(126, &LR35902::LD_A_aHL);
	opcodes.setOpcodePointer(127, &LR35902::LD_A_A);
	opcodes.setOpcodePointer(128, &LR35902::ADD_A_B);
	opcodes.setOpcodePointer(129, &LR35902::ADD_A_C);
	opcodes.setOpcodePointer(130, &LR35902::ADD_A_D);
	opcodes.setOpcodePointer(131, &LR35902::ADD_A_E);
	opcodes.setOpcodePointer(132, &LR35902::ADD_A_H);
	opcodes.setOpcodePointer(133, &LR35902::ADD_A_L);
	opcodes.setOpcodePointer(134, &LR35902::ADD_A_aHL);
	opcodes.setOpcodePointer(135, &LR35902::ADD_A_A);
	opcodes.setOpcodePointer(136, &LR35902::ADC_A_B);
	opcodes.setOpcodePointer(137, &LR35902::ADC_A_C);
	opcodes.setOpcodePointer(138, &LR35902::ADC_A_D);
	opcodes.setOpcodePointer(139, &LR35902::ADC_A_E);
	opcodes.setOpcodePointer(140, &LR35902::ADC_A_H);
	opcodes.setOpcodePointer(141, &LR35902::ADC_A_L);
	opcodes.setOpcodePointer(142, &LR35902::ADC_A_aHL);
	opcodes.setOpcodePointer(143, &LR35902::ADC_A_A);
	opcodes.setOpcodePointer(144, &LR35902::SUB_B);
	opcodes.setOpcodePointer(145, &LR35902::SUB_C);
	opcodes.setOpcodePointer(146, &LR35902::SUB_D);
	opcodes.setOpcodePointer(147, &LR35902::SUB_E);
	opcodes.setOpcodePointer(148, &LR35902::SUB_H);
	opcodes.setOpcodePointer(149, &LR35902::SUB_L);
	opcodes.setOpcodePointer(150, &LR35902::SUB_aHL);
	opcodes.setOpcodePointer(151, &LR35902::SUB_A);
	opcodes.setOpcodePointer(152, &LR35902::SBC_A_B);
	opcodes.setOpcodePointer(153, &LR35902::SBC_A_C);
	opcodes.setOpcodePointer(154, &LR35902::SBC_A_D);
	opcodes.setOpcodePointer(155, &LR35902::SBC_A_E);
	opcodes.setOpcodePointer(156, &LR35902::SBC_A_H);
	opcodes.setOpcodePointer(157, &LR35902::SBC_A_L);
	opcodes.setOpcodePointer(158, &LR35902::SBC_A_aHL);
	opcodes.setOpcodePointer(159, &LR35902::SBC_A_A);
	opcodes.setOpcodePointer(160, &LR35902::AND_B);
	opcodes.setOpcodePointer(161, &LR35902::AND_C);
	opcodes.setOpcodePointer(162, &LR35902::AND_D);
	opcodes.setOpcodePointer(163, &LR35902::AND_E);
	opcodes.setOpcodePointer(164, &LR35902::AND_H);
	opcodes.setOpcodePointer(165, &LR35902::AND_L);
	opcodes.setOpcodePointer(166, &LR35902::AND_aHL);
	opcodes.setOpcodePointer(167, &LR35902::AND_A);
	opcodes.setOpcodePointer(168, &LR35902::XOR_B);
	opcodes.setOpcodePointer(169, &LR35902::XOR_C);
	opcodes.setOpcodePointer(170, &LR35902::XOR_D);
	opcodes.setOpcodePointer(171, &LR35902::XOR_E);
	opcodes.setOpcodePointer(172, &LR35902::XOR_H);
	opcodes.setOpcodePointer(173, &LR35902::XOR_L);
	opcodes.setOpcodePointer(174, &LR35902::XOR_aHL);
	opcodes.setOpcodePointer(175, &LR35902::XOR_A);
	opcodes.setOpcodePointer(176, &LR35902::OR_B);
	opcodes.setOpcodePointer(177, &LR35902::OR_C);
	opcodes.setOpcodePointer(178, &LR35902::OR_D);
	opcodes.setOpcodePointer(179, &LR35902::OR_E);
	opcodes.setOpcodePointer(180, &LR35902::OR_H);
	opcodes.setOpcodePointer(181, &LR35902::OR_L);
	opcodes.setOpcodePointer(182, &LR35902::OR_aHL);
	opcodes.setOpcodePointer(183, &LR35902::OR_A);
	opcodes.setOpcodePointer(184, &LR35902::CP_B);
	opcodes.setOpcodePointer(185, &LR35902::CP_C);
	opcodes.setOpcodePointer(186, &LR35902::CP_D);
	opcodes.setOpcodePointer(187, &LR35902::CP_E);
	opcodes.setOpcodePointer(188, &LR35902::CP_H);
	opcodes.setOpcodePointer(189, &LR35902::CP_L);
	opcodes.setOpcodePointer(190, &LR35902::CP_aHL);
	opcodes.setOpcodePointer(191, &LR35902::CP_A);
	opcodes.setOpcodePointer(192, &LR35902::RET_NZ);
	opcodes.setOpcodePointer(193, &LR35902::POP_BC);
	opcodes.setOpcodePointer(194, &LR35902::JP_NZ_d16);
	opcodes.setOpcodePointer(195, &LR35902::JP_d16);
	opcodes.setOpcodePointer(196, &LR35902::CALL_NZ_a16);
	opcodes.setOpcodePointer(197, &LR35902::PUSH_BC);
	opcodes.setOpcodePointer(198, &LR35902::ADD_A_d8);
	opcodes.setOpcodePointer(199, &LR35902::RST_00H);
	opcodes.setOpcodePointer(200, &LR35902::RET_Z);
	opcodes.setOpcodePointer(201, &LR35902::RET);
	opcodes.setOpcodePointer(202, &LR35902::JP_Z_d16);
	opcodes.setOpcodePointer(203, 0x0);
	opcodes.setOpcodePointer(204, &LR35902::CALL_Z_a16);
	opcodes.setOpcodePointer(205, &LR35902::CALL_a16);
	opcodes.setOpcodePointer(206, &LR35902::ADC_A_d8);
	opcodes.setOpcodePointer(207, &LR35902::RST_08H);
	opcodes.setOpcodePointer(208, &LR35902::RET_NC);
	opcodes.setOpcodePointer(209, &LR35902::POP_DE);
	opcodes.setOpcodePointer(210, &LR35902::JP_NC_d16);
	opcodes.setOpcodePointer(211, &LR35902::NOP);
	opcodes.setOpcodePointer(212, &LR35902::CALL_NC_a16);
	opcodes.setOpcodePointer(213, &LR35902::PUSH_DE);
	opcodes.setOpcodePointer(214, &LR35902::SUB_d8);
	opcodes.setOpcodePointer(215, &LR35902::RST_10H);
	opcodes.setOpcodePointer(216, &LR35902::RET_C);
	opcodes.setOpcodePointer(217, &LR35902::RETI);
	opcodes.setOpcodePointer(218, &LR35902::JP_C_d16);
	opcodes.setOpcodePointer(219, &LR35902::NOP);
	opcodes.setOpcodePointer(220, &LR35902::CALL_C_a16);
	opcodes.setOpcodePointer(221, &LR35902::NOP);
	opcodes.setOpcodePointer(222, &LR35902::SBC_A_d8);
	opcodes.setOpcodePointer(223, &LR35902::RST_18H);
	opcodes.setOpcodePointer(224, &LR35902::LDH_a8_A);
	opcodes.setOpcodePointer(225, &LR35902::POP_HL);
	opcodes.setOpcodePointer(226, &LR35902::LD_aC_A);
	opcodes.setOpcodePointer(227, &LR35902::NOP);
	opcodes.setOpcodePointer(228, &LR35902::NOP);
	opcodes.setOpcodePointer(229, &LR35902::PUSH_HL);
	opcodes.setOpcodePointer(230, &LR35902::AND_d8);
	opcodes.setOpcodePointer(231, &LR35902::RST_20H);
	opcodes.setOpcodePointer(232, &LR35902::ADD_SP_r8);
	opcodes.setOpcodePointer(233, &LR35902::JP_aHL);
	opcodes.setOpcodePointer(234, &LR35902::LD_a16_A);
	opcodes.setOpcodePointer(235, &LR35902::NOP);
	opcodes.setOpcodePointer(236, &LR35902::NOP);
	opcodes.setOpcodePointer(237, &LR35902::NOP);
	opcodes.setOpcodePointer(238, &LR35902::XOR_d8);
	opcodes.setOpcodePointer(239, &LR35902::RST_28H);
	opcodes.setOpcodePointer(240, &LR35902::LDH_A_a8);
	opcodes.setOpcodePointer(241, &LR35902::POP_AF);
	opcodes.setOpcodePointer(242, &LR35902::LD_A_aC);
	opcodes.setOpcodePointer(243, &LR35902::DI);
	opcodes.setOpcodePointer(244, &LR35902::NOP);
	opcodes.setOpcodePointer(245, &LR35902::PUSH_AF);
	opcodes.setOpcodePointer(246, &LR35902::OR_d8);
	opcodes.setOpcodePointer(247, &LR35902::RST_30H);
	opcodes.setOpcodePointer(248, &LR35902::LD_HL_SP_r8);
	opcodes.setOpcodePointer(249, &LR35902::LD_SP_HL);
	opcodes.setOpcodePointer(250, &LR35902::LD_A_a16);
	opcodes.setOpcodePointer(251, &LR35902::EI);
	opcodes.setOpcodePointer(252, &LR35902::NOP);
	opcodes.setOpcodePointer(253, &LR35902::NOP);
	opcodes.setOpcodePointer(254, &LR35902::CP_d8);
	opcodes.setOpcodePointer(255, &LR35902::RST_38H);
			
	// CB prefix opcodes
	opcodes.setOpcodePointerCB(0, &LR35902::RLC_B);
	opcodes.setOpcodePointerCB(1, &LR35902::RLC_C);
	opcodes.setOpcodePointerCB(2, &LR35902::RLC_D);
	opcodes.setOpcodePointerCB(3, &LR35902::RLC_E);
	opcodes.setOpcodePointerCB(4, &LR35902::RLC_H);
	opcodes.setOpcodePointerCB(5, &LR35902::RLC_L);
	opcodes.setOpcodePointerCB(6, &LR35902::RLC_aHL);
	opcodes.setOpcodePointerCB(7, &LR35902::RLC_A);
	opcodes.setOpcodePointerCB(8, &LR35902::RRC_B);
	opcodes.setOpcodePointerCB(9, &LR35902::RRC_C);
	opcodes.setOpcodePointerCB(10, &LR35902::RRC_D);
	opcodes.setOpcodePointerCB(11, &LR35902::RRC_E);
	opcodes.setOpcodePointerCB(12, &LR35902::RRC_H);
	opcodes.setOpcodePointerCB(13, &LR35902::RRC_L);
	opcodes.setOpcodePointerCB(14, &LR35902::RRC_aHL);
	opcodes.setOpcodePointerCB(15, &LR35902::RRC_A);
	opcodes.setOpcodePointerCB(16, &LR35902::RL_B);
	opcodes.setOpcodePointerCB(17, &LR35902::RL_C);
	opcodes.setOpcodePointerCB(18, &LR35902::RL_D);
	opcodes.setOpcodePointerCB(19, &LR35902::RL_E);
	opcodes.setOpcodePointerCB(20, &LR35902::RL_H);
	opcodes.setOpcodePointerCB(21, &LR35902::RL_L);
	opcodes.setOpcodePointerCB(22, &LR35902::RL_aHL);
	opcodes.setOpcodePointerCB(23, &LR35902::RL_A);
	opcodes.setOpcodePointerCB(24, &LR35902::RR_B);
	opcodes.setOpcodePointerCB(25, &LR35902::RR_C);
	opcodes.setOpcodePointerCB(26, &LR35902::RR_D);
	opcodes.setOpcodePointerCB(27, &LR35902::RR_E);
	opcodes.setOpcodePointerCB(28, &LR35902::RR_H);
	opcodes.setOpcodePointerCB(29, &LR35902::RR_L);
	opcodes.setOpcodePointerCB(30, &LR35902::RR_aHL);
	opcodes.setOpcodePointerCB(31, &LR35902::RR_A);
	opcodes.setOpcodePointerCB(32, &LR35902::SLA_B);
	opcodes.setOpcodePointerCB(33, &LR35902::SLA_C);
	opcodes.setOpcodePointerCB(34, &LR35902::SLA_D);
	opcodes.setOpcodePointerCB(35, &LR35902::SLA_E);
	opcodes.setOpcodePointerCB(36, &LR35902::SLA_H);
	opcodes.setOpcodePointerCB(37, &LR35902::SLA_L);
	opcodes.setOpcodePointerCB(38, &LR35902::SLA_aHL);
	opcodes.setOpcodePointerCB(39, &LR35902::SLA_A);
	opcodes.setOpcodePointerCB(40, &LR35902::SRA_B);
	opcodes.setOpcodePointerCB(41, &LR35902::SRA_C);
	opcodes.setOpcodePointerCB(42, &LR35902::SRA_D);
	opcodes.setOpcodePointerCB(43, &LR35902::SRA_E);
	opcodes.setOpcodePointerCB(44, &LR35902::SRA_H);
	opcodes.setOpcodePointerCB(45, &LR35902::SRA_L);
	opcodes.setOpcodePointerCB(46, &LR35902::SRA_aHL);
	opcodes.setOpcodePointerCB(47, &LR35902::SRA_A);
	opcodes.setOpcodePointerCB(48, &LR35902::SWAP_B);
	opcodes.setOpcodePointerCB(49, &LR35902::SWAP_C);
	opcodes.setOpcodePointerCB(50, &LR35902::SWAP_D);
	opcodes.setOpcodePointerCB(51, &LR35902::SWAP_E);
	opcodes.setOpcodePointerCB(52, &LR35902::SWAP_H);
	opcodes.setOpcodePointerCB(53, &LR35902::SWAP_L);
	opcodes.setOpcodePointerCB(54, &LR35902::SWAP_aHL);
	opcodes.setOpcodePointerCB(55, &LR35902::SWAP_A);
	opcodes.setOpcodePointerCB(56, &LR35902::SRL_B);
	opcodes.setOpcodePointerCB(57, &LR35902::SRL_C);
	opcodes.setOpcodePointerCB(58, &LR35902::SRL_D);
	opcodes.setOpcodePointerCB(59, &LR35902::SRL_E);
	opcodes.setOpcodePointerCB(60, &LR35902::SRL_H);
	opcodes.setOpcodePointerCB(61, &LR35902::SRL_L);
	opcodes.setOpcodePointerCB(62, &LR35902::SRL_aHL);
	opcodes.setOpcodePointerCB(63, &LR35902::SRL_A);
	opcodes.setOpcodePointerCB(64, &LR35902::BIT_0_B);
	opcodes.setOpcodePointerCB(65, &LR35902::BIT_0_C);
	opcodes.setOpcodePointerCB(66, &LR35902::BIT_0_D);
	opcodes.setOpcodePointerCB(67, &LR35902::BIT_0_E);
	opcodes.setOpcodePointerCB(68, &LR35902::BIT_0_H);
	opcodes.setOpcodePointerCB(69, &LR35902::BIT_0_L);
	opcodes.setOpcodePointerCB(70, &LR35902::BIT_0_aHL);
	opcodes.setOpcodePointerCB(71, &LR35902::BIT_0_A);
	opcodes.setOpcodePointerCB(72, &LR35902::BIT_1_B);
	opcodes.setOpcodePointerCB(73, &LR35902::BIT_1_C);
	opcodes.setOpcodePointerCB(74, &LR35902::BIT_1_D);
	opcodes.setOpcodePointerCB(75, &LR35902::BIT_1_E);
	opcodes.setOpcodePointerCB(76, &LR35902::BIT_1_H);
	opcodes.setOpcodePointerCB(77, &LR35902::BIT_1_L);
	opcodes.setOpcodePointerCB(78, &LR35902::BIT_1_aHL);
	opcodes.setOpcodePointerCB(79, &LR35902::BIT_1_A);
	opcodes.setOpcodePointerCB(80, &LR35902::BIT_2_B);
	opcodes.setOpcodePointerCB(81, &LR35902::BIT_2_C);
	opcodes.setOpcodePointerCB(82, &LR35902::BIT_2_D);
	opcodes.setOpcodePointerCB(83, &LR35902::BIT_2_E);
	opcodes.setOpcodePointerCB(84, &LR35902::BIT_2_H);
	opcodes.setOpcodePointerCB(85, &LR35902::BIT_2_L);
	opcodes.setOpcodePointerCB(86, &LR35902::BIT_2_aHL);
	opcodes.setOpcodePointerCB(87, &LR35902::BIT_2_A);
	opcodes.setOpcodePointerCB(88, &LR35902::BIT_3_B);
	opcodes.setOpcodePointerCB(89, &LR35902::BIT_3_C);
	opcodes.setOpcodePointerCB(90, &LR35902::BIT_3_D);
	opcodes.setOpcodePointerCB(91, &LR35902::BIT_3_E);
	opcodes.setOpcodePointerCB(92, &LR35902::BIT_3_H);
	opcodes.setOpcodePointerCB(93, &LR35902::BIT_3_L);
	opcodes.setOpcodePointerCB(94, &LR35902::BIT_3_aHL);
	opcodes.setOpcodePointerCB(95, &LR35902::BIT_3_A);
	opcodes.setOpcodePointerCB(96, &LR35902::BIT_4_B);
	opcodes.setOpcodePointerCB(97, &LR35902::BIT_4_C);
	opcodes.setOpcodePointerCB(98, &LR35902::BIT_4_D);
	opcodes.setOpcodePointerCB(99, &LR35902::BIT_4_E);
	opcodes.setOpcodePointerCB(100, &LR35902::BIT_4_H);
	opcodes.setOpcodePointerCB(101, &LR35902::BIT_4_L);
	opcodes.setOpcodePointerCB(102, &LR35902::BIT_4_aHL);
	opcodes.setOpcodePointerCB(103, &LR35902::BIT_4_A);
	opcodes.setOpcodePointerCB(104, &LR35902::BIT_5_B);
	opcodes.setOpcodePointerCB(105, &LR35902::BIT_5_C);
	opcodes.setOpcodePointerCB(106, &LR35902::BIT_5_D);
	opcodes.setOpcodePointerCB(107, &LR35902::BIT_5_E);
	opcodes.setOpcodePointerCB(108, &LR35902::BIT_5_H);
	opcodes.setOpcodePointerCB(109, &LR35902::BIT_5_L);
	opcodes.setOpcodePointerCB(110, &LR35902::BIT_5_aHL);
	opcodes.setOpcodePointerCB(111, &LR35902::BIT_5_A);
	opcodes.setOpcodePointerCB(112, &LR35902::BIT_6_B);
	opcodes.setOpcodePointerCB(113, &LR35902::BIT_6_C);
	opcodes.setOpcodePointerCB(114, &LR35902::BIT_6_D);
	opcodes.setOpcodePointerCB(115, &LR35902::BIT_6_E);
	opcodes.setOpcodePointerCB(116, &LR35902::BIT_6_H);
	opcodes.setOpcodePointerCB(117, &LR35902::BIT_6_L);
	opcodes.setOpcodePointerCB(118, &LR35902::BIT_6_aHL);
	opcodes.setOpcodePointerCB(119, &LR35902::BIT_6_A);
	opcodes.setOpcodePointerCB(120, &LR35902::BIT_7_B);
	opcodes.setOpcodePointerCB(121, &LR35902::BIT_7_C);
	opcodes.setOpcodePointerCB(122, &LR35902::BIT_7_D);
	opcodes.setOpcodePointerCB(123, &LR35902::BIT_7_E);
	opcodes.setOpcodePointerCB(124, &LR35902::BIT_7_H);
	opcodes.setOpcodePointerCB(125, &LR35902::BIT_7_L);
	opcodes.setOpcodePointerCB(126, &LR35902::BIT_7_aHL);
	opcodes.setOpcodePointerCB(127, &LR35902::BIT_7_A);
	opcodes.setOpcodePointerCB(128, &LR35902::RES_0_B);
	opcodes.setOpcodePointerCB(129, &LR35902::RES_0_C);
	opcodes.setOpcodePointerCB(130, &LR35902::RES_0_D);
	opcodes.setOpcodePointerCB(131, &LR35902::RES_0_E);
	opcodes.setOpcodePointerCB(132, &LR35902::RES_0_H);
	opcodes.setOpcodePointerCB(133, &LR35902::RES_0_L);
	opcodes.setOpcodePointerCB(134, &LR35902::RES_0_aHL);
	opcodes.setOpcodePointerCB(135, &LR35902::RES_0_A);
	opcodes.setOpcodePointerCB(136, &LR35902::RES_1_B);
	opcodes.setOpcodePointerCB(137, &LR35902::RES_1_C);
	opcodes.setOpcodePointerCB(138, &LR35902::RES_1_D);
	opcodes.setOpcodePointerCB(139, &LR35902::RES_1_E);
	opcodes.setOpcodePointerCB(140, &LR35902::RES_1_H);
	opcodes.setOpcodePointerCB(141, &LR35902::RES_1_L);
	opcodes.setOpcodePointerCB(142, &LR35902::RES_1_aHL);
	opcodes.setOpcodePointerCB(143, &LR35902::RES_1_A);
	opcodes.setOpcodePointerCB(144, &LR35902::RES_2_B);
	opcodes.setOpcodePointerCB(145, &LR35902::RES_2_C);
	opcodes.setOpcodePointerCB(146, &LR35902::RES_2_D);
	opcodes.setOpcodePointerCB(147, &LR35902::RES_2_E);
	opcodes.setOpcodePointerCB(148, &LR35902::RES_2_H);
	opcodes.setOpcodePointerCB(149, &LR35902::RES_2_L);
	opcodes.setOpcodePointerCB(150, &LR35902::RES_2_aHL);
	opcodes.setOpcodePointerCB(151, &LR35902::RES_2_A);
	opcodes.setOpcodePointerCB(152, &LR35902::RES_3_B);
	opcodes.setOpcodePointerCB(153, &LR35902::RES_3_C);
	opcodes.setOpcodePointerCB(154, &LR35902::RES_3_D);
	opcodes.setOpcodePointerCB(155, &LR35902::RES_3_E);
	opcodes.setOpcodePointerCB(156, &LR35902::RES_3_H);
	opcodes.setOpcodePointerCB(157, &LR35902::RES_3_L);
	opcodes.setOpcodePointerCB(158, &LR35902::RES_3_aHL);
	opcodes.setOpcodePointerCB(159, &LR35902::RES_3_A);
	opcodes.setOpcodePointerCB(160, &LR35902::RES_4_B);
	opcodes.setOpcodePointerCB(161, &LR35902::RES_4_C);
	opcodes.setOpcodePointerCB(162, &LR35902::RES_4_D);
	opcodes.setOpcodePointerCB(163, &LR35902::RES_4_E);
	opcodes.setOpcodePointerCB(164, &LR35902::RES_4_H);
	opcodes.setOpcodePointerCB(165, &LR35902::RES_4_L);
	opcodes.setOpcodePointerCB(166, &LR35902::RES_4_aHL);
	opcodes.setOpcodePointerCB(167, &LR35902::RES_4_A);
	opcodes.setOpcodePointerCB(168, &LR35902::RES_5_B);
	opcodes.setOpcodePointerCB(169, &LR35902::RES_5_C);
	opcodes.setOpcodePointerCB(170, &LR35902::RES_5_D);
	opcodes.setOpcodePointerCB(171, &LR35902::RES_5_E);
	opcodes.setOpcodePointerCB(172, &LR35902::RES_5_H);
	opcodes.setOpcodePointerCB(173, &LR35902::RES_5_L);
	opcodes.setOpcodePointerCB(174, &LR35902::RES_5_aHL);
	opcodes.setOpcodePointerCB(175, &LR35902::RES_5_A);
	opcodes.setOpcodePointerCB(176, &LR35902::RES_6_B);
	opcodes.setOpcodePointerCB(177, &LR35902::RES_6_C);
	opcodes.setOpcodePointerCB(178, &LR35902::RES_6_D);
	opcodes.setOpcodePointerCB(179, &LR35902::RES_6_E);
	opcodes.setOpcodePointerCB(180, &LR35902::RES_6_H);
	opcodes.setOpcodePointerCB(181, &LR35902::RES_6_L);
	opcodes.setOpcodePointerCB(182, &LR35902::RES_6_aHL);
	opcodes.setOpcodePointerCB(183, &LR35902::RES_6_A);
	opcodes.setOpcodePointerCB(184, &LR35902::RES_7_B);
	opcodes.setOpcodePointerCB(185, &LR35902::RES_7_C);
	opcodes.setOpcodePointerCB(186, &LR35902::RES_7_D);
	opcodes.setOpcodePointerCB(187, &LR35902::RES_7_E);
	opcodes.setOpcodePointerCB(188, &LR35902::RES_7_H);
	opcodes.setOpcodePointerCB(189, &LR35902::RES_7_L);
	opcodes.setOpcodePointerCB(190, &LR35902::RES_7_aHL);
	opcodes.setOpcodePointerCB(191, &LR35902::RES_7_A);
	opcodes.setOpcodePointerCB(192, &LR35902::SET_0_B);
	opcodes.setOpcodePointerCB(193, &LR35902::SET_0_C);
	opcodes.setOpcodePointerCB(194, &LR35902::SET_0_D);
	opcodes.setOpcodePointerCB(195, &LR35902::SET_0_E);
	opcodes.setOpcodePointerCB(196, &LR35902::SET_0_H);
	opcodes.setOpcodePointerCB(197, &LR35902::SET_0_L);
	opcodes.setOpcodePointerCB(198, &LR35902::SET_0_aHL);
	opcodes.setOpcodePointerCB(199, &LR35902::SET_0_A);
	opcodes.setOpcodePointerCB(200, &LR35902::SET_1_B);
	opcodes.setOpcodePointerCB(201, &LR35902::SET_1_C);
	opcodes.setOpcodePointerCB(202, &LR35902::SET_1_D);
	opcodes.setOpcodePointerCB(203, &LR35902::SET_1_E);
	opcodes.setOpcodePointerCB(204, &LR35902::SET_1_H);
	opcodes.setOpcodePointerCB(205, &LR35902::SET_1_L);
	opcodes.setOpcodePointerCB(206, &LR35902::SET_1_aHL);
	opcodes.setOpcodePointerCB(207, &LR35902::SET_1_A);
	opcodes.setOpcodePointerCB(208, &LR35902::SET_2_B);
	opcodes.setOpcodePointerCB(209, &LR35902::SET_2_C);
	opcodes.setOpcodePointerCB(210, &LR35902::SET_2_D);
	opcodes.setOpcodePointerCB(211, &LR35902::SET_2_E);
	opcodes.setOpcodePointerCB(212, &LR35902::SET_2_H);
	opcodes.setOpcodePointerCB(213, &LR35902::SET_2_L);
	opcodes.setOpcodePointerCB(214, &LR35902::SET_2_aHL);
	opcodes.setOpcodePointerCB(215, &LR35902::SET_2_A);
	opcodes.setOpcodePointerCB(216, &LR35902::SET_3_B);
	opcodes.setOpcodePointerCB(217, &LR35902::SET_3_C);
	opcodes.setOpcodePointerCB(218, &LR35902::SET_3_D);
	opcodes.setOpcodePointerCB(219, &LR35902::SET_3_E);
	opcodes.setOpcodePointerCB(220, &LR35902::SET_3_H);
	opcodes.setOpcodePointerCB(221, &LR35902::SET_3_L);
	opcodes.setOpcodePointerCB(222, &LR35902::SET_3_aHL);
	opcodes.setOpcodePointerCB(223, &LR35902::SET_3_A);
	opcodes.setOpcodePointerCB(224, &LR35902::SET_4_B);
	opcodes.setOpcodePointerCB(225, &LR35902::SET_4_C);
	opcodes.setOpcodePointerCB(226, &LR35902::SET_4_D);
	opcodes.setOpcodePointerCB(227, &LR35902::SET_4_E);
	opcodes.setOpcodePointerCB(228, &LR35902::SET_4_H);
	opcodes.setOpcodePointerCB(229, &LR35902::SET_4_L);
	opcodes.setOpcodePointerCB(230, &LR35902::SET_4_aHL);
	opcodes.setOpcodePointerCB(231, &LR35902::SET_4_A);
	opcodes.setOpcodePointerCB(232, &LR35902::SET_5_B);
	opcodes.setOpcodePointerCB(233, &LR35902::SET_5_C);
	opcodes.setOpcodePointerCB(234, &LR35902::SET_5_D);
	opcodes.setOpcodePointerCB(235, &LR35902::SET_5_E);
	opcodes.setOpcodePointerCB(236, &LR35902::SET_5_H);
	opcodes.setOpcodePointerCB(237, &LR35902::SET_5_L);
	opcodes.setOpcodePointerCB(238, &LR35902::SET_5_aHL);
	opcodes.setOpcodePointerCB(239, &LR35902::SET_5_A);
	opcodes.setOpcodePointerCB(240, &LR35902::SET_6_B);
	opcodes.setOpcodePointerCB(241, &LR35902::SET_6_C);
	opcodes.setOpcodePointerCB(242, &LR35902::SET_6_D);
	opcodes.setOpcodePointerCB(243, &LR35902::SET_6_E);
	opcodes.setOpcodePointerCB(244, &LR35902::SET_6_H);
	opcodes.setOpcodePointerCB(245, &LR35902::SET_6_L);
	opcodes.setOpcodePointerCB(246, &LR35902::SET_6_aHL);
	opcodes.setOpcodePointerCB(247, &LR35902::SET_6_A);
	opcodes.setOpcodePointerCB(248, &LR35902::SET_7_B);
	opcodes.setOpcodePointerCB(249, &LR35902::SET_7_C);
	opcodes.setOpcodePointerCB(250, &LR35902::SET_7_D);
	opcodes.setOpcodePointerCB(251, &LR35902::SET_7_E);
	opcodes.setOpcodePointerCB(252, &LR35902::SET_7_H);
	opcodes.setOpcodePointerCB(253, &LR35902::SET_7_L);
	opcodes.setOpcodePointerCB(254, &LR35902::SET_7_aHL);
	opcodes.setOpcodePointerCB(255, &LR35902::SET_7_A); 

	// Set memory address getters/setters for opcodes which access system memory
	opcodes.setMemoryAccess(this);
}

void LR35902::reset(){
	// Set startup values for the CPU registers
	setAF(0x11B0); // 0x01B0
	setBC(0x0013); // 0x0013
	setDE(0x00D8); // 0x00D8
	setHL(0x014D); // 0x014D
	SP = 0xFFFE;   // 0xFFFE
	PC = 0x0100;   // 0x0100
}
