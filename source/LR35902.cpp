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

	Opcode* ops = opcodes.getOpcodes();

	// Standard opcodes
	//                          Mnemonic        C  L  R  W  Pointer
	ops[0]   = Opcode(this, "NOP         ", 1, 1, 0, 0, &LR35902::NOP);
	ops[1]   = Opcode(this, "LD BC,d16   ", 3, 3, 0, 0, &LR35902::LD_BC_d16);
	ops[2]   = Opcode(this, "LD (BC),A   ", 2, 1, 0, 2, &LR35902::LD_aBC_A);
	ops[3]   = Opcode(this, "INC BC      ", 2, 1, 0, 0, &LR35902::INC_BC);
	ops[4]   = Opcode(this, "INC B       ", 1, 1, 0, 0, &LR35902::INC_B);
	ops[5]   = Opcode(this, "DEC B       ", 1, 1, 0, 0, &LR35902::DEC_B);
	ops[6]   = Opcode(this, "LD B,d8     ", 2, 2, 0, 0, &LR35902::LD_B_d8);
	ops[7]   = Opcode(this, "RLCA        ", 1, 1, 0, 0, &LR35902::RLCA);
	ops[8]   = Opcode(this, "LD (a16),SP ", 5, 3, 0, 0, &LR35902::LD_a16_SP);
	ops[9]   = Opcode(this, "ADD HL,BC   ", 2, 1, 0, 0, &LR35902::ADD_HL_BC);
	ops[10]  = Opcode(this, "LD A,(BC)   ", 2, 1, 2, 0, &LR35902::LD_A_aBC);
	ops[11]  = Opcode(this, "DEC BC      ", 2, 1, 0, 0, &LR35902::DEC_BC);
	ops[12]  = Opcode(this, "INC C       ", 1, 1, 0, 0, &LR35902::INC_C);
	ops[13]  = Opcode(this, "DEC C       ", 1, 1, 0, 0, &LR35902::DEC_C);
	ops[14]  = Opcode(this, "LD C,d8     ", 2, 2, 0, 0, &LR35902::LD_C_d8);
	ops[15]  = Opcode(this, "RRCA        ", 1, 1, 0, 0, &LR35902::RRCA);
	ops[16]  = Opcode(this, "STOP 0      ", 1, 2, 0, 0, &LR35902::STOP_0);
	ops[17]  = Opcode(this, "LD DE,d16   ", 3, 3, 0, 0, &LR35902::LD_DE_d16);
	ops[18]  = Opcode(this, "LD (DE),A   ", 2, 1, 0, 2, &LR35902::LD_aDE_A);
	ops[19]  = Opcode(this, "INC DE      ", 2, 1, 0, 0, &LR35902::INC_DE);
	ops[20]  = Opcode(this, "INC D       ", 1, 1, 0, 0, &LR35902::INC_D);
	ops[21]  = Opcode(this, "DEC D       ", 1, 1, 0, 0, &LR35902::DEC_D);
	ops[22]  = Opcode(this, "LD D,d8     ", 2, 2, 0, 0, &LR35902::LD_D_d8);
	ops[23]  = Opcode(this, "RLA         ", 1, 1, 0, 0, &LR35902::RLA);
	ops[24]  = Opcode(this, "JR r8       ", 3, 2, 0, 0, &LR35902::JR_r8);
	ops[25]  = Opcode(this, "ADD HL,DE   ", 2, 1, 0, 0, &LR35902::ADD_HL_DE);
	ops[26]  = Opcode(this, "LD A,(DE)   ", 2, 1, 2, 0, &LR35902::LD_A_aDE);
	ops[27]  = Opcode(this, "DEC DE      ", 2, 1, 0, 0, &LR35902::DEC_DE);
	ops[28]  = Opcode(this, "INC E       ", 1, 1, 0, 0, &LR35902::INC_E);
	ops[29]  = Opcode(this, "DEC E       ", 1, 1, 0, 0, &LR35902::DEC_E);
	ops[30]  = Opcode(this, "LD E,d8     ", 2, 2, 0, 0, &LR35902::LD_E_d8);
	ops[31]  = Opcode(this, "RRA         ", 1, 1, 0, 0, &LR35902::RRA);
	ops[32]  = Opcode(this, "JR NZ,r8    ", 2, 2, 0, 0, &LR35902::JR_NZ_r8);
	ops[33]  = Opcode(this, "LD HL,d16   ", 3, 3, 0, 0, &LR35902::LD_HL_d16);
	ops[34]  = Opcode(this, "LDI (HL),A  ", 2, 1, 0, 2, &LR35902::LDI_aHL_A);
	ops[35]  = Opcode(this, "INC HL      ", 2, 1, 0, 0, &LR35902::INC_HL);
	ops[36]  = Opcode(this, "INC H       ", 1, 1, 0, 0, &LR35902::INC_H);
	ops[37]  = Opcode(this, "DEC H       ", 1, 1, 0, 0, &LR35902::DEC_H);
	ops[38]  = Opcode(this, "LD H,d8     ", 2, 2, 0, 0, &LR35902::LD_H_d8);
	ops[39]  = Opcode(this, "DAA         ", 1, 1, 0, 0, &LR35902::DAA);
	ops[40]  = Opcode(this, "JR Z,r8     ", 2, 2, 0, 0, &LR35902::JR_Z_r8);
	ops[41]  = Opcode(this, "ADD HL,HL   ", 2, 1, 0, 0, &LR35902::ADD_HL_HL);
	ops[42]  = Opcode(this, "LDI A,(HL)  ", 2, 1, 2, 0, &LR35902::LDI_A_aHL);
	ops[43]  = Opcode(this, "DEC HL      ", 2, 1, 0, 0, &LR35902::DEC_HL);
	ops[44]  = Opcode(this, "INC L       ", 1, 1, 0, 0, &LR35902::INC_L);
	ops[45]  = Opcode(this, "DEC L       ", 1, 1, 0, 0, &LR35902::DEC_L);
	ops[46]  = Opcode(this, "LD L,d8     ", 2, 2, 0, 0, &LR35902::LD_L_d8);
	ops[47]  = Opcode(this, "CPL         ", 1, 1, 0, 0, &LR35902::CPL);
	ops[48]  = Opcode(this, "JR NC,r8    ", 2, 2, 0, 0, &LR35902::JR_NC_r8);
	ops[49]  = Opcode(this, "LD SP,d16   ", 3, 3, 0, 0, &LR35902::LD_SP_d16);
	ops[50]  = Opcode(this, "LDD (HL),A  ", 2, 1, 0, 2, &LR35902::LDD_aHL_A);
	ops[51]  = Opcode(this, "INC SP      ", 2, 1, 0, 0, &LR35902::INC_SP);
	ops[52]  = Opcode(this, "INC (HL)    ", 3, 1, 2, 3, &LR35902::INC_aHL);
	ops[53]  = Opcode(this, "DEC (HL)    ", 3, 1, 2, 3, &LR35902::DEC_aHL);
	ops[54]  = Opcode(this, "LD (HL),d8  ", 3, 2, 0, 3, &LR35902::LD_aHL_d8);
	ops[55]  = Opcode(this, "SCF         ", 1, 1, 0, 0, &LR35902::SCF);
	ops[56]  = Opcode(this, "JR C,r8     ", 2, 2, 0, 0, &LR35902::JR_C_r8);
	ops[57]  = Opcode(this, "ADD HL,SP   ", 2, 1, 0, 0, &LR35902::ADD_HL_SP);
	ops[58]  = Opcode(this, "LDD A,(HL)  ", 2, 1, 2, 0, &LR35902::LDD_A_aHL);
	ops[59]  = Opcode(this, "DEC SP      ", 2, 1, 0, 0, &LR35902::DEC_SP);
	ops[60]  = Opcode(this, "INC A       ", 1, 1, 0, 0, &LR35902::INC_A);
	ops[61]  = Opcode(this, "DEC A       ", 1, 1, 0, 0, &LR35902::DEC_A);
	ops[62]  = Opcode(this, "LD A,d8     ", 2, 2, 0, 0, &LR35902::LD_A_d8);
	ops[63]  = Opcode(this, "CCF         ", 1, 1, 0, 0, &LR35902::CCF);
	ops[64]  = Opcode(this, "LD B,B      ", 1, 1, 0, 0, &LR35902::LD_B_B);
	ops[65]  = Opcode(this, "LD B,C      ", 1, 1, 0, 0, &LR35902::LD_B_C);
	ops[66]  = Opcode(this, "LD B,D      ", 1, 1, 0, 0, &LR35902::LD_B_D);
	ops[67]  = Opcode(this, "LD B,E      ", 1, 1, 0, 0, &LR35902::LD_B_E);
	ops[68]  = Opcode(this, "LD B,H      ", 1, 1, 0, 0, &LR35902::LD_B_H);
	ops[69]  = Opcode(this, "LD B,L      ", 1, 1, 0, 0, &LR35902::LD_B_L);
	ops[70]  = Opcode(this, "LD B,(HL)   ", 2, 1, 2, 0, &LR35902::LD_B_aHL);
	ops[71]  = Opcode(this, "LD B,A      ", 1, 1, 0, 0, &LR35902::LD_B_A);
	ops[72]  = Opcode(this, "LD C,B      ", 1, 1, 0, 0, &LR35902::LD_C_B);
	ops[73]  = Opcode(this, "LD C,C      ", 1, 1, 0, 0, &LR35902::LD_C_C);
	ops[74]  = Opcode(this, "LD C,D      ", 1, 1, 0, 0, &LR35902::LD_C_D);
	ops[75]  = Opcode(this, "LD C,E      ", 1, 1, 0, 0, &LR35902::LD_C_E);
	ops[76]  = Opcode(this, "LD C,H      ", 1, 1, 0, 0, &LR35902::LD_C_H);
	ops[77]  = Opcode(this, "LD C,L      ", 1, 1, 0, 0, &LR35902::LD_C_L);
	ops[78]  = Opcode(this, "LD C,(HL)   ", 2, 1, 2, 0, &LR35902::LD_C_aHL);
	ops[79]  = Opcode(this, "LD C,A      ", 1, 1, 0, 0, &LR35902::LD_C_A);
	ops[80]  = Opcode(this, "LD D,B      ", 1, 1, 0, 0, &LR35902::LD_D_B);
	ops[81]  = Opcode(this, "LD D,C      ", 1, 1, 0, 0, &LR35902::LD_D_C);
	ops[82]  = Opcode(this, "LD D,D      ", 1, 1, 0, 0, &LR35902::LD_D_D);
	ops[83]  = Opcode(this, "LD D,E      ", 1, 1, 0, 0, &LR35902::LD_D_E);
	ops[84]  = Opcode(this, "LD D,H      ", 1, 1, 0, 0, &LR35902::LD_D_H);
	ops[85]  = Opcode(this, "LD D,L      ", 1, 1, 0, 0, &LR35902::LD_D_L);
	ops[86]  = Opcode(this, "LD D,(HL)   ", 2, 1, 2, 0, &LR35902::LD_D_aHL);
	ops[87]  = Opcode(this, "LD D,A      ", 1, 1, 0, 0, &LR35902::LD_D_A);
	ops[88]  = Opcode(this, "LD E,B      ", 1, 1, 0, 0, &LR35902::LD_E_B);
	ops[89]  = Opcode(this, "LD E,C      ", 1, 1, 0, 0, &LR35902::LD_E_C);
	ops[90]  = Opcode(this, "LD E,D      ", 1, 1, 0, 0, &LR35902::LD_E_D);
	ops[91]  = Opcode(this, "LD E,E      ", 1, 1, 0, 0, &LR35902::LD_E_E);
	ops[92]  = Opcode(this, "LD E,H      ", 1, 1, 0, 0, &LR35902::LD_E_H);
	ops[93]  = Opcode(this, "LD E,L      ", 1, 1, 0, 0, &LR35902::LD_E_L);
	ops[94]  = Opcode(this, "LD E,(HL)   ", 2, 1, 2, 0, &LR35902::LD_E_aHL);
	ops[95]  = Opcode(this, "LD E,A      ", 1, 1, 0, 0, &LR35902::LD_E_A);
	ops[96]  = Opcode(this, "LD H,B      ", 1, 1, 0, 0, &LR35902::LD_H_B);
	ops[97]  = Opcode(this, "LD H,C      ", 1, 1, 0, 0, &LR35902::LD_H_C);
	ops[98]  = Opcode(this, "LD H,D      ", 1, 1, 0, 0, &LR35902::LD_H_D);
	ops[99]  = Opcode(this, "LD H,E      ", 1, 1, 0, 0, &LR35902::LD_H_E);
	ops[100] = Opcode(this, "LD H,H      ", 1, 1, 0, 0, &LR35902::LD_H_H);
	ops[101] = Opcode(this, "LD H,L      ", 1, 1, 0, 0, &LR35902::LD_H_L);
	ops[102] = Opcode(this, "LD H,(HL)   ", 2, 1, 2, 0, &LR35902::LD_H_aHL);
	ops[103] = Opcode(this, "LD H,A      ", 1, 1, 0, 0, &LR35902::LD_H_A);
	ops[104] = Opcode(this, "LD L,B      ", 1, 1, 0, 0, &LR35902::LD_L_B);
	ops[105] = Opcode(this, "LD L,C      ", 1, 1, 0, 0, &LR35902::LD_L_C);
	ops[106] = Opcode(this, "LD L,D      ", 1, 1, 0, 0, &LR35902::LD_L_D);
	ops[107] = Opcode(this, "LD L,E      ", 1, 1, 0, 0, &LR35902::LD_L_E);
	ops[108] = Opcode(this, "LD L,H      ", 1, 1, 0, 0, &LR35902::LD_L_H);
	ops[109] = Opcode(this, "LD L,L      ", 1, 1, 0, 0, &LR35902::LD_L_L);
	ops[110] = Opcode(this, "LD L,(HL)   ", 2, 1, 2, 0, &LR35902::LD_L_aHL);
	ops[111] = Opcode(this, "LD L,A      ", 1, 1, 0, 0, &LR35902::LD_L_A);
	ops[112] = Opcode(this, "LD (HL),B   ", 2, 1, 0, 2, &LR35902::LD_aHL_B);
	ops[113] = Opcode(this, "LD (HL),C   ", 2, 1, 0, 2, &LR35902::LD_aHL_C);
	ops[114] = Opcode(this, "LD (HL),D   ", 2, 1, 0, 2, &LR35902::LD_aHL_D);
	ops[115] = Opcode(this, "LD (HL),E   ", 2, 1, 0, 2, &LR35902::LD_aHL_E);
	ops[116] = Opcode(this, "LD (HL),H   ", 2, 1, 0, 2, &LR35902::LD_aHL_H);
	ops[117] = Opcode(this, "LD (HL),L   ", 2, 1, 0, 2, &LR35902::LD_aHL_L);
	ops[118] = Opcode(this, "HALT        ", 1, 1, 0, 0, &LR35902::HALT);
	ops[119] = Opcode(this, "LD (HL),A   ", 2, 1, 0, 2, &LR35902::LD_aHL_A);
	ops[120] = Opcode(this, "LD A,B      ", 1, 1, 0, 0, &LR35902::LD_A_B);
	ops[121] = Opcode(this, "LD A,C      ", 1, 1, 0, 0, &LR35902::LD_A_C);
	ops[122] = Opcode(this, "LD A,D      ", 1, 1, 0, 0, &LR35902::LD_A_D);
	ops[123] = Opcode(this, "LD A,E      ", 1, 1, 0, 0, &LR35902::LD_A_E);
	ops[124] = Opcode(this, "LD A,H      ", 1, 1, 0, 0, &LR35902::LD_A_H);
	ops[125] = Opcode(this, "LD A,L      ", 1, 1, 0, 0, &LR35902::LD_A_L);
	ops[126] = Opcode(this, "LD A,(HL)   ", 2, 1, 2, 0, &LR35902::LD_A_aHL);
	ops[127] = Opcode(this, "LD A,A      ", 1, 1, 0, 0, &LR35902::LD_A_A);
	ops[128] = Opcode(this, "ADD A,B     ", 1, 1, 0, 0, &LR35902::ADD_A_B);
	ops[129] = Opcode(this, "ADD A,C     ", 1, 1, 0, 0, &LR35902::ADD_A_C);
	ops[130] = Opcode(this, "ADD A,D     ", 1, 1, 0, 0, &LR35902::ADD_A_D);
	ops[131] = Opcode(this, "ADD A,E     ", 1, 1, 0, 0, &LR35902::ADD_A_E);
	ops[132] = Opcode(this, "ADD A,H     ", 1, 1, 0, 0, &LR35902::ADD_A_H);
	ops[133] = Opcode(this, "ADD A,L     ", 1, 1, 0, 0, &LR35902::ADD_A_L);
	ops[134] = Opcode(this, "ADD A,(HL)  ", 2, 1, 2, 0, &LR35902::ADD_A_aHL);
	ops[135] = Opcode(this, "ADD A,A     ", 1, 1, 0, 0, &LR35902::ADD_A_A);
	ops[136] = Opcode(this, "ADC A,B     ", 1, 1, 0, 0, &LR35902::ADC_A_B);
	ops[137] = Opcode(this, "ADC A,C     ", 1, 1, 0, 0, &LR35902::ADC_A_C);
	ops[138] = Opcode(this, "ADC A,D     ", 1, 1, 0, 0, &LR35902::ADC_A_D);
	ops[139] = Opcode(this, "ADC A,E     ", 1, 1, 0, 0, &LR35902::ADC_A_E);
	ops[140] = Opcode(this, "ADC A,H     ", 1, 1, 0, 0, &LR35902::ADC_A_H);
	ops[141] = Opcode(this, "ADC A,L     ", 1, 1, 0, 0, &LR35902::ADC_A_L);
	ops[142] = Opcode(this, "ADC A,(HL)  ", 2, 1, 2, 0, &LR35902::ADC_A_aHL);
	ops[143] = Opcode(this, "ADC A,A     ", 1, 1, 0, 0, &LR35902::ADC_A_A);
	ops[144] = Opcode(this, "SUB B       ", 1, 1, 0, 0, &LR35902::SUB_B);
	ops[145] = Opcode(this, "SUB C       ", 1, 1, 0, 0, &LR35902::SUB_C);
	ops[146] = Opcode(this, "SUB D       ", 1, 1, 0, 0, &LR35902::SUB_D);
	ops[147] = Opcode(this, "SUB E       ", 1, 1, 0, 0, &LR35902::SUB_E);
	ops[148] = Opcode(this, "SUB H       ", 1, 1, 0, 0, &LR35902::SUB_H);
	ops[149] = Opcode(this, "SUB L       ", 1, 1, 0, 0, &LR35902::SUB_L);
	ops[150] = Opcode(this, "SUB (HL)    ", 2, 1, 2, 0, &LR35902::SUB_aHL);
	ops[151] = Opcode(this, "SUB A       ", 1, 1, 0, 0, &LR35902::SUB_A);
	ops[152] = Opcode(this, "SBC A,B     ", 1, 1, 0, 0, &LR35902::SBC_A_B);
	ops[153] = Opcode(this, "SBC A,C     ", 1, 1, 0, 0, &LR35902::SBC_A_C);
	ops[154] = Opcode(this, "SBC A,D     ", 1, 1, 0, 0, &LR35902::SBC_A_D);
	ops[155] = Opcode(this, "SBC A,E     ", 1, 1, 0, 0, &LR35902::SBC_A_E);
	ops[156] = Opcode(this, "SBC A,H     ", 1, 1, 0, 0, &LR35902::SBC_A_H);
	ops[157] = Opcode(this, "SBC A,L     ", 1, 1, 0, 0, &LR35902::SBC_A_L);
	ops[158] = Opcode(this, "SBC A,(HL)  ", 2, 1, 2, 0, &LR35902::SBC_A_aHL);
	ops[159] = Opcode(this, "SBC A,A     ", 1, 1, 0, 0, &LR35902::SBC_A_A);
	ops[160] = Opcode(this, "AND B       ", 1, 1, 0, 0, &LR35902::AND_B);
	ops[161] = Opcode(this, "AND C       ", 1, 1, 0, 0, &LR35902::AND_C);
	ops[162] = Opcode(this, "AND D       ", 1, 1, 0, 0, &LR35902::AND_D);
	ops[163] = Opcode(this, "AND E       ", 1, 1, 0, 0, &LR35902::AND_E);
	ops[164] = Opcode(this, "AND H       ", 1, 1, 0, 0, &LR35902::AND_H);
	ops[165] = Opcode(this, "AND L       ", 1, 1, 0, 0, &LR35902::AND_L);
	ops[166] = Opcode(this, "AND (HL)    ", 2, 1, 2, 0, &LR35902::AND_aHL);
	ops[167] = Opcode(this, "AND A       ", 1, 1, 0, 0, &LR35902::AND_A);
	ops[168] = Opcode(this, "XOR B       ", 1, 1, 0, 0, &LR35902::XOR_B);
	ops[169] = Opcode(this, "XOR C       ", 1, 1, 0, 0, &LR35902::XOR_C);
	ops[170] = Opcode(this, "XOR D       ", 1, 1, 0, 0, &LR35902::XOR_D);
	ops[171] = Opcode(this, "XOR E       ", 1, 1, 0, 0, &LR35902::XOR_E);
	ops[172] = Opcode(this, "XOR H       ", 1, 1, 0, 0, &LR35902::XOR_H);
	ops[173] = Opcode(this, "XOR L       ", 1, 1, 0, 0, &LR35902::XOR_L);
	ops[174] = Opcode(this, "XOR (HL)    ", 2, 1, 2, 0, &LR35902::XOR_aHL);
	ops[175] = Opcode(this, "XOR A       ", 1, 1, 0, 0, &LR35902::XOR_A);
	ops[176] = Opcode(this, "OR B        ", 1, 1, 0, 0, &LR35902::OR_B);
	ops[177] = Opcode(this, "OR C        ", 1, 1, 0, 0, &LR35902::OR_C);
	ops[178] = Opcode(this, "OR D        ", 1, 1, 0, 0, &LR35902::OR_D);
	ops[179] = Opcode(this, "OR E        ", 1, 1, 0, 0, &LR35902::OR_E);
	ops[180] = Opcode(this, "OR H        ", 1, 1, 0, 0, &LR35902::OR_H);
	ops[181] = Opcode(this, "OR L        ", 1, 1, 0, 0, &LR35902::OR_L);
	ops[182] = Opcode(this, "OR (HL)     ", 2, 1, 2, 0, &LR35902::OR_aHL);
	ops[183] = Opcode(this, "OR A        ", 1, 1, 0, 0, &LR35902::OR_A);
	ops[184] = Opcode(this, "CP B        ", 1, 1, 0, 0, &LR35902::CP_B);
	ops[185] = Opcode(this, "CP C        ", 1, 1, 0, 0, &LR35902::CP_C);
	ops[186] = Opcode(this, "CP D        ", 1, 1, 0, 0, &LR35902::CP_D);
	ops[187] = Opcode(this, "CP E        ", 1, 1, 0, 0, &LR35902::CP_E);
	ops[188] = Opcode(this, "CP H        ", 1, 1, 0, 0, &LR35902::CP_H);
	ops[189] = Opcode(this, "CP L        ", 1, 1, 0, 0, &LR35902::CP_L);
	ops[190] = Opcode(this, "CP (HL)     ", 2, 1, 2, 0, &LR35902::CP_aHL);
	ops[191] = Opcode(this, "CP A        ", 1, 1, 0, 0, &LR35902::CP_A);
	ops[192] = Opcode(this, "RET NZ      ", 2, 1, 0, 0, &LR35902::RET_NZ);
	ops[193] = Opcode(this, "POP BC      ", 3, 1, 0, 0, &LR35902::POP_BC);
	ops[194] = Opcode(this, "JP NZ,a16   ", 3, 3, 0, 0, &LR35902::JP_NZ_d16);
	ops[195] = Opcode(this, "JP a16      ", 4, 3, 0, 0, &LR35902::JP_d16);
	ops[196] = Opcode(this, "CALL NZ,a16 ", 3, 3, 0, 0, &LR35902::CALL_NZ_a16);
	ops[197] = Opcode(this, "PUSH BC     ", 4, 1, 0, 0, &LR35902::PUSH_BC);
	ops[198] = Opcode(this, "ADD A,d8    ", 2, 2, 0, 0, &LR35902::ADD_A_d8);
	ops[199] = Opcode(this, "RST 00H     ", 4, 1, 0, 0, &LR35902::RST_00H);
	ops[200] = Opcode(this, "RET Z       ", 2, 1, 0, 0, &LR35902::RET_Z);
	ops[201] = Opcode(this, "RET         ", 4, 1, 0, 0, &LR35902::RET);
	ops[202] = Opcode(this, "JP Z,a16    ", 3, 3, 0, 0, &LR35902::JP_Z_d16);
	ops[203] = Opcode(this, "PREFIX CB   ", 0, 1, 0, 0, 0x0);
	ops[204] = Opcode(this, "CALL Z,a16  ", 3, 3, 0, 0, &LR35902::CALL_Z_a16);
	ops[205] = Opcode(this, "CALL a16    ", 6, 3, 0, 0, &LR35902::CALL_a16);
	ops[206] = Opcode(this, "ADC A,d8    ", 2, 2, 0, 0, &LR35902::ADC_A_d8);
	ops[207] = Opcode(this, "RST 08H     ", 4, 1, 0, 0, &LR35902::RST_08H);
	ops[208] = Opcode(this, "RET NC      ", 2, 1, 0, 0, &LR35902::RET_NC);
	ops[209] = Opcode(this, "POP DE      ", 3, 1, 0, 0, &LR35902::POP_DE);
	ops[210] = Opcode(this, "JP NC,a16   ", 3, 3, 0, 0, &LR35902::JP_NC_d16);
	ops[211] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[212] = Opcode(this, "CALL NC,a16 ", 3, 3, 0, 0, &LR35902::CALL_NC_a16);
	ops[213] = Opcode(this, "PUSH DE     ", 4, 1, 0, 0, &LR35902::PUSH_DE);
	ops[214] = Opcode(this, "SUB d8      ", 2, 2, 0, 0, &LR35902::SUB_d8);
	ops[215] = Opcode(this, "RST 10H     ", 4, 1, 0, 0, &LR35902::RST_10H);
	ops[216] = Opcode(this, "RET C       ", 2, 1, 0, 0, &LR35902::RET_C);
	ops[217] = Opcode(this, "RETI        ", 4, 1, 0, 0, &LR35902::RETI);
	ops[218] = Opcode(this, "JP C,a16    ", 3, 3, 0, 0, &LR35902::JP_C_d16);
	ops[219] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[220] = Opcode(this, "CALL C,a16  ", 3, 3, 0, 0, &LR35902::CALL_C_a16);
	ops[221] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[222] = Opcode(this, "SBC A,d8    ", 2, 2, 0, 0, &LR35902::SBC_A_d8);
	ops[223] = Opcode(this, "RST 18H     ", 4, 1, 0, 0, &LR35902::RST_18H);
	ops[224] = Opcode(this, "LDH (a8),A  ", 3, 2, 0, 3, &LR35902::LDH_a8_A);
	ops[225] = Opcode(this, "POP HL      ", 3, 1, 0, 0, &LR35902::POP_HL);
	ops[226] = Opcode(this, "LD (C),A    ", 2, 1, 0, 2, &LR35902::LD_aC_A);
	ops[227] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[228] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[229] = Opcode(this, "PUSH HL     ", 4, 1, 0, 0, &LR35902::PUSH_HL);
	ops[230] = Opcode(this, "AND d8      ", 2, 2, 0, 0, &LR35902::AND_d8);
	ops[231] = Opcode(this, "RST 20H     ", 4, 1, 0, 0, &LR35902::RST_20H);
	ops[232] = Opcode(this, "ADD SP,r8   ", 4, 2, 0, 0, &LR35902::ADD_SP_r8);
	ops[233] = Opcode(this, "JP (HL)     ", 1, 1, 0, 0, &LR35902::JP_aHL);
	ops[234] = Opcode(this, "LD (a16),A  ", 4, 3, 0, 4, &LR35902::LD_a16_A);
	ops[235] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[236] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[237] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[238] = Opcode(this, "XOR d8      ", 2, 2, 0, 0, &LR35902::XOR_d8);
	ops[239] = Opcode(this, "RST 28H     ", 4, 1, 0, 0, &LR35902::RST_28H);
	ops[240] = Opcode(this, "LDH A,(a8)  ", 3, 2, 3, 0, &LR35902::LDH_A_a8);
	ops[241] = Opcode(this, "POP AF      ", 3, 1, 0, 0, &LR35902::POP_AF);
	ops[242] = Opcode(this, "LD A,(C)    ", 2, 1, 2, 0, &LR35902::LD_A_aC);
	ops[243] = Opcode(this, "DI          ", 1, 1, 0, 0, &LR35902::DI);
	ops[244] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[245] = Opcode(this, "PUSH AF     ", 4, 1, 0, 0, &LR35902::PUSH_AF);
	ops[246] = Opcode(this, "OR d8       ", 2, 2, 0, 0, &LR35902::OR_d8);
	ops[247] = Opcode(this, "RST 30H     ", 4, 1, 0, 0, &LR35902::RST_30H);
	ops[248] = Opcode(this, "LD HL,SP+r8 ", 3, 2, 0, 0, &LR35902::LD_HL_SP_r8);
	ops[249] = Opcode(this, "LD SP,HL    ", 2, 1, 0, 0, &LR35902::LD_SP_HL);
	ops[250] = Opcode(this, "LD A,(a16)  ", 4, 3, 4, 0, &LR35902::LD_A_a16);
	ops[251] = Opcode(this, "EI          ", 1, 1, 0, 0, &LR35902::EI);
	ops[252] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[253] = Opcode(this, "            ", 0, 1, 0, 0, &LR35902::NOP);
	ops[254] = Opcode(this, "CP d8       ", 2, 2, 0, 0, &LR35902::CP_d8);
	ops[255] = Opcode(this, "RST 38H     ", 4, 1, 0, 0, &LR35902::RST_38H);

	ops = opcodes.getOpcodesCB();

	// CB prefix opcodes
	//                      Mnemonic        C  L  R  W  Pointer
	ops[0]   = Opcode(this, "RLC B       ", 2, 1, 0, 0, &LR35902::RLC_B);
	ops[1]   = Opcode(this, "RLC C       ", 2, 1, 0, 0, &LR35902::RLC_C);
	ops[2]   = Opcode(this, "RLC D       ", 2, 1, 0, 0, &LR35902::RLC_D);
	ops[3]   = Opcode(this, "RLC E       ", 2, 1, 0, 0, &LR35902::RLC_E);
	ops[4]   = Opcode(this, "RLC H       ", 2, 1, 0, 0, &LR35902::RLC_H);
	ops[5]   = Opcode(this, "RLC L       ", 2, 1, 0, 0, &LR35902::RLC_L);
	ops[6]   = Opcode(this, "RLC (HL)    ", 4, 1, 3, 4, &LR35902::RLC_aHL);
	ops[7]   = Opcode(this, "RLC A       ", 2, 1, 0, 0, &LR35902::RLC_A);
	ops[8]   = Opcode(this, "RRC B       ", 2, 1, 0, 0, &LR35902::RRC_B);
	ops[9]   = Opcode(this, "RRC C       ", 2, 1, 0, 0, &LR35902::RRC_C);
	ops[10]  = Opcode(this, "RRC D       ", 2, 1, 0, 0, &LR35902::RRC_D);
	ops[11]  = Opcode(this, "RRC E       ", 2, 1, 0, 0, &LR35902::RRC_E);
	ops[12]  = Opcode(this, "RRC H       ", 2, 1, 0, 0, &LR35902::RRC_H);
	ops[13]  = Opcode(this, "RRC L       ", 2, 1, 0, 0, &LR35902::RRC_L);
	ops[14]  = Opcode(this, "RRC (HL)    ", 4, 1, 3, 4, &LR35902::RRC_aHL);
	ops[15]  = Opcode(this, "RRC A       ", 2, 1, 0, 0, &LR35902::RRC_A);
	ops[16]  = Opcode(this, "RL B        ", 2, 1, 0, 0, &LR35902::RL_B);
	ops[17]  = Opcode(this, "RL C        ", 2, 1, 0, 0, &LR35902::RL_C);
	ops[18]  = Opcode(this, "RL D        ", 2, 1, 0, 0, &LR35902::RL_D);
	ops[19]  = Opcode(this, "RL E        ", 2, 1, 0, 0, &LR35902::RL_E);
	ops[20]  = Opcode(this, "RL H        ", 2, 1, 0, 0, &LR35902::RL_H);
	ops[21]  = Opcode(this, "RL L        ", 2, 1, 0, 0, &LR35902::RL_L);
	ops[22]  = Opcode(this, "RL (HL)     ", 4, 1, 3, 4, &LR35902::RL_aHL);
	ops[23]  = Opcode(this, "RL A        ", 2, 1, 0, 0, &LR35902::RL_A);
	ops[24]  = Opcode(this, "RR B        ", 2, 1, 0, 0, &LR35902::RR_B);
	ops[25]  = Opcode(this, "RR C        ", 2, 1, 0, 0, &LR35902::RR_C);
	ops[26]  = Opcode(this, "RR D        ", 2, 1, 0, 0, &LR35902::RR_D);
	ops[27]  = Opcode(this, "RR E        ", 2, 1, 0, 0, &LR35902::RR_E);
	ops[28]  = Opcode(this, "RR H        ", 2, 1, 0, 0, &LR35902::RR_H);
	ops[29]  = Opcode(this, "RR L        ", 2, 1, 0, 0, &LR35902::RR_L);
	ops[30]  = Opcode(this, "RR (HL)     ", 4, 1, 3, 4, &LR35902::RR_aHL);
	ops[31]  = Opcode(this, "RR A        ", 2, 1, 0, 0, &LR35902::RR_A);
	ops[32]  = Opcode(this, "SLA B       ", 2, 1, 0, 0, &LR35902::SLA_B);
	ops[33]  = Opcode(this, "SLA C       ", 2, 1, 0, 0, &LR35902::SLA_C);
	ops[34]  = Opcode(this, "SLA D       ", 2, 1, 0, 0, &LR35902::SLA_D);
	ops[35]  = Opcode(this, "SLA E       ", 2, 1, 0, 0, &LR35902::SLA_E);
	ops[36]  = Opcode(this, "SLA H       ", 2, 1, 0, 0, &LR35902::SLA_H);
	ops[37]  = Opcode(this, "SLA L       ", 2, 1, 0, 0, &LR35902::SLA_L);
	ops[38]  = Opcode(this, "SLA (HL)    ", 4, 1, 3, 4, &LR35902::SLA_aHL);
	ops[39]  = Opcode(this, "SLA A       ", 2, 1, 0, 0, &LR35902::SLA_A);
	ops[40]  = Opcode(this, "SRA B       ", 2, 1, 0, 0, &LR35902::SRA_B);
	ops[41]  = Opcode(this, "SRA C       ", 2, 1, 0, 0, &LR35902::SRA_C);
	ops[42]  = Opcode(this, "SRA D       ", 2, 1, 0, 0, &LR35902::SRA_D);
	ops[43]  = Opcode(this, "SRA E       ", 2, 1, 0, 0, &LR35902::SRA_E);
	ops[44]  = Opcode(this, "SRA H       ", 2, 1, 0, 0, &LR35902::SRA_H);
	ops[45]  = Opcode(this, "SRA L       ", 2, 1, 0, 0, &LR35902::SRA_L);
	ops[46]  = Opcode(this, "SRA (HL)    ", 4, 1, 3, 4, &LR35902::SRA_aHL);
	ops[47]  = Opcode(this, "SRA A       ", 2, 1, 0, 0, &LR35902::SRA_A);
	ops[48]  = Opcode(this, "SWAP B      ", 2, 1, 0, 0, &LR35902::SWAP_B);
	ops[49]  = Opcode(this, "SWAP C      ", 2, 1, 0, 0, &LR35902::SWAP_C);
	ops[50]  = Opcode(this, "SWAP D      ", 2, 1, 0, 0, &LR35902::SWAP_D);
	ops[51]  = Opcode(this, "SWAP E      ", 2, 1, 0, 0, &LR35902::SWAP_E);
	ops[52]  = Opcode(this, "SWAP H      ", 2, 1, 0, 0, &LR35902::SWAP_H);
	ops[53]  = Opcode(this, "SWAP L      ", 2, 1, 0, 0, &LR35902::SWAP_L);
	ops[54]  = Opcode(this, "SWAP (HL)   ", 4, 1, 3, 4, &LR35902::SWAP_aHL);
	ops[55]  = Opcode(this, "SWAP A      ", 2, 1, 0, 0, &LR35902::SWAP_A);
	ops[56]  = Opcode(this, "SRL B       ", 2, 1, 0, 0, &LR35902::SRL_B);
	ops[57]  = Opcode(this, "SRL C       ", 2, 1, 0, 0, &LR35902::SRL_C);
	ops[58]  = Opcode(this, "SRL D       ", 2, 1, 0, 0, &LR35902::SRL_D);
	ops[59]  = Opcode(this, "SRL E       ", 2, 1, 0, 0, &LR35902::SRL_E);
	ops[60]  = Opcode(this, "SRL H       ", 2, 1, 0, 0, &LR35902::SRL_H);
	ops[61]  = Opcode(this, "SRL L       ", 2, 1, 0, 0, &LR35902::SRL_L);
	ops[62]  = Opcode(this, "SRL (HL)    ", 4, 1, 3, 4, &LR35902::SRL_aHL);
	ops[63]  = Opcode(this, "SRL A       ", 2, 1, 0, 0, &LR35902::SRL_A);
	ops[64]  = Opcode(this, "BIT 0,B     ", 2, 1, 0, 0, &LR35902::BIT_0_B);
	ops[65]  = Opcode(this, "BIT 0,C     ", 2, 1, 0, 0, &LR35902::BIT_0_C);
	ops[66]  = Opcode(this, "BIT 0,D     ", 2, 1, 0, 0, &LR35902::BIT_0_D);
	ops[67]  = Opcode(this, "BIT 0,E     ", 2, 1, 0, 0, &LR35902::BIT_0_E);
	ops[68]  = Opcode(this, "BIT 0,H     ", 2, 1, 0, 0, &LR35902::BIT_0_H);
	ops[69]  = Opcode(this, "BIT 0,L     ", 2, 1, 0, 0, &LR35902::BIT_0_L);
	ops[70]  = Opcode(this, "BIT 0,(HL)  ", 3, 1, 3, 0, &LR35902::BIT_0_aHL);
	ops[71]  = Opcode(this, "BIT 0,A     ", 2, 1, 0, 0, &LR35902::BIT_0_A);
	ops[72]  = Opcode(this, "BIT 1,B     ", 2, 1, 0, 0, &LR35902::BIT_1_B);
	ops[73]  = Opcode(this, "BIT 1,C     ", 2, 1, 0, 0, &LR35902::BIT_1_C);
	ops[74]  = Opcode(this, "BIT 1,D     ", 2, 1, 0, 0, &LR35902::BIT_1_D);
	ops[75]  = Opcode(this, "BIT 1,E     ", 2, 1, 0, 0, &LR35902::BIT_1_E);
	ops[76]  = Opcode(this, "BIT 1,H     ", 2, 1, 0, 0, &LR35902::BIT_1_H);
	ops[77]  = Opcode(this, "BIT 1,L     ", 2, 1, 0, 0, &LR35902::BIT_1_L);
	ops[78]  = Opcode(this, "BIT 1,(HL)  ", 3, 1, 3, 0, &LR35902::BIT_1_aHL);
	ops[79]  = Opcode(this, "BIT 1,A     ", 2, 1, 0, 0, &LR35902::BIT_1_A);
	ops[80]  = Opcode(this, "BIT 2,B     ", 2, 1, 0, 0, &LR35902::BIT_2_B);
	ops[81]  = Opcode(this, "BIT 2,C     ", 2, 1, 0, 0, &LR35902::BIT_2_C);
	ops[82]  = Opcode(this, "BIT 2,D     ", 2, 1, 0, 0, &LR35902::BIT_2_D);
	ops[83]  = Opcode(this, "BIT 2,E     ", 2, 1, 0, 0, &LR35902::BIT_2_E);
	ops[84]  = Opcode(this, "BIT 2,H     ", 2, 1, 0, 0, &LR35902::BIT_2_H);
	ops[85]  = Opcode(this, "BIT 2,L     ", 2, 1, 0, 0, &LR35902::BIT_2_L);
	ops[86]  = Opcode(this, "BIT 2,(HL)  ", 3, 1, 3, 0, &LR35902::BIT_2_aHL);
	ops[87]  = Opcode(this, "BIT 2,A     ", 2, 1, 0, 0, &LR35902::BIT_2_A);
	ops[88]  = Opcode(this, "BIT 3,B     ", 2, 1, 0, 0, &LR35902::BIT_3_B);
	ops[89]  = Opcode(this, "BIT 3,C     ", 2, 1, 0, 0, &LR35902::BIT_3_C);
	ops[90]  = Opcode(this, "BIT 3,D     ", 2, 1, 0, 0, &LR35902::BIT_3_D);
	ops[91]  = Opcode(this, "BIT 3,E     ", 2, 1, 0, 0, &LR35902::BIT_3_E);
	ops[92]  = Opcode(this, "BIT 3,H     ", 2, 1, 0, 0, &LR35902::BIT_3_H);
	ops[93]  = Opcode(this, "BIT 3,L     ", 2, 1, 0, 0, &LR35902::BIT_3_L);
	ops[94]  = Opcode(this, "BIT 3,(HL)  ", 3, 1, 3, 0, &LR35902::BIT_3_aHL);
	ops[95]  = Opcode(this, "BIT 3,A     ", 2, 1, 0, 0, &LR35902::BIT_3_A);
	ops[96]  = Opcode(this, "BIT 4,B     ", 2, 1, 0, 0, &LR35902::BIT_4_B);
	ops[97]  = Opcode(this, "BIT 4,C     ", 2, 1, 0, 0, &LR35902::BIT_4_C);
	ops[98]  = Opcode(this, "BIT 4,D     ", 2, 1, 0, 0, &LR35902::BIT_4_D);
	ops[99]  = Opcode(this, "BIT 4,E     ", 2, 1, 0, 0, &LR35902::BIT_4_E);
	ops[100] = Opcode(this, "BIT 4,H     ", 2, 1, 0, 0, &LR35902::BIT_4_H);
	ops[101] = Opcode(this, "BIT 4,L     ", 2, 1, 0, 0, &LR35902::BIT_4_L);
	ops[102] = Opcode(this, "BIT 4,(HL)  ", 3, 1, 3, 0, &LR35902::BIT_4_aHL);
	ops[103] = Opcode(this, "BIT 4,A     ", 2, 1, 0, 0, &LR35902::BIT_4_A);
	ops[104] = Opcode(this, "BIT 5,B     ", 2, 1, 0, 0, &LR35902::BIT_5_B);
	ops[105] = Opcode(this, "BIT 5,C     ", 2, 1, 0, 0, &LR35902::BIT_5_C);
	ops[106] = Opcode(this, "BIT 5,D     ", 2, 1, 0, 0, &LR35902::BIT_5_D);
	ops[107] = Opcode(this, "BIT 5,E     ", 2, 1, 0, 0, &LR35902::BIT_5_E);
	ops[108] = Opcode(this, "BIT 5,H     ", 2, 1, 0, 0, &LR35902::BIT_5_H);
	ops[109] = Opcode(this, "BIT 5,L     ", 2, 1, 0, 0, &LR35902::BIT_5_L);
	ops[110] = Opcode(this, "BIT 5,(HL)  ", 3, 1, 3, 0, &LR35902::BIT_5_aHL);
	ops[111] = Opcode(this, "BIT 5,A     ", 2, 1, 0, 0, &LR35902::BIT_5_A);
	ops[112] = Opcode(this, "BIT 6,B     ", 2, 1, 0, 0, &LR35902::BIT_6_B);
	ops[113] = Opcode(this, "BIT 6,C     ", 2, 1, 0, 0, &LR35902::BIT_6_C);
	ops[114] = Opcode(this, "BIT 6,D     ", 2, 1, 0, 0, &LR35902::BIT_6_D);
	ops[115] = Opcode(this, "BIT 6,E     ", 2, 1, 0, 0, &LR35902::BIT_6_E);
	ops[116] = Opcode(this, "BIT 6,H     ", 2, 1, 0, 0, &LR35902::BIT_6_H);
	ops[117] = Opcode(this, "BIT 6,L     ", 2, 1, 0, 0, &LR35902::BIT_6_L);
	ops[118] = Opcode(this, "BIT 6,(HL)  ", 3, 1, 3, 0, &LR35902::BIT_6_aHL);
	ops[119] = Opcode(this, "BIT 6,A     ", 2, 1, 0, 0, &LR35902::BIT_6_A);
	ops[120] = Opcode(this, "BIT 7,B     ", 2, 1, 0, 0, &LR35902::BIT_7_B);
	ops[121] = Opcode(this, "BIT 7,C     ", 2, 1, 0, 0, &LR35902::BIT_7_C);
	ops[122] = Opcode(this, "BIT 7,D     ", 2, 1, 0, 0, &LR35902::BIT_7_D);
	ops[123] = Opcode(this, "BIT 7,E     ", 2, 1, 0, 0, &LR35902::BIT_7_E);
	ops[124] = Opcode(this, "BIT 7,H     ", 2, 1, 0, 0, &LR35902::BIT_7_H);
	ops[125] = Opcode(this, "BIT 7,L     ", 2, 1, 0, 0, &LR35902::BIT_7_L);
	ops[126] = Opcode(this, "BIT 7,(HL)  ", 3, 1, 3, 0, &LR35902::BIT_7_aHL);
	ops[127] = Opcode(this, "BIT 7,A     ", 2, 1, 0, 0, &LR35902::BIT_7_A);
	ops[128] = Opcode(this, "RES 0,B     ", 2, 1, 0, 0, &LR35902::RES_0_B);
	ops[129] = Opcode(this, "RES 0,C     ", 2, 1, 0, 0, &LR35902::RES_0_C);
	ops[130] = Opcode(this, "RES 0,D     ", 2, 1, 0, 0, &LR35902::RES_0_D);
	ops[131] = Opcode(this, "RES 0,E     ", 2, 1, 0, 0, &LR35902::RES_0_E);
	ops[132] = Opcode(this, "RES 0,H     ", 2, 1, 0, 0, &LR35902::RES_0_H);
	ops[133] = Opcode(this, "RES 0,L     ", 2, 1, 0, 0, &LR35902::RES_0_L);
	ops[134] = Opcode(this, "RES 0,(HL)  ", 4, 1, 3, 4, &LR35902::RES_0_aHL);
	ops[135] = Opcode(this, "RES 0,A     ", 2, 1, 0, 0, &LR35902::RES_0_A);
	ops[136] = Opcode(this, "RES 1,B     ", 2, 1, 0, 0, &LR35902::RES_1_B);
	ops[137] = Opcode(this, "RES 1,C     ", 2, 1, 0, 0, &LR35902::RES_1_C);
	ops[138] = Opcode(this, "RES 1,D     ", 2, 1, 0, 0, &LR35902::RES_1_D);
	ops[139] = Opcode(this, "RES 1,E     ", 2, 1, 0, 0, &LR35902::RES_1_E);
	ops[140] = Opcode(this, "RES 1,H     ", 2, 1, 0, 0, &LR35902::RES_1_H);
	ops[141] = Opcode(this, "RES 1,L     ", 2, 1, 0, 0, &LR35902::RES_1_L);
	ops[142] = Opcode(this, "RES 1,(HL)  ", 4, 1, 3, 4, &LR35902::RES_1_aHL);
	ops[143] = Opcode(this, "RES 1,A     ", 2, 1, 0, 0, &LR35902::RES_1_A);
	ops[144] = Opcode(this, "RES 2,B     ", 2, 1, 0, 0, &LR35902::RES_2_B);
	ops[145] = Opcode(this, "RES 2,C     ", 2, 1, 0, 0, &LR35902::RES_2_C);
	ops[146] = Opcode(this, "RES 2,D     ", 2, 1, 0, 0, &LR35902::RES_2_D);
	ops[147] = Opcode(this, "RES 2,E     ", 2, 1, 0, 0, &LR35902::RES_2_E);
	ops[148] = Opcode(this, "RES 2,H     ", 2, 1, 0, 0, &LR35902::RES_2_H);
	ops[149] = Opcode(this, "RES 2,L     ", 2, 1, 0, 0, &LR35902::RES_2_L);
	ops[150] = Opcode(this, "RES 2,(HL)  ", 4, 1, 3, 4, &LR35902::RES_2_aHL);
	ops[151] = Opcode(this, "RES 2,A     ", 2, 1, 0, 0, &LR35902::RES_2_A);
	ops[152] = Opcode(this, "RES 3,B     ", 2, 1, 0, 0, &LR35902::RES_3_B);
	ops[153] = Opcode(this, "RES 3,C     ", 2, 1, 0, 0, &LR35902::RES_3_C);
	ops[154] = Opcode(this, "RES 3,D     ", 2, 1, 0, 0, &LR35902::RES_3_D);
	ops[155] = Opcode(this, "RES 3,E     ", 2, 1, 0, 0, &LR35902::RES_3_E);
	ops[156] = Opcode(this, "RES 3,H     ", 2, 1, 0, 0, &LR35902::RES_3_H);
	ops[157] = Opcode(this, "RES 3,L     ", 2, 1, 0, 0, &LR35902::RES_3_L);
	ops[158] = Opcode(this, "RES 3,(HL)  ", 4, 1, 3, 4, &LR35902::RES_3_aHL);
	ops[159] = Opcode(this, "RES 3,A     ", 2, 1, 0, 0, &LR35902::RES_3_A);
	ops[160] = Opcode(this, "RES 4,B     ", 2, 1, 0, 0, &LR35902::RES_4_B);
	ops[161] = Opcode(this, "RES 4,C     ", 2, 1, 0, 0, &LR35902::RES_4_C);
	ops[162] = Opcode(this, "RES 4,D     ", 2, 1, 0, 0, &LR35902::RES_4_D);
	ops[163] = Opcode(this, "RES 4,E     ", 2, 1, 0, 0, &LR35902::RES_4_E);
	ops[164] = Opcode(this, "RES 4,H     ", 2, 1, 0, 0, &LR35902::RES_4_H);
	ops[165] = Opcode(this, "RES 4,L     ", 2, 1, 0, 0, &LR35902::RES_4_L);
	ops[166] = Opcode(this, "RES 4,(HL)  ", 4, 1, 3, 4, &LR35902::RES_4_aHL);
	ops[167] = Opcode(this, "RES 4,A     ", 2, 1, 0, 0, &LR35902::RES_4_A);
	ops[168] = Opcode(this, "RES 5,B     ", 2, 1, 0, 0, &LR35902::RES_5_B);
	ops[169] = Opcode(this, "RES 5,C     ", 2, 1, 0, 0, &LR35902::RES_5_C);
	ops[170] = Opcode(this, "RES 5,D     ", 2, 1, 0, 0, &LR35902::RES_5_D);
	ops[171] = Opcode(this, "RES 5,E     ", 2, 1, 0, 0, &LR35902::RES_5_E);
	ops[172] = Opcode(this, "RES 5,H     ", 2, 1, 0, 0, &LR35902::RES_5_H);
	ops[173] = Opcode(this, "RES 5,L     ", 2, 1, 0, 0, &LR35902::RES_5_L);
	ops[174] = Opcode(this, "RES 5,(HL)  ", 4, 1, 3, 4, &LR35902::RES_5_aHL);
	ops[175] = Opcode(this, "RES 5,A     ", 2, 1, 0, 0, &LR35902::RES_5_A);
	ops[176] = Opcode(this, "RES 6,B     ", 2, 1, 0, 0, &LR35902::RES_6_B);
	ops[177] = Opcode(this, "RES 6,C     ", 2, 1, 0, 0, &LR35902::RES_6_C);
	ops[178] = Opcode(this, "RES 6,D     ", 2, 1, 0, 0, &LR35902::RES_6_D);
	ops[179] = Opcode(this, "RES 6,E     ", 2, 1, 0, 0, &LR35902::RES_6_E);
	ops[180] = Opcode(this, "RES 6,H     ", 2, 1, 0, 0, &LR35902::RES_6_H);
	ops[181] = Opcode(this, "RES 6,L     ", 2, 1, 0, 0, &LR35902::RES_6_L);
	ops[182] = Opcode(this, "RES 6,(HL)  ", 4, 1, 3, 4, &LR35902::RES_6_aHL);
	ops[183] = Opcode(this, "RES 6,A     ", 2, 1, 0, 0, &LR35902::RES_6_A);
	ops[184] = Opcode(this, "RES 7,B     ", 2, 1, 0, 0, &LR35902::RES_7_B);
	ops[185] = Opcode(this, "RES 7,C     ", 2, 1, 0, 0, &LR35902::RES_7_C);
	ops[186] = Opcode(this, "RES 7,D     ", 2, 1, 0, 0, &LR35902::RES_7_D);
	ops[187] = Opcode(this, "RES 7,E     ", 2, 1, 0, 0, &LR35902::RES_7_E);
	ops[188] = Opcode(this, "RES 7,H     ", 2, 1, 0, 0, &LR35902::RES_7_H);
	ops[189] = Opcode(this, "RES 7,L     ", 2, 1, 0, 0, &LR35902::RES_7_L);
	ops[190] = Opcode(this, "RES 7,(HL)  ", 4, 1, 3, 4, &LR35902::RES_7_aHL);
	ops[191] = Opcode(this, "RES 7,A     ", 2, 1, 0, 0, &LR35902::RES_7_A);
	ops[192] = Opcode(this, "SET 0,B     ", 2, 1, 0, 0, &LR35902::SET_0_B);
	ops[193] = Opcode(this, "SET 0,C     ", 2, 1, 0, 0, &LR35902::SET_0_C);
	ops[194] = Opcode(this, "SET 0,D     ", 2, 1, 0, 0, &LR35902::SET_0_D);
	ops[195] = Opcode(this, "SET 0,E     ", 2, 1, 0, 0, &LR35902::SET_0_E);
	ops[196] = Opcode(this, "SET 0,H     ", 2, 1, 0, 0, &LR35902::SET_0_H);
	ops[197] = Opcode(this, "SET 0,L     ", 2, 1, 0, 0, &LR35902::SET_0_L);
	ops[198] = Opcode(this, "SET 0,(HL)  ", 4, 1, 3, 4, &LR35902::SET_0_aHL);
	ops[199] = Opcode(this, "SET 0,A     ", 2, 1, 0, 0, &LR35902::SET_0_A);
	ops[200] = Opcode(this, "SET 1,B     ", 2, 1, 0, 0, &LR35902::SET_1_B);
	ops[201] = Opcode(this, "SET 1,C     ", 2, 1, 0, 0, &LR35902::SET_1_C);
	ops[202] = Opcode(this, "SET 1,D     ", 2, 1, 0, 0, &LR35902::SET_1_D);
	ops[203] = Opcode(this, "SET 1,E     ", 2, 1, 0, 0, &LR35902::SET_1_E);
	ops[204] = Opcode(this, "SET 1,H     ", 2, 1, 0, 0, &LR35902::SET_1_H);
	ops[205] = Opcode(this, "SET 1,L     ", 2, 1, 0, 0, &LR35902::SET_1_L);
	ops[206] = Opcode(this, "SET 1,(HL)  ", 4, 1, 3, 4, &LR35902::SET_1_aHL);
	ops[207] = Opcode(this, "SET 1,A     ", 2, 1, 0, 0, &LR35902::SET_1_A);
	ops[208] = Opcode(this, "SET 2,B     ", 2, 1, 0, 0, &LR35902::SET_2_B);
	ops[209] = Opcode(this, "SET 2,C     ", 2, 1, 0, 0, &LR35902::SET_2_C);
	ops[210] = Opcode(this, "SET 2,D     ", 2, 1, 0, 0, &LR35902::SET_2_D);
	ops[211] = Opcode(this, "SET 2,E     ", 2, 1, 0, 0, &LR35902::SET_2_E);
	ops[212] = Opcode(this, "SET 2,H     ", 2, 1, 0, 0, &LR35902::SET_2_H);
	ops[213] = Opcode(this, "SET 2,L     ", 2, 1, 0, 0, &LR35902::SET_2_L);
	ops[214] = Opcode(this, "SET 2,(HL)  ", 4, 1, 3, 4, &LR35902::SET_2_aHL);
	ops[215] = Opcode(this, "SET 2,A     ", 2, 1, 0, 0, &LR35902::SET_2_A);
	ops[216] = Opcode(this, "SET 3,B     ", 2, 1, 0, 0, &LR35902::SET_3_B);
	ops[217] = Opcode(this, "SET 3,C     ", 2, 1, 0, 0, &LR35902::SET_3_C);
	ops[218] = Opcode(this, "SET 3,D     ", 2, 1, 0, 0, &LR35902::SET_3_D);
	ops[219] = Opcode(this, "SET 3,E     ", 2, 1, 0, 0, &LR35902::SET_3_E);
	ops[220] = Opcode(this, "SET 3,H     ", 2, 1, 0, 0, &LR35902::SET_3_H);
	ops[221] = Opcode(this, "SET 3,L     ", 2, 1, 0, 0, &LR35902::SET_3_L);
	ops[222] = Opcode(this, "SET 3,(HL)  ", 4, 1, 3, 4, &LR35902::SET_3_aHL);
	ops[223] = Opcode(this, "SET 3,A     ", 2, 1, 0, 0, &LR35902::SET_3_A);
	ops[224] = Opcode(this, "SET 4,B     ", 2, 1, 0, 0, &LR35902::SET_4_B);
	ops[225] = Opcode(this, "SET 4,C     ", 2, 1, 0, 0, &LR35902::SET_4_C);
	ops[226] = Opcode(this, "SET 4,D     ", 2, 1, 0, 0, &LR35902::SET_4_D);
	ops[227] = Opcode(this, "SET 4,E     ", 2, 1, 0, 0, &LR35902::SET_4_E);
	ops[228] = Opcode(this, "SET 4,H     ", 2, 1, 0, 0, &LR35902::SET_4_H);
	ops[229] = Opcode(this, "SET 4,L     ", 2, 1, 0, 0, &LR35902::SET_4_L);
	ops[230] = Opcode(this, "SET 4,(HL)  ", 4, 1, 3, 4, &LR35902::SET_4_aHL);
	ops[231] = Opcode(this, "SET 4,A     ", 2, 1, 0, 0, &LR35902::SET_4_A);
	ops[232] = Opcode(this, "SET 5,B     ", 2, 1, 0, 0, &LR35902::SET_5_B);
	ops[233] = Opcode(this, "SET 5,C     ", 2, 1, 0, 0, &LR35902::SET_5_C);
	ops[234] = Opcode(this, "SET 5,D     ", 2, 1, 0, 0, &LR35902::SET_5_D);
	ops[235] = Opcode(this, "SET 5,E     ", 2, 1, 0, 0, &LR35902::SET_5_E);
	ops[236] = Opcode(this, "SET 5,H     ", 2, 1, 0, 0, &LR35902::SET_5_H);
	ops[237] = Opcode(this, "SET 5,L     ", 2, 1, 0, 0, &LR35902::SET_5_L);
	ops[238] = Opcode(this, "SET 5,(HL)  ", 4, 1, 3, 4, &LR35902::SET_5_aHL);
	ops[239] = Opcode(this, "SET 5,A     ", 2, 1, 0, 0, &LR35902::SET_5_A);
	ops[240] = Opcode(this, "SET 6,B     ", 2, 1, 0, 0, &LR35902::SET_6_B);
	ops[241] = Opcode(this, "SET 6,C     ", 2, 1, 0, 0, &LR35902::SET_6_C);
	ops[242] = Opcode(this, "SET 6,D     ", 2, 1, 0, 0, &LR35902::SET_6_D);
	ops[243] = Opcode(this, "SET 6,E     ", 2, 1, 0, 0, &LR35902::SET_6_E);
	ops[244] = Opcode(this, "SET 6,H     ", 2, 1, 0, 0, &LR35902::SET_6_H);
	ops[245] = Opcode(this, "SET 6,L     ", 2, 1, 0, 0, &LR35902::SET_6_L);
	ops[246] = Opcode(this, "SET 6,(HL)  ", 4, 1, 3, 4, &LR35902::SET_6_aHL);
	ops[247] = Opcode(this, "SET 6,A     ", 2, 1, 0, 0, &LR35902::SET_6_A);
	ops[248] = Opcode(this, "SET 7,B     ", 2, 1, 0, 0, &LR35902::SET_7_B);
	ops[249] = Opcode(this, "SET 7,C     ", 2, 1, 0, 0, &LR35902::SET_7_C);
	ops[250] = Opcode(this, "SET 7,D     ", 2, 1, 0, 0, &LR35902::SET_7_D);
	ops[251] = Opcode(this, "SET 7,E     ", 2, 1, 0, 0, &LR35902::SET_7_E);
	ops[252] = Opcode(this, "SET 7,H     ", 2, 1, 0, 0, &LR35902::SET_7_H);
	ops[253] = Opcode(this, "SET 7,L     ", 2, 1, 0, 0, &LR35902::SET_7_L);
	ops[254] = Opcode(this, "SET 7,(HL)  ", 4, 1, 3, 4, &LR35902::SET_7_aHL);
	ops[255] = Opcode(this, "SET 7,A     ", 2, 1, 0, 0, &LR35902::SET_7_A);	
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
