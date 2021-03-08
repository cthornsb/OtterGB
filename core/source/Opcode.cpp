#include <stdlib.h>
#include <algorithm>

#include "Support.hpp"
#include "Opcode.hpp"

const unsigned char IMMEDIATE_LEFT_BIT  = 0;
const unsigned char IMMEDIATE_RIGHT_BIT = 1;
const unsigned char ADDRESS_LEFT_BIT    = 2;
const unsigned char ADDRESS_RIGHT_BIT   = 3;
const unsigned char REGISTER_LEFT_BIT   = 4;
const unsigned char REGISTER_RIGHT_BIT  = 5;

const std::vector<std::string> cpuRegisters = { "a", "b", "c", "d", "e", "f", "h", "l", "af", "bc", "de", "hl", "hl-", "hl+", "pc", "sp", "nz", "z", "nc", "c" };

Opcode::Opcode(
	const std::string& mnemonic, 
	const unsigned short& cycles, 
	const unsigned short& bytes, 
	const unsigned short& read, 
	const unsigned short& write
) :
	nType(0x0),
	nCycles(cycles), 
	nBytes(bytes), 
	nReadCycles(read), 
	nWriteCycles(write), 
	sName(toLowercase(mnemonic)),
	sPrefix(),
	sSuffix(),
	sOpname(),
	sOperandsLeft(),
	sOperandsRight(),
	ptr(0x0),
	addrptr(0x0)
{
	const std::vector<std::string> dataTargets = { "d8", "r8", "a8", "d16", "a16" };
	if(nBytes > 1){
		for (auto target = dataTargets.begin(); target != dataTargets.end(); target++) {
			size_t index = sName.find(*target);
			if(index != std::string::npos){
				sPrefix = sName.substr(0, index);
				sSuffix = sName.substr(index+ target->length());
				if(nBytes == 2)
					sSuffix += " ";
				break;
			}
		}
	}
	else{
		sPrefix = sName + "  ";
	}
	size_t index = sName.find(' ');
	if(index != std::string::npos){
		sOpname = sName.substr(0, index);
		std::string temp = sName.substr(index+1);
		index = temp.find(',');
		if(index != std::string::npos){
			sOperandsLeft = temp.substr(0, index);
			sOperandsRight = temp.substr(index+1);
		}
		else{
			sOperandsLeft = temp;
		}
		sOperandsLeft = stripWhitespace(sOperandsLeft);
		sOperandsRight = stripWhitespace(sOperandsRight);
		// Search operands for memory addresses
		if (sOperandsLeft.find('(') != std::string::npos) {
			removeCharacter(sOperandsLeft, '(');
			removeCharacter(sOperandsLeft, ')');
			bitSet(nType, ADDRESS_LEFT_BIT);
		}
		else if (sOperandsRight.find('(') != std::string::npos) {
			removeCharacter(sOperandsRight, '(');
			removeCharacter(sOperandsRight, ')');
			bitSet(nType, ADDRESS_RIGHT_BIT);
		}
		// Search operands for immediate data targets
		if (std::find(dataTargets.begin(), dataTargets.end(), sOperandsLeft) != dataTargets.end()) {
			bitSet(nType, IMMEDIATE_LEFT_BIT);
		}
		if (std::find(dataTargets.begin(), dataTargets.end(), sOperandsRight) != dataTargets.end()) {
			bitSet(nType, IMMEDIATE_RIGHT_BIT);
		}
		// Search operands for cpu registers
		if (std::find(cpuRegisters.begin(), cpuRegisters.end(), sOperandsLeft) != cpuRegisters.end()) {
			bitSet(nType, REGISTER_LEFT_BIT);
		}
		if (std::find(cpuRegisters.begin(), cpuRegisters.end(), sOperandsRight) != cpuRegisters.end()) {
			bitSet(nType, REGISTER_RIGHT_BIT);
		}
	}
}

Opcode::Opcode(
	LR35902* cpu, 
	const std::string &mnemonic, 
	const unsigned short &cycles, 
	const unsigned short &bytes, 
	const unsigned short &read, 
	const unsigned short &write, 
	void (LR35902::*p)()
) :
	Opcode(mnemonic, cycles, bytes, read, write)
{
	ptr = p;
}

bool Opcode::hasImmediateDataLeft() const {
	return bitTest(nType, IMMEDIATE_LEFT_BIT);
}

bool Opcode::hasImmediateDataRight() const {
	return bitTest(nType, IMMEDIATE_RIGHT_BIT);
}

bool Opcode::hasAddressLeft() const {
	return bitTest(nType, ADDRESS_LEFT_BIT);
}

bool Opcode::hasAddressRight() const {
	return bitTest(nType, ADDRESS_RIGHT_BIT);
}

bool Opcode::hasRegisterLeft() const {
	return bitTest(nType, REGISTER_LEFT_BIT);
}

bool Opcode::hasRegisterRight() const {
	return bitTest(nType, REGISTER_RIGHT_BIT);
}

bool Opcode::check(const std::string& op, const unsigned char& type, const std::string& arg1/*=""*/, const std::string& arg2/*=""*/) const {
	if(op == sOpname && type == nType){
		if (bitTest(nType, REGISTER_LEFT_BIT) && bitTest(nType, REGISTER_RIGHT_BIT)) {
			return (sOperandsLeft == arg1 && sOperandsRight == arg2);
		}
		else if (bitTest(nType, REGISTER_LEFT_BIT)) {
			return (sOperandsLeft == arg1);
		}
		else if (bitTest(nType, REGISTER_RIGHT_BIT)) {
			return (sOperandsRight == arg2);
		}
		return true;
	}
	return false;
}

OpcodeData::OpcodeData() : 
	op(0x0), 
	nIndex(0), 
	nData(0), 
	nPC(0), 
	nCycles(0), 
	nExtraCycles(0), 
	nReadCycle(0),
	nWriteCycle(0),
	nExecuteCycle(0), 
	sLabel(),
	cbPrefix(false) 
{ 
}

