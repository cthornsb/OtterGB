#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>

#include "Support.hpp"
#include "LR35902.hpp"
#include "SystemGBC.hpp"
#include "Cartridge.hpp"
#include "Opcodes.hpp"

const unsigned char FLAG_Z_BIT = 7;
const unsigned char FLAG_S_BIT = 6;
const unsigned char FLAG_H_BIT = 5;
const unsigned char FLAG_C_BIT = 4;

const unsigned char FLAG_Z_MASK = 0x80;
const unsigned char FLAG_S_MASK = 0x40;
const unsigned char FLAG_H_MASK = 0x20;
const unsigned char FLAG_C_MASK = 0x10;

const unsigned char ZERO = 0x0;

unsigned short LR35902::execute(Cartridge *cart){
	// Start reading the rom
	std::string name;
	unsigned short len;
	unsigned char op;
	bool isPrefixCB = false;
	nCycles = 0; // Zero the CPU cycles counter

	if(PC == BP) // Check for instruction breakpoint
		debugMode = true;

	// Read an opcode
	if(!sys->read(PC++, &op))
		std::cout << " Opcode read failed! PC=" << getHex((unsigned short)(PC-1)) << std::endl;
	
	if(op != 0xCB){ // Normal opcode
		if(debugMode)
			name = opcodes[op];
		len = opcodeLengths[op];
		nCycles = opcodeCycles[op];
	}
	else{ // CB prefix
		isPrefixCB = true;
		sys->read(PC++, op);
		if(debugMode)
			name = opcodesCB[op];
		len = 1;
		nCycles = (op & 0xF != 0x6) ? 8 : 16;
	}
	
	d8 = 0x0;
	d16h = 0x0;
	d16l = 0x0;

	std::stringstream stream;
	if(debugMode){
		stream << getHex(A) << " " << getHex(B) << " " << getHex(C) << " " << getHex(D) << " " << getHex(E);
		stream << " " << getHex(H) << " " <<  getHex(L) << " F=" << getBinary(F, 4) << " PC=" << getHex((unsigned short)(PC-1)) << " SP=" << getHex(SP);
	}

	// Read the opcode's accompanying value (if any)
	if(len == 2){ // Read 8 bits (valid targets: d8, d8, d8)
		sys->read(PC++, d8);
	}
	else if(len == 3){ // Read 16 bits (valid targets: d16, d16)
		// Low byte read first!
		sys->read(PC++, d16l);
		sys->read(PC++, d16h);
	}

	if(debugMode)
		stream << " d8=" << getHex(d8) << " d16=" << getHex(get_d16()) << " " << name;

	if(!isPrefixCB)
		(this->*funcPtr[op])();
	else
		(this->*funcPtrCB[op])();

	if(debugMode){// && op != 0x0)
		std::cout << stream.str();

		// Wait for the user to hit enter
		std::string dummy;
		getline(std::cin, dummy);
	}
	
	return nCycles;
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

unsigned short LR35902::get_d16(){ return getUShort(d16h, d16l); }

unsigned short LR35902::getAF(){ return getUShort(A, F); }

unsigned short LR35902::getBC(){ return getUShort(B, C); }

unsigned short LR35902::getDE(){ return getUShort(D, E); }

unsigned short LR35902::getHL(){ return getUShort(H, L); }

unsigned short LR35902::setAF(const unsigned short &val){
	A = (0xFF00 & val) >> 8;
	F = 0x00FF & val;
}

unsigned short LR35902::setBC(const unsigned short &val){
	B = (0xFF00 & val) >> 8;
	C = 0x00FF & val;
}

unsigned short LR35902::setDE(const unsigned short &val){
	D = (0xFF00 & val) >> 8;
	E = 0x00FF & val;
}

unsigned short LR35902::setHL(const unsigned short &val){
	H = (0xFF00 & val) >> 8;
	L = 0x00FF & val;
}

void LR35902::rlc_d8(unsigned char *arg){
	// Left bitshift (arg), copying the 7th bit into the carry 
	// and into 0th bit of (arg).
	bool highBit = ((*arg) & 0x80) != 0;
	if(highBit) (*arg) = ((*arg) << 1) | 0x01;
	else        (*arg) = ((*arg) << 1) & 0xFE;
	setFlags((*arg == 0), 0, 0, highBit);
}

void LR35902::rrc_d8(unsigned char *arg){
	// Right bitshift (arg), copying the 0th bit into the carry 
	// and into 7th bit of (arg).
	bool lowBit = ((*arg) & 0x1) != 0;
	if(lowBit) (*arg) = ((*arg) >> 1) | 0x80;
	else       (*arg) = ((*arg) >> 1) & 0x7F;
	setFlags((*arg == 0), 0, 0, lowBit);
}

void LR35902::rl_d8(unsigned char *arg){
	// Left bitshift (arg), copying the 7th bit into the carry 
	// bit which is copied into 0th bit of (arg).
	bool highBit = ((*arg) & 0x80) != 0;
	if(F & FLAG_C_MASK != 0) (*arg) = ((*arg) << 1) | 0x01;
	else                     (*arg) = ((*arg) << 1) & 0xFE;
	setFlags((*arg == 0), 0, 0, highBit);
}

void LR35902::rr_d8(unsigned char *arg){
	// Right bitshift (arg), copying the 0th bit into the carry 
	// bit which is copied into 7th bit of (arg).
	bool lowBit = ((*arg) & 0x1) != 0;
	if(F & FLAG_C_MASK) (*arg) = ((*arg) >> 1) | 0x80;
	else                (*arg) = ((*arg) >> 1) & 0x7F;
	setFlags((*arg == 0), 0, 0, lowBit);
}

void LR35902::res_d8(unsigned char *arg, const unsigned char &bit){
	// Clear a bit of (arg).
	(*arg) = (*arg) & (~(0x1 << bit));
}

void LR35902::set_d8(unsigned char *arg, const unsigned char &bit){
	// Set a bit of (arg).
	(*arg) |= (0x1 << bit);
}

void LR35902::inc_d16(unsigned char *addrH, unsigned char *addrL){
	if(++(*addrL) == 0x0) // Half-Carry
		(*addrH)++;
}

void LR35902::dec_d16(unsigned char *addrH, unsigned char *addrL){
	if(--(*addrL) == 0xFF) // Half-Carry
		(*addrH)--;
}

void LR35902::inc_d8(unsigned char *arg){
	getCarries(arg);
	(*arg) = (*arg) + 1;
	setFlag(FLAG_Z_BIT, (*arg == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, halfCarry);
}

void LR35902::dec_d8(unsigned char *arg){
	getCarries(arg, true);
	(*arg) = (*arg) - 1;
	setFlag(FLAG_Z_BIT, (*arg == 0));
	setFlag(FLAG_S_BIT, 1);
	setFlag(FLAG_H_BIT, halfCarry);
}

void LR35902::jr_n(unsigned char *n){
	PC += twosComp(*n);
}

void LR35902::jr_cc_n(unsigned char *n){
	nCycles += 4; // Conditional JR takes 4 additional cycles if true (8->12)
	jr_n(n);
}

void LR35902::ld_d8(unsigned char *dest, unsigned char *src){
	(*dest) = (*src);
}

void LR35902::ld_SP_d16(unsigned char *addrH, unsigned char *addrL){
	// Load immediate 16-bit value into the stack pointer, SP.
	SP = getUShort(*addrH, *addrL);
}

void LR35902::add_A_d8(unsigned char *arg){
	getCarries(&A, arg);
	A += (*arg);
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

void LR35902::add_A_aHL(){
	add_A_d8(sys->getPtr(getHL()));
}

void LR35902::add_HL_d16(unsigned char *addrH, unsigned char *addrL){
	// Add d16 to HL.
	//getCarries(&H, addrH);
	setHL(getHL() + getUShort(*addrH, *addrL));
	setFlag(FLAG_S_BIT, 0);
	//setFlag(FLAG_H_BIT, halfCarry); // set if carry from bit 11
	//setFlag(FLAG_C_BIT, fullCarry); // set if carry from bit 15
}

void LR35902::adc_A_d8(unsigned char *arg){
	getCarries(&A, arg);
	A += ((*arg) + (F & FLAG_C_MASK));
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

void LR35902::sub_A_d8(unsigned char *arg){
	getCarries(&A, arg);
	A = A - (*arg);
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 1);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

void LR35902::sbc_A_d8(unsigned char *arg){
	getCarries(&A, arg);
	A = A - ((*arg) + (F & FLAG_C_MASK));
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 1);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

void LR35902::and_d8(unsigned char *arg){
	A &= (*arg);
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, 1);
	setFlag(FLAG_C_BIT, 0);		
}

void LR35902::xor_d8(unsigned char *arg){
	A ^= (*arg);
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, 0);
	setFlag(FLAG_C_BIT, 0);	
}

void LR35902::or_d8(unsigned char *arg){
	A |= (*arg);
	setFlag(FLAG_Z_BIT, (A == 0));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, 0);
	setFlag(FLAG_C_BIT, 0);
}

void LR35902::cp_d8(unsigned char *arg){
	getCarries(&A, arg);
	setFlag(FLAG_Z_BIT, (A - (*arg) == 0));
	setFlag(FLAG_S_BIT, 1);
	setFlag(FLAG_H_BIT, halfCarry);
	setFlag(FLAG_C_BIT, fullCarry);
}

void LR35902::push_d16(unsigned char *addrH, unsigned char *addrL){
	sys->write(SP-1, addrH);
	sys->write(SP-2, addrL);
	SP = SP-2;
}

void LR35902::pop_d16(unsigned char *addrH, unsigned char *addrL){
	sys->read(SP, addrL);
	sys->read(SP+1, addrH);
	SP = SP+2;
}

void LR35902::jp_d16(unsigned char *addrH, unsigned char *addrL){
	// Jump to address
	PC = ((*addrH) << 8) + (*addrL);
}

void LR35902::jp_cc_d16(unsigned char *addrH, unsigned char *addrL){
	nCycles += 4; // Conditional JP takes 4 additional cycles if true (12->16)
	jp_d16(addrH, addrL);
}

void LR35902::call_a16(unsigned char *addrH, unsigned char *addrL){
	sys->write(SP-1, (PC & 0xFF00) >> 8);
	sys->write(SP-2, PC & 0x00FF);
	jp_d16(addrH, addrL); 
	SP = SP-2;
}

void LR35902::call_cc_a16(unsigned char *addrH, unsigned char *addrL){
	nCycles += 12; // Conditional CALL takes 12 additional cycles if true (12->24)
	call_a16(addrH, addrL);
}

void LR35902::rst_n(unsigned char n){
	sys->write(SP-1, (PC & 0xFF00) >> 8);
	sys->write(SP-2, PC & 0x00FF);	
	PC = n & 0x00FF; // Zero the high bits
	SP = SP-2;
}

void LR35902::ret(){
	unsigned char pcl, pch;
	sys->read(SP, pcl);
	sys->read(SP+1, pch);
	PC = getUShort(pch, pcl);
	SP = SP+2;
}

void LR35902::ret_cc(){
	nCycles += 12; // Conditional RET takes 12 additional cycles if true (8->20)
	ret();
}

void LR35902::getCarries(unsigned char *arg1, unsigned char *arg2, bool sub/*=false*/){
	halfCarry = (((*arg1 & 0xF) + (!sub ? 1 : -1)*(*arg2 & 0xF)) & 0x10) != 0;
	fullCarry = (*arg1 + (!sub ? 1 : -1)*(*arg2)) & 0x100 != 0;
}

void LR35902::getCarries(unsigned char *arg1, bool sub/*=false*/){
	halfCarry = (((*arg1 & 0xF) + (!sub ? 1 : -1)) & 0x10) != 0;
	fullCarry = (*arg1 + (!sub ? 1 : -1)) & 0x100 != 0;
}

void LR35902::sla_d8(unsigned char *arg){
	// Left bitshift (arg), setting its 0th bit to 0 and moving
	// its 7th bit into the carry bit.
	bool highBit = ((*arg) & 0xFF) != 0;
	(*arg) = (*arg) << 1;
	setFlags((*arg == 0), 0, 0, highBit);
}

void LR35902::sra_d8(unsigned char *arg){
	// Right bitshift (arg), its 0th bit is moved to the carry
	// and its 7th bit retains its original value.
	bool lowBit = ((*arg) & 0x1) != 0;
	bool highBit = ((*arg) & 0xFF) != 0;
	(*arg) = ((*arg) >> 1) | highBit ? 0xFE : 0x1;
	setFlags((*arg == 0), 0, 0, lowBit);
}

void LR35902::swap_d8(unsigned char *arg){
	// Swap the low and high nibbles (4-bits) of (arg).
	char lowNibble = (*arg) & 0xF;
	char highNibble = (*arg) & 0xF0;
	(*arg) = (lowNibble << 4) + (highNibble >> 4);
	setFlags((*arg == 0), 0, 0, 0);
}

void LR35902::srl_d8(unsigned char *arg){
	// Right bitshift (arg), setting its 7th bit to 0 and moving
	// its 0th bit into the carry bit.
	bool lowBit = ((*arg) & 0x1) != 0;
	(*arg) = (*arg) >> 1;
	setFlags((*arg == 0), 0, 0, lowBit);
}

void LR35902::bit_d8(unsigned char *arg, const unsigned char &bit){
	// Copy a bit of (arg) to the zero bit.
	setFlag(FLAG_Z_BIT, ((*arg) & (0x1 << bit)));
	setFlag(FLAG_S_BIT, 0);
	setFlag(FLAG_H_BIT, 1);
}

void LR35902::callInterruptVector(const unsigned char &offset){
	// Interrupt vectors behave the same way that RST vectors do
	rst_n(offset);
}

/////////////////////////////////////////////////////////////////////
// OPCODES
/////////////////////////////////////////////////////////////////////

// INC BC[DE|HL|(HL)]

void LR35902::INC_aHL(){
	// Incremement memory location (HL).
	unsigned short HL = getHL();
	sys->write(HL, sys->getValue(HL)+1);
}

// DEC BC[DE|HL|(HL)]

void LR35902::DEC_aHL(){
	// Decrement memory location (HL).
	unsigned short HL = getHL();
	sys->write(HL, sys->getValue(HL)-1);
}

// DAA

void LR35902::DAA(){
	// Decimal adjust register A.
	// Convert register A into packed Binary Coded Decimal (BCD) representation.
	std::cout << "not implemented\n";
}

// CPL

void LR35902::CPL(){ A = ~A; }

// INC SP

void LR35902::INC_SP(){ SP++; }

// SCF

void LR35902::SCF(){ set_d8(&F, FLAG_C_BIT); }

// ADD SP,d8

void LR35902::ADD_SP_r8(){
	// Add d8 (signed) to the stack pointer
	SP += twosComp(d8);
	//setFlags(0, 0, halfCarry, fullCarry);
}

// LD d16,A 

void LR35902::LD_a16_A(){
	// Set memory location (a16) to register A.
	sys->write(get_d16(), &A);
}

// LD HL,SP+r8 or LDHL SP,r8

void LR35902::LD_HL_SP_r8(){
	// Load the address (SP+r8) into HL, note: r8 is signed.
	setHL(SP + twosComp(d8));
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
	unsigned short target = get_d16();
	sys->write(target, (0x00FF & SP)); // Write the low byte first
	sys->write(target+1, (0xFF00 &SP) >> 8); // Write the high byte last
}

// ADD HL,BC[DE|HL|SP]

void LR35902::ADD_HL_SP(){
	// Add SP to HL.
	setHL(getHL() + SP);
	setFlag(FLAG_S_BIT, 0);
	//setFlag(FLAG_H_BIT, // set if carry from bit 11
	//setFlag(FLAG_C_BIT, // set if carry from bit 15
}

// LD HL-,A or LDD HL,A

void LR35902::LDD_aHL_A(){
	// Write register A to memory address (HL) and decrement HL.
	unsigned short HL = getHL();
	sys->write(HL, &A);
	setHL(HL-1);
}

// LD A,HL- or LDD A,HL

void LR35902::LDD_A_aHL(){
	// Write memory address (HL) into register A and decrement HL.
	unsigned short HL = getHL();
	sys->read(HL, &A);
	setHL(HL-1);
}

// LD HL+,A or LDI HL,A

void LR35902::LDI_aHL_A(){
	// Write register A to memory address (HL) and increment HL.
	unsigned short HL = getHL();
	sys->write(HL, &A);
	setHL(HL+1);
}

// LD A,HL+ or LDI A,HL

void LR35902::LDI_A_aHL(){
	// Write memory address (HL) into register A and increment HL.
	unsigned short HL = getHL();
	sys->read(HL, &A);
	setHL(HL+1);
}

// LDH d8,A

void LR35902::LDH_a8_A(){
	// Put register A into high memory location ($FF00+a8).
	sys->write((0xFF00 + d8), &A);
}

// LDH A,d8

void LR35902::LDH_A_a8(){
	// Put high memory location ($FF00+a8) into register A.
	sys->read((0xFF00 + d8), &A);
}

// LD (C),A

void LR35902::LD_aC_A(){
	// Write register A into memory location (0xFF00+C).
	sys->write((0xFF00 + C), &A); 
}

// LD HL,A[B|C|D|E|H|L|d8]

void LR35902::ld_aHL_d8(unsigned char *arg){
	// Write d8 into memory location (HL).
	sys->write(getHL(), arg);
}

void LR35902::LD_aHL_d16(){
	// Write d16 into memory location (HL).
	unsigned short val = get_d16();
	unsigned short HL = getHL();
	sys->write(HL, (0x00FF & val)); // Write the low byte first
	sys->write(HL+1, (0xFF00 & val) >> 8); // Write the high byte last		
}

void LR35902::ld_d8_aHL(unsigned char *arg){
	// Load memory location (HL) into register arg.
	sys->read(getHL(), arg);
}

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

void LR35902::LD_aBC_A(){
	// Load A register into memory location (BC).
	sys->write(getBC(), &A);
}

void LR35902::LD_aDE_A(){
	// Load A register into memory location (DE).
	sys->write(getDE(), &A);
}

// LD A,A[B|C|D|E|H|L|d8]

void LR35902::ld_A_a16(unsigned char *addrH, unsigned char *addrL){
	// Load memory location (a16) into register A.
	sys->read(getUShort(*addrH, *addrL), &A);
}

void LR35902::LD_A_aC(){ 
	// Load memory location (0xFF00+C) into register A.
	sys->read((0xFF00 + C), &A);
}

// ADD A,A[B|C|D|E|H|L]

void LR35902::ADD_A_aHL(){
	add_A_d8(sys->getPtr(getHL()));
}

// ADC A,A[B|C|D|E|H|L]

void LR35902::ADC_A_aHL(){
	adc_A_d8(sys->getPtr(getHL()));
}

// SUB A[B|C|D|E|H|L]

void LR35902::SUB_aHL(){
	sub_A_d8(sys->getPtr(getHL()));
}

// SBC A,A[B|C|D|E|H|L]

void LR35902::SBC_A_aHL(){
	sbc_A_d8(sys->getPtr(getHL()));
}

// AND A[B|C|D|E|H|L]

void LR35902::AND_aHL(){
	and_d8(sys->getPtr(getHL()));
}

// XOR A[B|C|D|E|H|L]

void LR35902::XOR_aHL(){
	xor_d8(sys->getPtr(getHL()));
}

// OR A[B|C|D|E|H|L]

void LR35902::OR_aHL(){
	or_d8(sys->getPtr(getHL()));
}

// CP A[B|C|D|E|H|L]

void LR35902::CP_aHL(){
	cp_d8(sys->getPtr(getHL()));
}

// JP 

void LR35902::JP_aHL(){ 
	// Jump to memory address (HL).
	jp_d16(&H, &L);
}

// RET NZ[Z|NC|C]

void LR35902::RETI(){
	EI();
	ret();
}

// DI

void LR35902::DI(){ sys->enableInterrupts(false); }

// EI

void LR35902::EI(){ sys->enableInterrupts(); }

/////////////////////////////////////////////////////////////////////
// CB-PREFIX OPCODES
/////////////////////////////////////////////////////////////////////

// RLC A[B|C|D|E|H|L]

void LR35902::RLC_aHL(){
	rlc_d8(sys->getPtr(getHL()));
}

// RRC A[B|C|D|E|H|L]

void LR35902::RRC_aHL(){
	rrc_d8(sys->getPtr(getHL()));
}

// RL A[B|C|D|E|H|L]

void LR35902::RL_aHL(){
	rl_d8(sys->getPtr(getHL()));
}

// RR A[B|C|D|E|H|L]

void LR35902::RR_aHL(){
	rr_d8(sys->getPtr(getHL()));
}

// SLA A[B|C|D|E|H|L]

void LR35902::SLA_aHL(){
	sla_d8(sys->getPtr(getHL()));
}

// SRA A[B|C|D|E|H|L]

void LR35902::SRA_aHL(){
	sra_d8(sys->getPtr(getHL()));
}

// SWAP A[B|C|D|E|H|L]

void LR35902::SWAP_aHL(){
	swap_d8(sys->getPtr(getHL()));
}

// SRL A[B|C|D|E|H|L]

void LR35902::SRL_aHL(){
	srl_d8(sys->getPtr(getHL()));
}

// BIT 0[1|2|3|4|5|6|7],HL

void LR35902::BIT_0_aHL(){
	bit_d8(sys->getPtr(getHL()), 0);
}

void LR35902::BIT_1_aHL(){
	bit_d8(sys->getPtr(getHL()), 1);
}

void LR35902::BIT_2_aHL(){
	bit_d8(sys->getPtr(getHL()), 2);
}

void LR35902::BIT_3_aHL(){
	bit_d8(sys->getPtr(getHL()), 3);
}

void LR35902::BIT_4_aHL(){
	bit_d8(sys->getPtr(getHL()), 4);
}

void LR35902::BIT_5_aHL(){
	bit_d8(sys->getPtr(getHL()), 5);
}

void LR35902::BIT_6_aHL(){
	bit_d8(sys->getPtr(getHL()), 6);
}

void LR35902::BIT_7_aHL(){
	bit_d8(sys->getPtr(getHL()), 7);
}

// RES 0[1|2|3|4|5|6|7],HL

void LR35902::RES_0_aHL(){
	res_d8(sys->getPtr(getHL()), 0);
}

void LR35902::RES_1_aHL(){
	res_d8(sys->getPtr(getHL()), 1);
}

void LR35902::RES_2_aHL(){
	res_d8(sys->getPtr(getHL()), 2);
}

void LR35902::RES_3_aHL(){
	res_d8(sys->getPtr(getHL()), 3);
}

void LR35902::RES_4_aHL(){
	res_d8(sys->getPtr(getHL()), 4);
}

void LR35902::RES_5_aHL(){
	res_d8(sys->getPtr(getHL()), 5);
}

void LR35902::RES_6_aHL(){
	res_d8(sys->getPtr(getHL()), 6);
}

void LR35902::RES_7_aHL(){
	res_d8(sys->getPtr(getHL()), 7);
}

// SET 0[1|2|3|4|5|6|7],HL

void LR35902::SET_0_aHL(){
	set_d8(sys->getPtr(getHL()), 0);
}

void LR35902::SET_1_aHL(){
	set_d8(sys->getPtr(getHL()), 1);
}

void LR35902::SET_2_aHL(){
	set_d8(sys->getPtr(getHL()), 2);
}

void LR35902::SET_3_aHL(){
	set_d8(sys->getPtr(getHL()), 3);
}

void LR35902::SET_4_aHL(){
	set_d8(sys->getPtr(getHL()), 4);
}

void LR35902::SET_5_aHL(){
	set_d8(sys->getPtr(getHL()), 5);
}

void LR35902::SET_6_aHL(){
	set_d8(sys->getPtr(getHL()), 6);
}

void LR35902::SET_7_aHL(){
	set_d8(sys->getPtr(getHL()), 7);
}

bool LR35902::initialize(){
	std::ifstream f;
	
	//if(debugMode){ // Read the opcode names
	if(true){
		unsigned int row = 0;
		unsigned int col = 0;
		std::string line;
		f.open("../../assets/opcodes.dat");
		if(!f.good()){
			std::cout << " WARNING! Failed to read file \"opcodes.dat\", disabling debug mode.\n";
			debugMode = false;
		}
		while(true){
			getline(f, line);
			if(f.eof() || !f.good()) break;
			std::vector<std::string> cells;
			if(splitString(line, cells) < 16) continue;
			for(int i = 0; i < 16; i++)
				opcodes[row*16+i] = cells[i];
			row++;
		}
		f.close();
	}
	
	//if(debugMode){ // Read the CB prefix opcodes
	if(true){
		unsigned int row = 0;
		std::string line;
		f.open("../../assets/opcodesCB.dat");
		if(!f.good()){
			std::cout << " WARNING! Failed to read file \"opcodesCB.dat\", disabling debug mode.\n";
			debugMode = false;
		}
		while(true){
			getline(f, line);
			if(f.eof() || !f.good()) break;
			std::vector<std::string> cells;
			if(splitString(line, cells) < 16) continue;
			for(int i = 0; i < 16; i++)
				opcodesCB[row*16+i] = cells[i];
			row++;
		}
		f.close();
	}

	// Set startup values for the CPU registers
	setAF(0x01B0);
	setBC(0x0013);
	setDE(0x00D8);
	setHL(0x014D);
	SP = 0xFFFE;
	PC = 0x0100;
	
	funcPtr[0] = &LR35902::NOP;
	funcPtr[1] = &LR35902::LD_BC_d16;
	funcPtr[2] = &LR35902::LD_aBC_A;
	funcPtr[3] = &LR35902::INC_BC;
	funcPtr[4] = &LR35902::INC_B;
	funcPtr[5] = &LR35902::DEC_B;
	funcPtr[6] = &LR35902::LD_B_d8;
	funcPtr[7] = &LR35902::RLCA;
	funcPtr[8] = &LR35902::LD_a16_SP;
	funcPtr[9] = &LR35902::ADD_HL_BC;
	funcPtr[10] = &LR35902::LD_A_aBC;
	funcPtr[11] = &LR35902::DEC_BC;
	funcPtr[12] = &LR35902::INC_C;
	funcPtr[13] = &LR35902::DEC_C;
	funcPtr[14] = &LR35902::LD_C_d8;
	funcPtr[15] = &LR35902::RRCA;
	funcPtr[16] = &LR35902::STOP_0;
	funcPtr[17] = &LR35902::LD_DE_d16;
	funcPtr[18] = &LR35902::LD_aDE_A;
	funcPtr[19] = &LR35902::INC_DE;
	funcPtr[20] = &LR35902::INC_D;
	funcPtr[21] = &LR35902::DEC_D;
	funcPtr[22] = &LR35902::LD_D_d8;
	funcPtr[23] = &LR35902::RLA;
	funcPtr[24] = &LR35902::JR_r8;
	funcPtr[25] = &LR35902::ADD_HL_DE;
	funcPtr[26] = &LR35902::LD_A_aDE;
	funcPtr[27] = &LR35902::DEC_DE;
	funcPtr[28] = &LR35902::INC_E;
	funcPtr[29] = &LR35902::DEC_E;
	funcPtr[30] = &LR35902::LD_E_d8;
	funcPtr[31] = &LR35902::RRA;
	funcPtr[32] = &LR35902::JR_NZ_r8;
	funcPtr[33] = &LR35902::LD_HL_d16;
	funcPtr[34] = &LR35902::LDI_aHL_A;
	funcPtr[35] = &LR35902::INC_HL;
	funcPtr[36] = &LR35902::INC_H;
	funcPtr[37] = &LR35902::DEC_H;
	funcPtr[38] = &LR35902::LD_H_d8;
	funcPtr[39] = &LR35902::DAA;
	funcPtr[40] = &LR35902::JR_Z_r8;
	funcPtr[41] = &LR35902::ADD_HL_HL;
	funcPtr[42] = &LR35902::LDI_A_aHL;
	funcPtr[43] = &LR35902::DEC_HL;
	funcPtr[44] = &LR35902::INC_L;
	funcPtr[45] = &LR35902::DEC_L;
	funcPtr[46] = &LR35902::LD_L_d8;
	funcPtr[47] = &LR35902::CPL;
	funcPtr[48] = &LR35902::JR_NC_r8;
	funcPtr[49] = &LR35902::LD_SP_d16;
	funcPtr[50] = &LR35902::LDD_aHL_A;
	funcPtr[51] = &LR35902::INC_SP;
	funcPtr[52] = &LR35902::INC_aHL;
	funcPtr[53] = &LR35902::DEC_aHL;
	funcPtr[54] = &LR35902::LD_aHL_d8;
	funcPtr[55] = &LR35902::SCF;
	funcPtr[56] = &LR35902::JR_C_r8;
	funcPtr[57] = &LR35902::ADD_HL_SP;
	funcPtr[58] = &LR35902::LDD_A_aHL;
	funcPtr[59] = &LR35902::DEC_SP;
	funcPtr[60] = &LR35902::INC_A;
	funcPtr[61] = &LR35902::DEC_A;
	funcPtr[62] = &LR35902::LD_A_d8;
	funcPtr[63] = &LR35902::CCF;
	funcPtr[64] = &LR35902::LD_B_B;
	funcPtr[65] = &LR35902::LD_B_C;
	funcPtr[66] = &LR35902::LD_B_D;
	funcPtr[67] = &LR35902::LD_B_E;
	funcPtr[68] = &LR35902::LD_B_H;
	funcPtr[69] = &LR35902::LD_B_L;
	funcPtr[70] = &LR35902::LD_B_aHL;
	funcPtr[71] = &LR35902::LD_B_A;
	funcPtr[72] = &LR35902::LD_C_B;
	funcPtr[73] = &LR35902::LD_C_C;
	funcPtr[74] = &LR35902::LD_C_D;
	funcPtr[75] = &LR35902::LD_C_E;
	funcPtr[76] = &LR35902::LD_C_H;
	funcPtr[77] = &LR35902::LD_C_L;
	funcPtr[78] = &LR35902::LD_C_aHL;
	funcPtr[79] = &LR35902::LD_C_A;
	funcPtr[80] = &LR35902::LD_D_B;
	funcPtr[81] = &LR35902::LD_D_C;
	funcPtr[82] = &LR35902::LD_D_D;
	funcPtr[83] = &LR35902::LD_D_E;
	funcPtr[84] = &LR35902::LD_D_H;
	funcPtr[85] = &LR35902::LD_D_L;
	funcPtr[86] = &LR35902::LD_D_aHL;
	funcPtr[87] = &LR35902::LD_D_A;
	funcPtr[88] = &LR35902::LD_E_B;
	funcPtr[89] = &LR35902::LD_E_C;
	funcPtr[90] = &LR35902::LD_E_D;
	funcPtr[91] = &LR35902::LD_E_E;
	funcPtr[92] = &LR35902::LD_E_H;
	funcPtr[93] = &LR35902::LD_E_L;
	funcPtr[94] = &LR35902::LD_E_aHL;
	funcPtr[95] = &LR35902::LD_E_A;
	funcPtr[96] = &LR35902::LD_H_B;
	funcPtr[97] = &LR35902::LD_H_C;
	funcPtr[98] = &LR35902::LD_H_D;
	funcPtr[99] = &LR35902::LD_H_E;
	funcPtr[100] = &LR35902::LD_H_H;
	funcPtr[101] = &LR35902::LD_H_L;
	funcPtr[102] = &LR35902::LD_H_aHL;
	funcPtr[103] = &LR35902::LD_H_A;
	funcPtr[104] = &LR35902::LD_L_B;
	funcPtr[105] = &LR35902::LD_L_C;
	funcPtr[106] = &LR35902::LD_L_D;
	funcPtr[107] = &LR35902::LD_L_E;
	funcPtr[108] = &LR35902::LD_L_H;
	funcPtr[109] = &LR35902::LD_L_L;
	funcPtr[110] = &LR35902::LD_L_aHL;
	funcPtr[111] = &LR35902::LD_L_A;
	funcPtr[112] = &LR35902::LD_aHL_B;
	funcPtr[113] = &LR35902::LD_aHL_C;
	funcPtr[114] = &LR35902::LD_aHL_D;
	funcPtr[115] = &LR35902::LD_aHL_E;
	funcPtr[116] = &LR35902::LD_aHL_H;
	funcPtr[117] = &LR35902::LD_aHL_L;
	funcPtr[118] = &LR35902::HALT;
	funcPtr[119] = &LR35902::LD_aHL_A;
	funcPtr[120] = &LR35902::LD_A_B;
	funcPtr[121] = &LR35902::LD_A_C;
	funcPtr[122] = &LR35902::LD_A_D;
	funcPtr[123] = &LR35902::LD_A_E;
	funcPtr[124] = &LR35902::LD_A_H;
	funcPtr[125] = &LR35902::LD_A_L;
	funcPtr[126] = &LR35902::LD_A_aHL;
	funcPtr[127] = &LR35902::LD_A_A;
	funcPtr[128] = &LR35902::ADD_A_B;
	funcPtr[129] = &LR35902::ADD_A_C;
	funcPtr[130] = &LR35902::ADD_A_D;
	funcPtr[131] = &LR35902::ADD_A_E;
	funcPtr[132] = &LR35902::ADD_A_H;
	funcPtr[133] = &LR35902::ADD_A_L;
	funcPtr[134] = &LR35902::ADD_A_aHL;
	funcPtr[135] = &LR35902::ADD_A_A;
	funcPtr[136] = &LR35902::ADC_A_B;
	funcPtr[137] = &LR35902::ADC_A_C;
	funcPtr[138] = &LR35902::ADC_A_D;
	funcPtr[139] = &LR35902::ADC_A_E;
	funcPtr[140] = &LR35902::ADC_A_H;
	funcPtr[141] = &LR35902::ADC_A_L;
	funcPtr[142] = &LR35902::ADC_A_aHL;
	funcPtr[143] = &LR35902::ADC_A_A;
	funcPtr[144] = &LR35902::SUB_B;
	funcPtr[145] = &LR35902::SUB_C;
	funcPtr[146] = &LR35902::SUB_D;
	funcPtr[147] = &LR35902::SUB_E;
	funcPtr[148] = &LR35902::SUB_H;
	funcPtr[149] = &LR35902::SUB_L;
	funcPtr[150] = &LR35902::SUB_aHL;
	funcPtr[151] = &LR35902::SUB_A;
	funcPtr[152] = &LR35902::SBC_A_B;
	funcPtr[153] = &LR35902::SBC_A_C;
	funcPtr[154] = &LR35902::SBC_A_D;
	funcPtr[155] = &LR35902::SBC_A_E;
	funcPtr[156] = &LR35902::SBC_A_H;
	funcPtr[157] = &LR35902::SBC_A_L;
	funcPtr[158] = &LR35902::SBC_A_aHL;
	funcPtr[159] = &LR35902::SBC_A_A;
	funcPtr[160] = &LR35902::AND_B;
	funcPtr[161] = &LR35902::AND_C;
	funcPtr[162] = &LR35902::AND_D;
	funcPtr[163] = &LR35902::AND_E;
	funcPtr[164] = &LR35902::AND_H;
	funcPtr[165] = &LR35902::AND_L;
	funcPtr[166] = &LR35902::AND_aHL;
	funcPtr[167] = &LR35902::AND_A;
	funcPtr[168] = &LR35902::XOR_B;
	funcPtr[169] = &LR35902::XOR_C;
	funcPtr[170] = &LR35902::XOR_D;
	funcPtr[171] = &LR35902::XOR_E;
	funcPtr[172] = &LR35902::XOR_H;
	funcPtr[173] = &LR35902::XOR_L;
	funcPtr[174] = &LR35902::XOR_aHL;
	funcPtr[175] = &LR35902::XOR_A;
	funcPtr[176] = &LR35902::OR_B;
	funcPtr[177] = &LR35902::OR_C;
	funcPtr[178] = &LR35902::OR_D;
	funcPtr[179] = &LR35902::OR_E;
	funcPtr[180] = &LR35902::OR_H;
	funcPtr[181] = &LR35902::OR_L;
	funcPtr[182] = &LR35902::OR_aHL;
	funcPtr[183] = &LR35902::OR_A;
	funcPtr[184] = &LR35902::CP_B;
	funcPtr[185] = &LR35902::CP_C;
	funcPtr[186] = &LR35902::CP_D;
	funcPtr[187] = &LR35902::CP_E;
	funcPtr[188] = &LR35902::CP_H;
	funcPtr[189] = &LR35902::CP_L;
	funcPtr[190] = &LR35902::CP_aHL;
	funcPtr[191] = &LR35902::CP_A;
	funcPtr[192] = &LR35902::RET_NZ;
	funcPtr[193] = &LR35902::POP_BC;
	funcPtr[194] = &LR35902::JP_NZ_d16;
	funcPtr[195] = &LR35902::JP_d16;
	funcPtr[196] = &LR35902::CALL_NZ_a16;
	funcPtr[197] = &LR35902::PUSH_BC;
	funcPtr[198] = &LR35902::ADD_A_d8;
	funcPtr[199] = &LR35902::RST_00H;
	funcPtr[200] = &LR35902::RET_Z;
	funcPtr[201] = &LR35902::RET;
	funcPtr[202] = &LR35902::JP_Z_d16;
	funcPtr[203] = 0x0; // PREFIX CB
	funcPtr[204] = &LR35902::CALL_Z_a16;
	funcPtr[205] = &LR35902::CALL_a16;
	funcPtr[206] = &LR35902::ADC_A_d8;
	funcPtr[207] = &LR35902::RST_08H;
	funcPtr[208] = &LR35902::RET_NC;
	funcPtr[209] = &LR35902::POP_DE;
	funcPtr[210] = &LR35902::JP_NC_d16;
	funcPtr[211] = &LR35902::NOP; // no operation
	funcPtr[212] = &LR35902::CALL_NC_a16;
	funcPtr[213] = &LR35902::PUSH_DE;
	funcPtr[214] = &LR35902::SUB_d8;
	funcPtr[215] = &LR35902::RST_10H;
	funcPtr[216] = &LR35902::RET_C;
	funcPtr[217] = &LR35902::RETI;
	funcPtr[218] = &LR35902::JP_C_d16;
	funcPtr[219] = &LR35902::NOP; // no operation
	funcPtr[220] = &LR35902::CALL_C_a16;
	funcPtr[221] = &LR35902::NOP; // no operation
	funcPtr[222] = &LR35902::SBC_A_d8;
	funcPtr[223] = &LR35902::RST_18H;
	funcPtr[224] = &LR35902::LDH_a8_A;
	funcPtr[225] = &LR35902::POP_HL;
	funcPtr[226] = &LR35902::LD_aC_A;
	funcPtr[227] = &LR35902::NOP; // no operation
	funcPtr[228] = &LR35902::NOP; // no operation
	funcPtr[229] = &LR35902::PUSH_HL;
	funcPtr[230] = &LR35902::AND_d8;
	funcPtr[231] = &LR35902::RST_20H;
	funcPtr[232] = &LR35902::ADD_SP_r8;
	funcPtr[233] = &LR35902::JP_aHL;
	funcPtr[234] = &LR35902::LD_a16_A;
	funcPtr[235] = &LR35902::NOP; // no operation
	funcPtr[236] = &LR35902::NOP; // no operation
	funcPtr[237] = &LR35902::NOP; // no operation
	funcPtr[238] = &LR35902::XOR_d8;
	funcPtr[239] = &LR35902::RST_28H;
	funcPtr[240] = &LR35902::LDH_A_a8;
	funcPtr[241] = &LR35902::POP_AF;
	funcPtr[242] = &LR35902::LD_A_aC;
	funcPtr[243] = &LR35902::DI;
	funcPtr[244] = &LR35902::NOP; // no operation
	funcPtr[245] = &LR35902::PUSH_AF;
	funcPtr[246] = &LR35902::OR_d8;
	funcPtr[247] = &LR35902::RST_30H;
	funcPtr[248] = &LR35902::LD_HL_SP_r8;
	funcPtr[249] = &LR35902::LD_SP_HL;
	funcPtr[250] = &LR35902::LD_A_a16;
	funcPtr[251] = &LR35902::EI;
	funcPtr[252] = &LR35902::NOP; // no operation
	funcPtr[253] = &LR35902::NOP; // no operation
	funcPtr[254] = &LR35902::CP_d8;
	funcPtr[255] = &LR35902::RST_38H;

	funcPtrCB[0] = &LR35902::RLC_B;
	funcPtrCB[1] = &LR35902::RLC_C;
	funcPtrCB[2] = &LR35902::RLC_D;
	funcPtrCB[3] = &LR35902::RLC_E;
	funcPtrCB[4] = &LR35902::RLC_H;
	funcPtrCB[5] = &LR35902::RLC_L;
	funcPtrCB[6] = &LR35902::RLC_aHL;
	funcPtrCB[7] = &LR35902::RLC_A;
	funcPtrCB[8] = &LR35902::RRC_B;
	funcPtrCB[9] = &LR35902::RRC_C;
	funcPtrCB[10] = &LR35902::RRC_D;
	funcPtrCB[11] = &LR35902::RRC_E;
	funcPtrCB[12] = &LR35902::RRC_H;
	funcPtrCB[13] = &LR35902::RRC_L;
	funcPtrCB[14] = &LR35902::RRC_aHL;
	funcPtrCB[15] = &LR35902::RRC_A;
	funcPtrCB[16] = &LR35902::RL_B;
	funcPtrCB[17] = &LR35902::RL_C;
	funcPtrCB[18] = &LR35902::RL_D;
	funcPtrCB[19] = &LR35902::RL_E;
	funcPtrCB[20] = &LR35902::RL_H;
	funcPtrCB[21] = &LR35902::RL_L;
	funcPtrCB[22] = &LR35902::RL_aHL;
	funcPtrCB[23] = &LR35902::RL_A;
	funcPtrCB[24] = &LR35902::RR_B;
	funcPtrCB[25] = &LR35902::RR_C;
	funcPtrCB[26] = &LR35902::RR_D;
	funcPtrCB[27] = &LR35902::RR_E;
	funcPtrCB[28] = &LR35902::RR_H;
	funcPtrCB[29] = &LR35902::RR_L;
	funcPtrCB[30] = &LR35902::RR_aHL;
	funcPtrCB[31] = &LR35902::RR_A;
	funcPtrCB[32] = &LR35902::SLA_B;
	funcPtrCB[33] = &LR35902::SLA_C;
	funcPtrCB[34] = &LR35902::SLA_D;
	funcPtrCB[35] = &LR35902::SLA_E;
	funcPtrCB[36] = &LR35902::SLA_H;
	funcPtrCB[37] = &LR35902::SLA_L;
	funcPtrCB[38] = &LR35902::SLA_aHL;
	funcPtrCB[39] = &LR35902::SLA_A;
	funcPtrCB[40] = &LR35902::SRA_B;
	funcPtrCB[41] = &LR35902::SRA_C;
	funcPtrCB[42] = &LR35902::SRA_D;
	funcPtrCB[43] = &LR35902::SRA_E;
	funcPtrCB[44] = &LR35902::SRA_H;
	funcPtrCB[45] = &LR35902::SRA_L;
	funcPtrCB[46] = &LR35902::SRA_aHL;
	funcPtrCB[47] = &LR35902::SRA_A;
	funcPtrCB[48] = &LR35902::SWAP_B;
	funcPtrCB[49] = &LR35902::SWAP_C;
	funcPtrCB[50] = &LR35902::SWAP_D;
	funcPtrCB[51] = &LR35902::SWAP_E;
	funcPtrCB[52] = &LR35902::SWAP_H;
	funcPtrCB[53] = &LR35902::SWAP_L;
	funcPtrCB[54] = &LR35902::SWAP_aHL;
	funcPtrCB[55] = &LR35902::SWAP_A;
	funcPtrCB[56] = &LR35902::SRL_B;
	funcPtrCB[57] = &LR35902::SRL_C;
	funcPtrCB[58] = &LR35902::SRL_D;
	funcPtrCB[59] = &LR35902::SRL_E;
	funcPtrCB[60] = &LR35902::SRL_H;
	funcPtrCB[61] = &LR35902::SRL_L;
	funcPtrCB[62] = &LR35902::SRL_aHL;
	funcPtrCB[63] = &LR35902::SRL_A;
	funcPtrCB[64] = &LR35902::BIT_0_B;
	funcPtrCB[65] = &LR35902::BIT_0_C;
	funcPtrCB[66] = &LR35902::BIT_0_D;
	funcPtrCB[67] = &LR35902::BIT_0_E;
	funcPtrCB[68] = &LR35902::BIT_0_H;
	funcPtrCB[69] = &LR35902::BIT_0_L;
	funcPtrCB[70] = &LR35902::BIT_0_aHL;
	funcPtrCB[71] = &LR35902::BIT_0_A;
	funcPtrCB[72] = &LR35902::BIT_1_B;
	funcPtrCB[73] = &LR35902::BIT_1_C;
	funcPtrCB[74] = &LR35902::BIT_1_D;
	funcPtrCB[75] = &LR35902::BIT_1_E;
	funcPtrCB[76] = &LR35902::BIT_1_H;
	funcPtrCB[77] = &LR35902::BIT_1_L;
	funcPtrCB[78] = &LR35902::BIT_1_aHL;
	funcPtrCB[79] = &LR35902::BIT_1_A;
	funcPtrCB[80] = &LR35902::BIT_2_B;
	funcPtrCB[81] = &LR35902::BIT_2_C;
	funcPtrCB[82] = &LR35902::BIT_2_D;
	funcPtrCB[83] = &LR35902::BIT_2_E;
	funcPtrCB[84] = &LR35902::BIT_2_H;
	funcPtrCB[85] = &LR35902::BIT_2_L;
	funcPtrCB[86] = &LR35902::BIT_2_aHL;
	funcPtrCB[87] = &LR35902::BIT_2_A;
	funcPtrCB[88] = &LR35902::BIT_3_B;
	funcPtrCB[89] = &LR35902::BIT_3_C;
	funcPtrCB[90] = &LR35902::BIT_3_D;
	funcPtrCB[91] = &LR35902::BIT_3_E;
	funcPtrCB[92] = &LR35902::BIT_3_H;
	funcPtrCB[93] = &LR35902::BIT_3_L;
	funcPtrCB[94] = &LR35902::BIT_3_aHL;
	funcPtrCB[95] = &LR35902::BIT_3_A;
	funcPtrCB[96] = &LR35902::BIT_4_B;
	funcPtrCB[97] = &LR35902::BIT_4_C;
	funcPtrCB[98] = &LR35902::BIT_4_D;
	funcPtrCB[99] = &LR35902::BIT_4_E;
	funcPtrCB[100] = &LR35902::BIT_4_H;
	funcPtrCB[101] = &LR35902::BIT_4_L;
	funcPtrCB[102] = &LR35902::BIT_4_aHL;
	funcPtrCB[103] = &LR35902::BIT_4_A;
	funcPtrCB[104] = &LR35902::BIT_5_B;
	funcPtrCB[105] = &LR35902::BIT_5_C;
	funcPtrCB[106] = &LR35902::BIT_5_D;
	funcPtrCB[107] = &LR35902::BIT_5_E;
	funcPtrCB[108] = &LR35902::BIT_5_H;
	funcPtrCB[109] = &LR35902::BIT_5_L;
	funcPtrCB[110] = &LR35902::BIT_5_aHL;
	funcPtrCB[111] = &LR35902::BIT_5_A;
	funcPtrCB[112] = &LR35902::BIT_6_B;
	funcPtrCB[113] = &LR35902::BIT_6_C;
	funcPtrCB[114] = &LR35902::BIT_6_D;
	funcPtrCB[115] = &LR35902::BIT_6_E;
	funcPtrCB[116] = &LR35902::BIT_6_H;
	funcPtrCB[117] = &LR35902::BIT_6_L;
	funcPtrCB[118] = &LR35902::BIT_6_aHL;
	funcPtrCB[119] = &LR35902::BIT_6_A;
	funcPtrCB[120] = &LR35902::BIT_7_B;
	funcPtrCB[121] = &LR35902::BIT_7_C;
	funcPtrCB[122] = &LR35902::BIT_7_D;
	funcPtrCB[123] = &LR35902::BIT_7_E;
	funcPtrCB[124] = &LR35902::BIT_7_H;
	funcPtrCB[125] = &LR35902::BIT_7_L;
	funcPtrCB[126] = &LR35902::BIT_7_aHL;
	funcPtrCB[127] = &LR35902::BIT_7_A;
	funcPtrCB[128] = &LR35902::RES_0_B;
	funcPtrCB[129] = &LR35902::RES_0_C;
	funcPtrCB[130] = &LR35902::RES_0_D;
	funcPtrCB[131] = &LR35902::RES_0_E;
	funcPtrCB[132] = &LR35902::RES_0_H;
	funcPtrCB[133] = &LR35902::RES_0_L;
	funcPtrCB[134] = &LR35902::RES_0_aHL;
	funcPtrCB[135] = &LR35902::RES_0_A;
	funcPtrCB[136] = &LR35902::RES_1_B;
	funcPtrCB[137] = &LR35902::RES_1_C;
	funcPtrCB[138] = &LR35902::RES_1_D;
	funcPtrCB[139] = &LR35902::RES_1_E;
	funcPtrCB[140] = &LR35902::RES_1_H;
	funcPtrCB[141] = &LR35902::RES_1_L;
	funcPtrCB[142] = &LR35902::RES_1_aHL;
	funcPtrCB[143] = &LR35902::RES_1_A;
	funcPtrCB[144] = &LR35902::RES_2_B;
	funcPtrCB[145] = &LR35902::RES_2_C;
	funcPtrCB[146] = &LR35902::RES_2_D;
	funcPtrCB[147] = &LR35902::RES_2_E;
	funcPtrCB[148] = &LR35902::RES_2_H;
	funcPtrCB[149] = &LR35902::RES_2_L;
	funcPtrCB[150] = &LR35902::RES_2_aHL;
	funcPtrCB[151] = &LR35902::RES_2_A;
	funcPtrCB[152] = &LR35902::RES_3_B;
	funcPtrCB[153] = &LR35902::RES_3_C;
	funcPtrCB[154] = &LR35902::RES_3_D;
	funcPtrCB[155] = &LR35902::RES_3_E;
	funcPtrCB[156] = &LR35902::RES_3_H;
	funcPtrCB[157] = &LR35902::RES_3_L;
	funcPtrCB[158] = &LR35902::RES_3_aHL;
	funcPtrCB[159] = &LR35902::RES_3_A;
	funcPtrCB[160] = &LR35902::RES_4_B;
	funcPtrCB[161] = &LR35902::RES_4_C;
	funcPtrCB[162] = &LR35902::RES_4_D;
	funcPtrCB[163] = &LR35902::RES_4_E;
	funcPtrCB[164] = &LR35902::RES_4_H;
	funcPtrCB[165] = &LR35902::RES_4_L;
	funcPtrCB[166] = &LR35902::RES_4_aHL;
	funcPtrCB[167] = &LR35902::RES_4_A;
	funcPtrCB[168] = &LR35902::RES_5_B;
	funcPtrCB[169] = &LR35902::RES_5_C;
	funcPtrCB[170] = &LR35902::RES_5_D;
	funcPtrCB[171] = &LR35902::RES_5_E;
	funcPtrCB[172] = &LR35902::RES_5_H;
	funcPtrCB[173] = &LR35902::RES_5_L;
	funcPtrCB[174] = &LR35902::RES_5_aHL;
	funcPtrCB[175] = &LR35902::RES_5_A;
	funcPtrCB[176] = &LR35902::RES_6_B;
	funcPtrCB[177] = &LR35902::RES_6_C;
	funcPtrCB[178] = &LR35902::RES_6_D;
	funcPtrCB[179] = &LR35902::RES_6_E;
	funcPtrCB[180] = &LR35902::RES_6_H;
	funcPtrCB[181] = &LR35902::RES_6_L;
	funcPtrCB[182] = &LR35902::RES_6_aHL;
	funcPtrCB[183] = &LR35902::RES_6_A;
	funcPtrCB[184] = &LR35902::RES_7_B;
	funcPtrCB[185] = &LR35902::RES_7_C;
	funcPtrCB[186] = &LR35902::RES_7_D;
	funcPtrCB[187] = &LR35902::RES_7_E;
	funcPtrCB[188] = &LR35902::RES_7_H;
	funcPtrCB[189] = &LR35902::RES_7_L;
	funcPtrCB[190] = &LR35902::RES_7_aHL;
	funcPtrCB[191] = &LR35902::RES_7_A;
	funcPtrCB[192] = &LR35902::SET_0_B;
	funcPtrCB[193] = &LR35902::SET_0_C;
	funcPtrCB[194] = &LR35902::SET_0_D;
	funcPtrCB[195] = &LR35902::SET_0_E;
	funcPtrCB[196] = &LR35902::SET_0_H;
	funcPtrCB[197] = &LR35902::SET_0_L;
	funcPtrCB[198] = &LR35902::SET_0_aHL;
	funcPtrCB[199] = &LR35902::SET_0_A;
	funcPtrCB[200] = &LR35902::SET_1_B;
	funcPtrCB[201] = &LR35902::SET_1_C;
	funcPtrCB[202] = &LR35902::SET_1_D;
	funcPtrCB[203] = &LR35902::SET_1_E;
	funcPtrCB[204] = &LR35902::SET_1_H;
	funcPtrCB[205] = &LR35902::SET_1_L;
	funcPtrCB[206] = &LR35902::SET_1_aHL;
	funcPtrCB[207] = &LR35902::SET_1_A;
	funcPtrCB[208] = &LR35902::SET_2_B;
	funcPtrCB[209] = &LR35902::SET_2_C;
	funcPtrCB[210] = &LR35902::SET_2_D;
	funcPtrCB[211] = &LR35902::SET_2_E;
	funcPtrCB[212] = &LR35902::SET_2_H;
	funcPtrCB[213] = &LR35902::SET_2_L;
	funcPtrCB[214] = &LR35902::SET_2_aHL;
	funcPtrCB[215] = &LR35902::SET_2_A;
	funcPtrCB[216] = &LR35902::SET_3_B;
	funcPtrCB[217] = &LR35902::SET_3_C;
	funcPtrCB[218] = &LR35902::SET_3_D;
	funcPtrCB[219] = &LR35902::SET_3_E;
	funcPtrCB[220] = &LR35902::SET_3_H;
	funcPtrCB[221] = &LR35902::SET_3_L;
	funcPtrCB[222] = &LR35902::SET_3_aHL;
	funcPtrCB[223] = &LR35902::SET_3_A;
	funcPtrCB[224] = &LR35902::SET_4_B;
	funcPtrCB[225] = &LR35902::SET_4_C;
	funcPtrCB[226] = &LR35902::SET_4_D;
	funcPtrCB[227] = &LR35902::SET_4_E;
	funcPtrCB[228] = &LR35902::SET_4_H;
	funcPtrCB[229] = &LR35902::SET_4_L;
	funcPtrCB[230] = &LR35902::SET_4_aHL;
	funcPtrCB[231] = &LR35902::SET_4_A;
	funcPtrCB[232] = &LR35902::SET_5_B;
	funcPtrCB[233] = &LR35902::SET_5_C;
	funcPtrCB[234] = &LR35902::SET_5_D;
	funcPtrCB[235] = &LR35902::SET_5_E;
	funcPtrCB[236] = &LR35902::SET_5_H;
	funcPtrCB[237] = &LR35902::SET_5_L;
	funcPtrCB[238] = &LR35902::SET_5_aHL;
	funcPtrCB[239] = &LR35902::SET_5_A;
	funcPtrCB[240] = &LR35902::SET_6_B;
	funcPtrCB[241] = &LR35902::SET_6_C;
	funcPtrCB[242] = &LR35902::SET_6_D;
	funcPtrCB[243] = &LR35902::SET_6_E;
	funcPtrCB[244] = &LR35902::SET_6_H;
	funcPtrCB[245] = &LR35902::SET_6_L;
	funcPtrCB[246] = &LR35902::SET_6_aHL;
	funcPtrCB[247] = &LR35902::SET_6_A;
	funcPtrCB[248] = &LR35902::SET_7_B;
	funcPtrCB[249] = &LR35902::SET_7_C;
	funcPtrCB[250] = &LR35902::SET_7_D;
	funcPtrCB[251] = &LR35902::SET_7_E;
	funcPtrCB[252] = &LR35902::SET_7_H;
	funcPtrCB[253] = &LR35902::SET_7_L;
	funcPtrCB[254] = &LR35902::SET_7_aHL;
	funcPtrCB[255] = &LR35902::SET_7_A;
	
	return true;
}