std::string OpcodeData::getInstruction() const {
	return (getHex(nPC) + " " + getShortInstruction());
}

std::string OpcodeData::getShortInstruction() const {
	std::string retval = op->sPrefix;
	if (!sLabel.empty())
		retval += sLabel;
	else if(op->nBytes == 2)
		retval += getHex(getd8());
	else if(op->nBytes == 3)
		retval += getHex(getd16());
	retval += op->sSuffix;
	return retval;
}

void OpcodeData::set(Opcode *opcodes_, const unsigned char &index_, const unsigned short &pc_){
	op            = &opcodes_[index_];
	nIndex        = index_;
	nPC           = pc_;
	nCycles       = 0;
	nExtraCycles  = 0;
	nReadCycle    = op->nReadCycles;
	nWriteCycle   = op->nWriteCycles;
	nExecuteCycle = op->nCycles;
	sLabel = "";
	cbPrefix = false;
}

void OpcodeData::set(Opcode *op_){
	op            = op_;
	nCycles       = 0;
	nReadCycle    = op->nReadCycles;
	nWriteCycle   = op->nWriteCycles;
	nExecuteCycle = op->nCycles;
	sLabel = "";
	cbPrefix = false;
}

void OpcodeData::setCB(Opcode *opcodes_, const unsigned char &index_, const unsigned short &pc_){
	set(opcodes_, index_, pc_);
	cbPrefix = true;
}

void OpcodeData::setCB(Opcode *op_){
	set(op_);
	cbPrefix = true;
}

void OpcodeData::setImmediateData(const unsigned char& d8) { 
	nData = (d8 & 0x00FF); 
}

void OpcodeData::setImmediateData(const unsigned short& d16) { 
	nData = d16; 
}

void OpcodeData::setImmediateData(const std::string &str){
	if(op->nBytes == 2){ // d8
		setImmediateData(getUserInputUChar(str));
	}
	else if(op->nBytes == 3){ // d16
		setImmediateData(getUserInputUShort(str));
	}
}

OpcodeHandler::OpcodeHandler() :
	aliases(),
	opcodes{
		// Standard opcodes
		//     Mnemonic        C  L  R  Wr
		Opcode("NOP         ", 1, 1, 0, 0),
		Opcode("LD BC,d16   ", 3, 3, 0, 0),
		Opcode("LD (BC),A   ", 2, 1, 0, 2),
		Opcode("INC BC      ", 2, 1, 0, 0),
		Opcode("INC B       ", 1, 1, 0, 0),
		Opcode("DEC B       ", 1, 1, 0, 0),
		Opcode("LD B,d8     ", 2, 2, 0, 0),
		Opcode("RLCA        ", 1, 1, 0, 0),
		Opcode("LD (a16),SP ", 5, 3, 0, 0),
		Opcode("ADD HL,BC   ", 2, 1, 0, 0),
		Opcode("LD A,(BC)   ", 2, 1, 2, 0),
		Opcode("DEC BC      ", 2, 1, 0, 0),
		Opcode("INC C       ", 1, 1, 0, 0),
		Opcode("DEC C       ", 1, 1, 0, 0),
		Opcode("LD C,d8     ", 2, 2, 0, 0),
		Opcode("RRCA        ", 1, 1, 0, 0),
		Opcode("STOP 0      ", 1, 2, 0, 0),
		Opcode("LD DE,d16   ", 3, 3, 0, 0),
		Opcode("LD (DE),A   ", 2, 1, 0, 2),
		Opcode("INC DE      ", 2, 1, 0, 0),
		Opcode("INC D       ", 1, 1, 0, 0),
		Opcode("DEC D       ", 1, 1, 0, 0),
		Opcode("LD D,d8     ", 2, 2, 0, 0),
		Opcode("RLA         ", 1, 1, 0, 0),
		Opcode("JR r8       ", 3, 2, 0, 0),
		Opcode("ADD HL,DE   ", 2, 1, 0, 0),
		Opcode("LD A,(DE)   ", 2, 1, 2, 0),
		Opcode("DEC DE      ", 2, 1, 0, 0),
		Opcode("INC E       ", 1, 1, 0, 0),
		Opcode("DEC E       ", 1, 1, 0, 0),
		Opcode("LD E,d8     ", 2, 2, 0, 0),
		Opcode("RRA         ", 1, 1, 0, 0),
		Opcode("JR NZ,r8    ", 2, 2, 0, 0),
		Opcode("LD HL,d16   ", 3, 3, 0, 0),
		Opcode("LDI (HL),A  ", 2, 1, 0, 2),
		Opcode("INC HL      ", 2, 1, 0, 0),
		Opcode("INC H       ", 1, 1, 0, 0),
		Opcode("DEC H       ", 1, 1, 0, 0),
		Opcode("LD H,d8     ", 2, 2, 0, 0),
		Opcode("DAA         ", 1, 1, 0, 0),
		Opcode("JR Z,r8     ", 2, 2, 0, 0),
		Opcode("ADD HL,HL   ", 2, 1, 0, 0),
		Opcode("LDI A,(HL)  ", 2, 1, 2, 0),
		Opcode("DEC HL      ", 2, 1, 0, 0),
		Opcode("INC L       ", 1, 1, 0, 0),
		Opcode("DEC L       ", 1, 1, 0, 0),
		Opcode("LD L,d8     ", 2, 2, 0, 0),
		Opcode("CPL         ", 1, 1, 0, 0),
		Opcode("JR NC,r8    ", 2, 2, 0, 0),
		Opcode("LD SP,d16   ", 3, 3, 0, 0),
		Opcode("LDD (HL),A  ", 2, 1, 0, 2),
		Opcode("INC SP      ", 2, 1, 0, 0),
		Opcode("INC (HL)    ", 3, 1, 2, 3),
		Opcode("DEC (HL)    ", 3, 1, 2, 3),
		Opcode("LD (HL),d8  ", 3, 2, 0, 3),
		Opcode("SCF         ", 1, 1, 0, 0),
		Opcode("JR C,r8     ", 2, 2, 0, 0),
		Opcode("ADD HL,SP   ", 2, 1, 0, 0),
		Opcode("LDD A,(HL)  ", 2, 1, 2, 0),
		Opcode("DEC SP      ", 2, 1, 0, 0),
		Opcode("INC A       ", 1, 1, 0, 0),
		Opcode("DEC A       ", 1, 1, 0, 0),
		Opcode("LD A,d8     ", 2, 2, 0, 0),
		Opcode("CCF         ", 1, 1, 0, 0),
		Opcode("LD B,B      ", 1, 1, 0, 0),
		Opcode("LD B,C      ", 1, 1, 0, 0),
		Opcode("LD B,D      ", 1, 1, 0, 0),
		Opcode("LD B,E      ", 1, 1, 0, 0),
		Opcode("LD B,H      ", 1, 1, 0, 0),
		Opcode("LD B,L      ", 1, 1, 0, 0),
		Opcode("LD B,(HL)   ", 2, 1, 2, 0),
		Opcode("LD B,A      ", 1, 1, 0, 0),
		Opcode("LD C,B      ", 1, 1, 0, 0),
		Opcode("LD C,C      ", 1, 1, 0, 0),
		Opcode("LD C,D      ", 1, 1, 0, 0),
		Opcode("LD C,E      ", 1, 1, 0, 0),
		Opcode("LD C,H      ", 1, 1, 0, 0),
		Opcode("LD C,L      ", 1, 1, 0, 0),
		Opcode("LD C,(HL)   ", 2, 1, 2, 0),
		Opcode("LD C,A      ", 1, 1, 0, 0),
		Opcode("LD D,B      ", 1, 1, 0, 0),
		Opcode("LD D,C      ", 1, 1, 0, 0),
		Opcode("LD D,D      ", 1, 1, 0, 0),
		Opcode("LD D,E      ", 1, 1, 0, 0),
		Opcode("LD D,H      ", 1, 1, 0, 0),
		Opcode("LD D,L      ", 1, 1, 0, 0),
		Opcode("LD D,(HL)   ", 2, 1, 2, 0),
		Opcode("LD D,A      ", 1, 1, 0, 0),
		Opcode("LD E,B      ", 1, 1, 0, 0),
		Opcode("LD E,C      ", 1, 1, 0, 0),
		Opcode("LD E,D      ", 1, 1, 0, 0),
		Opcode("LD E,E      ", 1, 1, 0, 0),
		Opcode("LD E,H      ", 1, 1, 0, 0),
		Opcode("LD E,L      ", 1, 1, 0, 0),
		Opcode("LD E,(HL)   ", 2, 1, 2, 0),
		Opcode("LD E,A      ", 1, 1, 0, 0),
		Opcode("LD H,B      ", 1, 1, 0, 0),
		Opcode("LD H,C      ", 1, 1, 0, 0),
		Opcode("LD H,D      ", 1, 1, 0, 0),
		Opcode("LD H,E      ", 1, 1, 0, 0),
		Opcode("LD H,H      ", 1, 1, 0, 0),
		Opcode("LD H,L      ", 1, 1, 0, 0),
		Opcode("LD H,(HL)   ", 2, 1, 2, 0),
		Opcode("LD H,A      ", 1, 1, 0, 0),
		Opcode("LD L,B      ", 1, 1, 0, 0),
		Opcode("LD L,C      ", 1, 1, 0, 0),
		Opcode("LD L,D      ", 1, 1, 0, 0),
		Opcode("LD L,E      ", 1, 1, 0, 0),
		Opcode("LD L,H      ", 1, 1, 0, 0),
		Opcode("LD L,L      ", 1, 1, 0, 0),
		Opcode("LD L,(HL)   ", 2, 1, 2, 0),
		Opcode("LD L,A      ", 1, 1, 0, 0),
		Opcode("LD (HL),B   ", 2, 1, 0, 2),
		Opcode("LD (HL),C   ", 2, 1, 0, 2),
		Opcode("LD (HL),D   ", 2, 1, 0, 2),
		Opcode("LD (HL),E   ", 2, 1, 0, 2),
		Opcode("LD (HL),H   ", 2, 1, 0, 2),
		Opcode("LD (HL),L   ", 2, 1, 0, 2),
		Opcode("HALT        ", 1, 1, 0, 0),
		Opcode("LD (HL),A   ", 2, 1, 0, 2),
		Opcode("LD A,B      ", 1, 1, 0, 0),
		Opcode("LD A,C      ", 1, 1, 0, 0),
		Opcode("LD A,D      ", 1, 1, 0, 0),
		Opcode("LD A,E      ", 1, 1, 0, 0),
		Opcode("LD A,H      ", 1, 1, 0, 0),
		Opcode("LD A,L      ", 1, 1, 0, 0),
		Opcode("LD A,(HL)   ", 2, 1, 2, 0),
		Opcode("LD A,A      ", 1, 1, 0, 0),
		Opcode("ADD A,B     ", 1, 1, 0, 0),
		Opcode("ADD A,C     ", 1, 1, 0, 0),
		Opcode("ADD A,D     ", 1, 1, 0, 0),
		Opcode("ADD A,E     ", 1, 1, 0, 0),
		Opcode("ADD A,H     ", 1, 1, 0, 0),
		Opcode("ADD A,L     ", 1, 1, 0, 0),
		Opcode("ADD A,(HL)  ", 2, 1, 2, 0),
		Opcode("ADD A,A     ", 1, 1, 0, 0),
		Opcode("ADC A,B     ", 1, 1, 0, 0),
		Opcode("ADC A,C     ", 1, 1, 0, 0),
		Opcode("ADC A,D     ", 1, 1, 0, 0),
		Opcode("ADC A,E     ", 1, 1, 0, 0),
		Opcode("ADC A,H     ", 1, 1, 0, 0),
		Opcode("ADC A,L     ", 1, 1, 0, 0),
		Opcode("ADC A,(HL)  ", 2, 1, 2, 0),
		Opcode("ADC A,A     ", 1, 1, 0, 0),
		Opcode("SUB B       ", 1, 1, 0, 0),
		Opcode("SUB C       ", 1, 1, 0, 0),
		Opcode("SUB D       ", 1, 1, 0, 0),
		Opcode("SUB E       ", 1, 1, 0, 0),
		Opcode("SUB H       ", 1, 1, 0, 0),
		Opcode("SUB L       ", 1, 1, 0, 0),
		Opcode("SUB (HL)    ", 2, 1, 2, 0),
		Opcode("SUB A       ", 1, 1, 0, 0),
		Opcode("SBC A,B     ", 1, 1, 0, 0),
		Opcode("SBC A,C     ", 1, 1, 0, 0),
		Opcode("SBC A,D     ", 1, 1, 0, 0),
		Opcode("SBC A,E     ", 1, 1, 0, 0),
		Opcode("SBC A,H     ", 1, 1, 0, 0),
		Opcode("SBC A,L     ", 1, 1, 0, 0),
		Opcode("SBC A,(HL)  ", 2, 1, 2, 0),
		Opcode("SBC A,A     ", 1, 1, 0, 0),
		Opcode("AND B       ", 1, 1, 0, 0),
		Opcode("AND C       ", 1, 1, 0, 0),
		Opcode("AND D       ", 1, 1, 0, 0),
		Opcode("AND E       ", 1, 1, 0, 0),
		Opcode("AND H       ", 1, 1, 0, 0),
		Opcode("AND L       ", 1, 1, 0, 0),
		Opcode("AND (HL)    ", 2, 1, 2, 0),
		Opcode("AND A       ", 1, 1, 0, 0),
		Opcode("XOR B       ", 1, 1, 0, 0),
		Opcode("XOR C       ", 1, 1, 0, 0),
		Opcode("XOR D       ", 1, 1, 0, 0),
		Opcode("XOR E       ", 1, 1, 0, 0),
		Opcode("XOR H       ", 1, 1, 0, 0),
		Opcode("XOR L       ", 1, 1, 0, 0),
		Opcode("XOR (HL)    ", 2, 1, 2, 0),
		Opcode("XOR A       ", 1, 1, 0, 0),
		Opcode("OR B        ", 1, 1, 0, 0),
		Opcode("OR C        ", 1, 1, 0, 0),
		Opcode("OR D        ", 1, 1, 0, 0),
		Opcode("OR E        ", 1, 1, 0, 0),
		Opcode("OR H        ", 1, 1, 0, 0),
		Opcode("OR L        ", 1, 1, 0, 0),
		Opcode("OR (HL)     ", 2, 1, 2, 0),
		Opcode("OR A        ", 1, 1, 0, 0),
		Opcode("CP B        ", 1, 1, 0, 0),
		Opcode("CP C        ", 1, 1, 0, 0),
		Opcode("CP D        ", 1, 1, 0, 0),
		Opcode("CP E        ", 1, 1, 0, 0),
		Opcode("CP H        ", 1, 1, 0, 0),
		Opcode("CP L        ", 1, 1, 0, 0),
		Opcode("CP (HL)     ", 2, 1, 2, 0),
		Opcode("CP A        ", 1, 1, 0, 0),
		Opcode("RET NZ      ", 2, 1, 0, 0),
		Opcode("POP BC      ", 3, 1, 0, 0),
		Opcode("JP NZ,a16   ", 3, 3, 0, 0),
		Opcode("JP a16      ", 4, 3, 0, 0),
		Opcode("CALL NZ,a16 ", 3, 3, 0, 0),
		Opcode("PUSH BC     ", 4, 1, 0, 0),
		Opcode("ADD A,d8    ", 2, 2, 0, 0),
		Opcode("RST 00H     ", 4, 1, 0, 0),
		Opcode("RET Z       ", 2, 1, 0, 0),
		Opcode("RET         ", 4, 1, 0, 0),
		Opcode("JP Z,a16    ", 3, 3, 0, 0),
		Opcode("PREFIX CB   ", 0, 1, 0, 0),
		Opcode("CALL Z,a16  ", 3, 3, 0, 0),
		Opcode("CALL a16    ", 6, 3, 0, 0),
		Opcode("ADC A,d8    ", 2, 2, 0, 0),
		Opcode("RST 08H     ", 4, 1, 0, 0),
		Opcode("RET NC      ", 2, 1, 0, 0),
		Opcode("POP DE      ", 3, 1, 0, 0),
		Opcode("JP NC,a16   ", 3, 3, 0, 0),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("CALL NC,a16 ", 3, 3, 0, 0),
		Opcode("PUSH DE     ", 4, 1, 0, 0),
		Opcode("SUB d8      ", 2, 2, 0, 0),
		Opcode("RST 10H     ", 4, 1, 0, 0),
		Opcode("RET C       ", 2, 1, 0, 0),
		Opcode("RETI        ", 4, 1, 0, 0),
		Opcode("JP C,a16    ", 3, 3, 0, 0),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("CALL C,a16  ", 3, 3, 0, 0),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("SBC A,d8    ", 2, 2, 0, 0),
		Opcode("RST 18H     ", 4, 1, 0, 0),
		Opcode("LDH (a8),A  ", 3, 2, 0, 3),
		Opcode("POP HL      ", 3, 1, 0, 0),
		Opcode("LD (C),A    ", 2, 1, 0, 2),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("PUSH HL     ", 4, 1, 0, 0),
		Opcode("AND d8      ", 2, 2, 0, 0),
		Opcode("RST 20H     ", 4, 1, 0, 0),
		Opcode("ADD SP,r8   ", 4, 2, 0, 0),
		Opcode("JP (HL)     ", 1, 1, 0, 0),
		Opcode("LD (a16),A  ", 4, 3, 0, 4),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("XOR d8      ", 2, 2, 0, 0),
		Opcode("RST 28H     ", 4, 1, 0, 0),
		Opcode("LDH A,(a8)  ", 3, 2, 3, 0),
		Opcode("POP AF      ", 3, 1, 0, 0),
		Opcode("LD A,(C)    ", 2, 1, 2, 0),
		Opcode("DI          ", 1, 1, 0, 0),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("PUSH AF     ", 4, 1, 0, 0),
		Opcode("OR d8       ", 2, 2, 0, 0),
		Opcode("RST 30H     ", 4, 1, 0, 0),
		Opcode("LD HL,SP+r8 ", 3, 2, 0, 0),
		Opcode("LD SP,HL    ", 2, 1, 0, 0),
		Opcode("LD A,(a16)  ", 4, 3, 4, 0),
		Opcode("EI          ", 1, 1, 0, 0),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("            ", 0, 1, 0, 0),
		Opcode("CP d8       ", 2, 2, 0, 0),
		Opcode("RST 38H     ", 4, 1, 0, 0)
	},
	opcodesCB{
		// CB-prefix opcodes
		//     Mnemonic        C  L  R  Wr	
		Opcode("RLC B       ", 2, 1, 0, 0),
		Opcode("RLC C       ", 2, 1, 0, 0),
		Opcode("RLC D       ", 2, 1, 0, 0),
		Opcode("RLC E       ", 2, 1, 0, 0),
		Opcode("RLC H       ", 2, 1, 0, 0),
		Opcode("RLC L       ", 2, 1, 0, 0),
		Opcode("RLC (HL)    ", 4, 1, 3, 4),
		Opcode("RLC A       ", 2, 1, 0, 0),
		Opcode("RRC B       ", 2, 1, 0, 0),
		Opcode("RRC C       ", 2, 1, 0, 0),
		Opcode("RRC D       ", 2, 1, 0, 0),
		Opcode("RRC E       ", 2, 1, 0, 0),
		Opcode("RRC H       ", 2, 1, 0, 0),
		Opcode("RRC L       ", 2, 1, 0, 0),
		Opcode("RRC (HL)    ", 4, 1, 3, 4),
		Opcode("RRC A       ", 2, 1, 0, 0),
		Opcode("RL B        ", 2, 1, 0, 0),
		Opcode("RL C        ", 2, 1, 0, 0),
		Opcode("RL D        ", 2, 1, 0, 0),
		Opcode("RL E        ", 2, 1, 0, 0),
		Opcode("RL H        ", 2, 1, 0, 0),
		Opcode("RL L        ", 2, 1, 0, 0),
		Opcode("RL (HL)     ", 4, 1, 3, 4),
		Opcode("RL A        ", 2, 1, 0, 0),
		Opcode("RR B        ", 2, 1, 0, 0),
		Opcode("RR C        ", 2, 1, 0, 0),
		Opcode("RR D        ", 2, 1, 0, 0),
		Opcode("RR E        ", 2, 1, 0, 0),
		Opcode("RR H        ", 2, 1, 0, 0),
		Opcode("RR L        ", 2, 1, 0, 0),
		Opcode("RR (HL)     ", 4, 1, 3, 4),
		Opcode("RR A        ", 2, 1, 0, 0),
		Opcode("SLA B       ", 2, 1, 0, 0),
		Opcode("SLA C       ", 2, 1, 0, 0),
		Opcode("SLA D       ", 2, 1, 0, 0),
		Opcode("SLA E       ", 2, 1, 0, 0),
		Opcode("SLA H       ", 2, 1, 0, 0),
		Opcode("SLA L       ", 2, 1, 0, 0),
		Opcode("SLA (HL)    ", 4, 1, 3, 4),
		Opcode("SLA A       ", 2, 1, 0, 0),
		Opcode("SRA B       ", 2, 1, 0, 0),
		Opcode("SRA C       ", 2, 1, 0, 0),
		Opcode("SRA D       ", 2, 1, 0, 0),
		Opcode("SRA E       ", 2, 1, 0, 0),
		Opcode("SRA H       ", 2, 1, 0, 0),
		Opcode("SRA L       ", 2, 1, 0, 0),
		Opcode("SRA (HL)    ", 4, 1, 3, 4),
		Opcode("SRA A       ", 2, 1, 0, 0),
		Opcode("SWAP B      ", 2, 1, 0, 0),
		Opcode("SWAP C      ", 2, 1, 0, 0),
		Opcode("SWAP D      ", 2, 1, 0, 0),
		Opcode("SWAP E      ", 2, 1, 0, 0),
		Opcode("SWAP H      ", 2, 1, 0, 0),
		Opcode("SWAP L      ", 2, 1, 0, 0),
		Opcode("SWAP (HL)   ", 4, 1, 3, 4),
		Opcode("SWAP A      ", 2, 1, 0, 0),
		Opcode("SRL B       ", 2, 1, 0, 0),
		Opcode("SRL C       ", 2, 1, 0, 0),
		Opcode("SRL D       ", 2, 1, 0, 0),
		Opcode("SRL E       ", 2, 1, 0, 0),
		Opcode("SRL H       ", 2, 1, 0, 0),
		Opcode("SRL L       ", 2, 1, 0, 0),
		Opcode("SRL (HL)    ", 4, 1, 3, 4),
		Opcode("SRL A       ", 2, 1, 0, 0),
		Opcode("BIT 0,B     ", 2, 1, 0, 0),
		Opcode("BIT 0,C     ", 2, 1, 0, 0),
		Opcode("BIT 0,D     ", 2, 1, 0, 0),
		Opcode("BIT 0,E     ", 2, 1, 0, 0),
		Opcode("BIT 0,H     ", 2, 1, 0, 0),
		Opcode("BIT 0,L     ", 2, 1, 0, 0),
		Opcode("BIT 0,(HL)  ", 3, 1, 3, 0),
		Opcode("BIT 0,A     ", 2, 1, 0, 0),
		Opcode("BIT 1,B     ", 2, 1, 0, 0),
		Opcode("BIT 1,C     ", 2, 1, 0, 0),
		Opcode("BIT 1,D     ", 2, 1, 0, 0),
		Opcode("BIT 1,E     ", 2, 1, 0, 0),
		Opcode("BIT 1,H     ", 2, 1, 0, 0),
		Opcode("BIT 1,L     ", 2, 1, 0, 0),
		Opcode("BIT 1,(HL)  ", 3, 1, 3, 0),
		Opcode("BIT 1,A     ", 2, 1, 0, 0),
		Opcode("BIT 2,B     ", 2, 1, 0, 0),
		Opcode("BIT 2,C     ", 2, 1, 0, 0),
		Opcode("BIT 2,D     ", 2, 1, 0, 0),
		Opcode("BIT 2,E     ", 2, 1, 0, 0),
		Opcode("BIT 2,H     ", 2, 1, 0, 0),
		Opcode("BIT 2,L     ", 2, 1, 0, 0),
		Opcode("BIT 2,(HL)  ", 3, 1, 3, 0),
		Opcode("BIT 2,A     ", 2, 1, 0, 0),
		Opcode("BIT 3,B     ", 2, 1, 0, 0),
		Opcode("BIT 3,C     ", 2, 1, 0, 0),
		Opcode("BIT 3,D     ", 2, 1, 0, 0),
		Opcode("BIT 3,E     ", 2, 1, 0, 0),
		Opcode("BIT 3,H     ", 2, 1, 0, 0),
		Opcode("BIT 3,L     ", 2, 1, 0, 0),
		Opcode("BIT 3,(HL)  ", 3, 1, 3, 0),
		Opcode("BIT 3,A     ", 2, 1, 0, 0),
		Opcode("BIT 4,B     ", 2, 1, 0, 0),
		Opcode("BIT 4,C     ", 2, 1, 0, 0),
		Opcode("BIT 4,D     ", 2, 1, 0, 0),
		Opcode("BIT 4,E     ", 2, 1, 0, 0),
		Opcode("BIT 4,H     ", 2, 1, 0, 0),
		Opcode("BIT 4,L     ", 2, 1, 0, 0),
		Opcode("BIT 4,(HL)  ", 3, 1, 3, 0),
		Opcode("BIT 4,A     ", 2, 1, 0, 0),
		Opcode("BIT 5,B     ", 2, 1, 0, 0),
		Opcode("BIT 5,C     ", 2, 1, 0, 0),
		Opcode("BIT 5,D     ", 2, 1, 0, 0),
		Opcode("BIT 5,E     ", 2, 1, 0, 0),
		Opcode("BIT 5,H     ", 2, 1, 0, 0),
		Opcode("BIT 5,L     ", 2, 1, 0, 0),
		Opcode("BIT 5,(HL)  ", 3, 1, 3, 0),
		Opcode("BIT 5,A     ", 2, 1, 0, 0),
		Opcode("BIT 6,B     ", 2, 1, 0, 0),
		Opcode("BIT 6,C     ", 2, 1, 0, 0),
		Opcode("BIT 6,D     ", 2, 1, 0, 0),
		Opcode("BIT 6,E     ", 2, 1, 0, 0),
		Opcode("BIT 6,H     ", 2, 1, 0, 0),
		Opcode("BIT 6,L     ", 2, 1, 0, 0),
		Opcode("BIT 6,(HL)  ", 3, 1, 3, 0),
		Opcode("BIT 6,A     ", 2, 1, 0, 0),
		Opcode("BIT 7,B     ", 2, 1, 0, 0),
		Opcode("BIT 7,C     ", 2, 1, 0, 0),
		Opcode("BIT 7,D     ", 2, 1, 0, 0),
		Opcode("BIT 7,E     ", 2, 1, 0, 0),
		Opcode("BIT 7,H     ", 2, 1, 0, 0),
		Opcode("BIT 7,L     ", 2, 1, 0, 0),
		Opcode("BIT 7,(HL)  ", 3, 1, 3, 0),
		Opcode("BIT 7,A     ", 2, 1, 0, 0),
		Opcode("RES 0,B     ", 2, 1, 0, 0),
		Opcode("RES 0,C     ", 2, 1, 0, 0),
		Opcode("RES 0,D     ", 2, 1, 0, 0),
		Opcode("RES 0,E     ", 2, 1, 0, 0),
		Opcode("RES 0,H     ", 2, 1, 0, 0),
		Opcode("RES 0,L     ", 2, 1, 0, 0),
		Opcode("RES 0,(HL)  ", 4, 1, 3, 4),
		Opcode("RES 0,A     ", 2, 1, 0, 0),
		Opcode("RES 1,B     ", 2, 1, 0, 0),
		Opcode("RES 1,C     ", 2, 1, 0, 0),
		Opcode("RES 1,D     ", 2, 1, 0, 0),
		Opcode("RES 1,E     ", 2, 1, 0, 0),
		Opcode("RES 1,H     ", 2, 1, 0, 0),
		Opcode("RES 1,L     ", 2, 1, 0, 0),
		Opcode("RES 1,(HL)  ", 4, 1, 3, 4),
		Opcode("RES 1,A     ", 2, 1, 0, 0),
		Opcode("RES 2,B     ", 2, 1, 0, 0),
		Opcode("RES 2,C     ", 2, 1, 0, 0),
		Opcode("RES 2,D     ", 2, 1, 0, 0),
		Opcode("RES 2,E     ", 2, 1, 0, 0),
		Opcode("RES 2,H     ", 2, 1, 0, 0),
		Opcode("RES 2,L     ", 2, 1, 0, 0),
		Opcode("RES 2,(HL)  ", 4, 1, 3, 4),
		Opcode("RES 2,A     ", 2, 1, 0, 0),
		Opcode("RES 3,B     ", 2, 1, 0, 0),
		Opcode("RES 3,C     ", 2, 1, 0, 0),
		Opcode("RES 3,D     ", 2, 1, 0, 0),
		Opcode("RES 3,E     ", 2, 1, 0, 0),
		Opcode("RES 3,H     ", 2, 1, 0, 0),
		Opcode("RES 3,L     ", 2, 1, 0, 0),
		Opcode("RES 3,(HL)  ", 4, 1, 3, 4),
		Opcode("RES 3,A     ", 2, 1, 0, 0),
		Opcode("RES 4,B     ", 2, 1, 0, 0),
		Opcode("RES 4,C     ", 2, 1, 0, 0),
		Opcode("RES 4,D     ", 2, 1, 0, 0),
		Opcode("RES 4,E     ", 2, 1, 0, 0),
		Opcode("RES 4,H     ", 2, 1, 0, 0),
		Opcode("RES 4,L     ", 2, 1, 0, 0),
		Opcode("RES 4,(HL)  ", 4, 1, 3, 4),
		Opcode("RES 4,A     ", 2, 1, 0, 0),
		Opcode("RES 5,B     ", 2, 1, 0, 0),
		Opcode("RES 5,C     ", 2, 1, 0, 0),
		Opcode("RES 5,D     ", 2, 1, 0, 0),
		Opcode("RES 5,E     ", 2, 1, 0, 0),
		Opcode("RES 5,H     ", 2, 1, 0, 0),
		Opcode("RES 5,L     ", 2, 1, 0, 0),
		Opcode("RES 5,(HL)  ", 4, 1, 3, 4),
		Opcode("RES 5,A     ", 2, 1, 0, 0),
		Opcode("RES 6,B     ", 2, 1, 0, 0),
		Opcode("RES 6,C     ", 2, 1, 0, 0),
		Opcode("RES 6,D     ", 2, 1, 0, 0),
		Opcode("RES 6,E     ", 2, 1, 0, 0),
		Opcode("RES 6,H     ", 2, 1, 0, 0),
		Opcode("RES 6,L     ", 2, 1, 0, 0),
		Opcode("RES 6,(HL)  ", 4, 1, 3, 4),
		Opcode("RES 6,A     ", 2, 1, 0, 0),
		Opcode("RES 7,B     ", 2, 1, 0, 0),
		Opcode("RES 7,C     ", 2, 1, 0, 0),
		Opcode("RES 7,D     ", 2, 1, 0, 0),
		Opcode("RES 7,E     ", 2, 1, 0, 0),
		Opcode("RES 7,H     ", 2, 1, 0, 0),
		Opcode("RES 7,L     ", 2, 1, 0, 0),
		Opcode("RES 7,(HL)  ", 4, 1, 3, 4),
		Opcode("RES 7,A     ", 2, 1, 0, 0),
		Opcode("SET 0,B     ", 2, 1, 0, 0),
		Opcode("SET 0,C     ", 2, 1, 0, 0),
		Opcode("SET 0,D     ", 2, 1, 0, 0),
		Opcode("SET 0,E     ", 2, 1, 0, 0),
		Opcode("SET 0,H     ", 2, 1, 0, 0),
		Opcode("SET 0,L     ", 2, 1, 0, 0),
		Opcode("SET 0,(HL)  ", 4, 1, 3, 4),
		Opcode("SET 0,A     ", 2, 1, 0, 0),
		Opcode("SET 1,B     ", 2, 1, 0, 0),
		Opcode("SET 1,C     ", 2, 1, 0, 0),
		Opcode("SET 1,D     ", 2, 1, 0, 0),
		Opcode("SET 1,E     ", 2, 1, 0, 0),
		Opcode("SET 1,H     ", 2, 1, 0, 0),
		Opcode("SET 1,L     ", 2, 1, 0, 0),
		Opcode("SET 1,(HL)  ", 4, 1, 3, 4),
		Opcode("SET 1,A     ", 2, 1, 0, 0),
		Opcode("SET 2,B     ", 2, 1, 0, 0),
		Opcode("SET 2,C     ", 2, 1, 0, 0),
		Opcode("SET 2,D     ", 2, 1, 0, 0),
		Opcode("SET 2,E     ", 2, 1, 0, 0),
		Opcode("SET 2,H     ", 2, 1, 0, 0),
		Opcode("SET 2,L     ", 2, 1, 0, 0),
		Opcode("SET 2,(HL)  ", 4, 1, 3, 4),
		Opcode("SET 2,A     ", 2, 1, 0, 0),
		Opcode("SET 3,B     ", 2, 1, 0, 0),
		Opcode("SET 3,C     ", 2, 1, 0, 0),
		Opcode("SET 3,D     ", 2, 1, 0, 0),
		Opcode("SET 3,E     ", 2, 1, 0, 0),
		Opcode("SET 3,H     ", 2, 1, 0, 0),
		Opcode("SET 3,L     ", 2, 1, 0, 0),
		Opcode("SET 3,(HL)  ", 4, 1, 3, 4),
		Opcode("SET 3,A     ", 2, 1, 0, 0),
		Opcode("SET 4,B     ", 2, 1, 0, 0),
		Opcode("SET 4,C     ", 2, 1, 0, 0),
		Opcode("SET 4,D     ", 2, 1, 0, 0),
		Opcode("SET 4,E     ", 2, 1, 0, 0),
		Opcode("SET 4,H     ", 2, 1, 0, 0),
		Opcode("SET 4,L     ", 2, 1, 0, 0),
		Opcode("SET 4,(HL)  ", 4, 1, 3, 4),
		Opcode("SET 4,A     ", 2, 1, 0, 0),
		Opcode("SET 5,B     ", 2, 1, 0, 0),
		Opcode("SET 5,C     ", 2, 1, 0, 0),
		Opcode("SET 5,D     ", 2, 1, 0, 0),
		Opcode("SET 5,E     ", 2, 1, 0, 0),
		Opcode("SET 5,H     ", 2, 1, 0, 0),
		Opcode("SET 5,L     ", 2, 1, 0, 0),
		Opcode("SET 5,(HL)  ", 4, 1, 3, 4),
		Opcode("SET 5,A     ", 2, 1, 0, 0),
		Opcode("SET 6,B     ", 2, 1, 0, 0),
		Opcode("SET 6,C     ", 2, 1, 0, 0),
		Opcode("SET 6,D     ", 2, 1, 0, 0),
		Opcode("SET 6,E     ", 2, 1, 0, 0),
		Opcode("SET 6,H     ", 2, 1, 0, 0),
		Opcode("SET 6,L     ", 2, 1, 0, 0),
		Opcode("SET 6,(HL)  ", 4, 1, 3, 4),
		Opcode("SET 6,A     ", 2, 1, 0, 0),
		Opcode("SET 7,B     ", 2, 1, 0, 0),
		Opcode("SET 7,C     ", 2, 1, 0, 0),
		Opcode("SET 7,D     ", 2, 1, 0, 0),
		Opcode("SET 7,E     ", 2, 1, 0, 0),
		Opcode("SET 7,H     ", 2, 1, 0, 0),
		Opcode("SET 7,L     ", 2, 1, 0, 0),
		Opcode("SET 7,(HL)  ", 4, 1, 3, 4),
		Opcode("SET 7,A     ", 2, 1, 0, 0)
	}
{
	// Add opcode aliases
	aliases.push_back(Opcode("LD (HL+),A  ", 2, 1, 0, 2));
	aliases.push_back(Opcode("LD A,(HL+)  ", 2, 1, 2, 0));	
	aliases.push_back(Opcode("LD (HL-),A  ", 2, 1, 0, 2));
	aliases.push_back(Opcode("LD A,(HL-)  ", 2, 1, 2, 0));
	aliases.push_back(Opcode("ADD d8      ", 2, 2, 0, 0));
	aliases.push_back(Opcode("ADC d8      ", 2, 2, 0, 0));
}

bool OpcodeHandler::findOpcode(const std::string& mnemonic, OpcodeData& data) {
	std::vector<std::string> args;
	unsigned int nArgs = splitString(mnemonic, args, ' ');
	if (!nArgs)
		return 0x0;
	std::string operand1, operand2;
	unsigned short d16 = 0; // Immediate data
	std::string label; // Variable label
	unsigned char type = 0x0;
	if (nArgs >= 2) {
		operand1 = args.at(1);
		size_t index = operand1.find(',');
		if (index != std::string::npos) { // Operands seperated by a comma
			operand2 = operand1.substr(index + 1);
			operand1 = operand1.substr(0, index);
		}
		else if (nArgs >= 3) { // Operands seperated by a space
			operand2 = args.at(2);
		}
		if (!operand1.empty()) { // Check left argument (destination)
			if (operand1.find('(') != std::string::npos) { // Memory address register
				bitSet(type, ADDRESS_LEFT_BIT);
				removeCharacter(operand1, '(');
				removeCharacter(operand1, ')');
			}
			if (isNotNumeric(operand1)) { // Assembler label or register?
				if (std::find(cpuRegisters.begin(), cpuRegisters.end(), operand1) != cpuRegisters.end()) { // Register
					bitSet(type, REGISTER_LEFT_BIT);
				}
				else{ // Label
					bitSet(type, IMMEDIATE_LEFT_BIT);
					label = operand1;
				}
			}
			else { 
				bitSet(type, IMMEDIATE_LEFT_BIT);
				d16 = getUserInputUShort(operand1);
			}
		}
		if (!operand2.empty()) { // Check right argument (source)
			if (operand2.find('(') != std::string::npos) { // Memory address register
				bitSet(type, ADDRESS_RIGHT_BIT);
				removeCharacter(operand2, '(');
				removeCharacter(operand2, ')');
			}
			if (isNotNumeric(operand2)) { // Assembler label or register?
				if (std::find(cpuRegisters.begin(), cpuRegisters.end(), operand2) != cpuRegisters.end()) { // Register
					bitSet(type, REGISTER_RIGHT_BIT);
				}
				else{ // Label
					bitSet(type, IMMEDIATE_RIGHT_BIT);
					label = operand2;
				}
			}
			else {
				bitSet(type, IMMEDIATE_RIGHT_BIT);
				d16 = getUserInputUShort(operand2);
			}
		}
	}
	bool foundMatch = false;
	for (unsigned short i = 0; i < 256; i++) {
		if (opcodes[i].check(args.front(), type, operand1, operand2)) {
			data.set(&opcodes[i]);
			foundMatch = true;
			break;
		}
		else if (opcodesCB[i].check(args.front(), type, operand1, operand2)) {
			data.set(&opcodesCB[i]);
			foundMatch = true;
			break;
		}
	}
	if (!foundMatch) {
		for (auto alias = aliases.begin(); alias != aliases.end(); alias++) {
			if (alias->check(args.front(), type, operand1, operand2)) {
				data.set(&(*alias));
				foundMatch = true;
				break;
			}
		}
	}
	if (foundMatch){
		if (bitTest(type, IMMEDIATE_LEFT_BIT) || bitTest(type, IMMEDIATE_RIGHT_BIT)) { // Set immediate data (if present)
			if (label.empty())
				data.setImmediateData(d16);
			else
				data.setLabel(label);
		}
		return true;
	}
	return false;
}

bool OpcodeHandler::clock(LR35902*){
	lastOpcode.clock();
	bool retval = false;
	if(lastOpcode.onExecute()){
		retval = true;
	}
	return retval;
}

