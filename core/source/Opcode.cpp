#include <iostream>
#include <stdlib.h>
#include <algorithm>

#include "Support.hpp"
#include "Opcode.hpp"

#ifdef PROJECT_GBC
#include "LR35902.hpp"
#endif // ifdef PROJECT_GBC

const unsigned char IMMEDIATE_LEFT_BIT  = 0;
const unsigned char IMMEDIATE_RIGHT_BIT = 1;
const unsigned char ADDRESS_LEFT_BIT    = 2;
const unsigned char ADDRESS_RIGHT_BIT   = 3;
const unsigned char REGISTER_LEFT_BIT   = 4;
const unsigned char REGISTER_RIGHT_BIT  = 5;

const std::vector<std::string> cpuRegisters = { "a", "b", "c", "d", "e", "f", "h", "l", "af", "bc", "de", "hl", "hl-", "hl+", "pc", "sp", "nz", "z", "nc", "c" };

Opcode::Opcode(const std::string& mnemonic, const unsigned short& cycles, const unsigned short& bytes, const unsigned short& read, const unsigned short& write) :
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
#ifdef PROJECT_GBC
	sOperandsRight(),
	ptr(0x0),
	addrptr(0x0)
#else
	sOperandsRight()
#endif // ifdef PROJECT_GBC
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

#ifdef PROJECT_GBC
Opcode::Opcode(LR35902* cpu, const std::string &mnemonic, const unsigned short &cycles, const unsigned short &bytes, const unsigned short &read, const unsigned short &write, void (LR35902::*p)()) :
	Opcode(mnemonic, cycles, bytes, read, write)
{
	ptr = p;
	setMemoryPointer(cpu);
}

void Opcode::setMemoryPointer(LR35902* cpu){
	if(bitTest(nType, ADDRESS_LEFT_BIT)) // mem-write
		addrptr = cpu->getMemoryAddressFunction(sOperandsLeft);
	else if(bitTest(nType, ADDRESS_RIGHT_BIT)) // mem-read
		addrptr = cpu->getMemoryAddressFunction(sOperandsRight);
	else
		addrptr = 0x0;
}
#endif // ifdef PROJECT_GBC

bool Opcode::check(const std::string& op, const unsigned char& type, const std::string& arg1/*=""*/, const std::string& arg2/*=""*/) const {
	if(op == sOpname && type == nType){
		if (bitTest(nType, REGISTER_LEFT_BIT) && bitTest(nType, REGISTER_RIGHT_BIT)) {
			/*if (sOperandsLeft == arg1 && sOperandsRight == arg2) {
				std::cout << "           \"" << arg1 << "\"=\"" << sOperandsLeft << "\", \"" << arg2 << "\"=\"" << sOperandsRight << "\"\n";
			}*/
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

#ifdef PROJECT_GBC
bool OpcodeData::clock(LR35902 *cpu){ 
	nCycles++;
	bool retval = false;
	if(nCycles == nReadCycle)
		cpu->readMemory();
	if(nCycles == nExecuteCycle){
		(cpu->*op->ptr)();
		retval = true;
	}
	if(nCycles == nWriteCycle)
		cpu->writeMemory();
	return retval;
}
#endif // ifdef PROJECT_GBC

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

OpcodeHandler::OpcodeHandler() {
	initialize();
}

#ifdef PROJECT_GBC
void OpcodeHandler::setMemoryAccess(LR35902* cpu){
	for (unsigned short i = 0; i < 256; i++) {
		opcodes[i].setMemoryPointer(cpu);
		opcodesCB[i].setMemoryPointer(cpu);
	}
}

void OpcodeHandler::setOpcodePointer(const unsigned char& index, void (LR35902::*p)()){
	opcodes[index].ptr = p;
}

void OpcodeHandler::setOpcodePointerCB(const unsigned char& index, void (LR35902::*p)()){
	opcodesCB[index].ptr = p;
}
#endif // ifdef PROJECT_GBC	

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

void OpcodeHandler::initialize() {
	// Standard opcodes
	//                  Mnemonic        C  L  R  W 
	opcodes[0] = Opcode("NOP         ", 1, 1, 0, 0);
	opcodes[1] = Opcode("LD BC,d16   ", 3, 3, 0, 0);
	opcodes[2] = Opcode("LD (BC),A   ", 2, 1, 0, 2);
	opcodes[3] = Opcode("INC BC      ", 2, 1, 0, 0);
	opcodes[4] = Opcode("INC B       ", 1, 1, 0, 0);
	opcodes[5] = Opcode("DEC B       ", 1, 1, 0, 0);
	opcodes[6] = Opcode("LD B,d8     ", 2, 2, 0, 0);
	opcodes[7] = Opcode("RLCA        ", 1, 1, 0, 0);
	opcodes[8] = Opcode("LD (a16),SP ", 5, 3, 0, 0);
	opcodes[9] = Opcode("ADD HL,BC   ", 2, 1, 0, 0);
	opcodes[10] = Opcode("LD A,(BC)   ", 2, 1, 2, 0);
	opcodes[11] = Opcode("DEC BC      ", 2, 1, 0, 0);
	opcodes[12] = Opcode("INC C       ", 1, 1, 0, 0);
	opcodes[13] = Opcode("DEC C       ", 1, 1, 0, 0);
	opcodes[14] = Opcode("LD C,d8     ", 2, 2, 0, 0);
	opcodes[15] = Opcode("RRCA        ", 1, 1, 0, 0);
	opcodes[16] = Opcode("STOP 0      ", 1, 2, 0, 0);
	opcodes[17] = Opcode("LD DE,d16   ", 3, 3, 0, 0);
	opcodes[18] = Opcode("LD (DE),A   ", 2, 1, 0, 2);
	opcodes[19] = Opcode("INC DE      ", 2, 1, 0, 0);
	opcodes[20] = Opcode("INC D       ", 1, 1, 0, 0);
	opcodes[21] = Opcode("DEC D       ", 1, 1, 0, 0);
	opcodes[22] = Opcode("LD D,d8     ", 2, 2, 0, 0);
	opcodes[23] = Opcode("RLA         ", 1, 1, 0, 0);
	opcodes[24] = Opcode("JR r8       ", 3, 2, 0, 0);
	opcodes[25] = Opcode("ADD HL,DE   ", 2, 1, 0, 0);
	opcodes[26] = Opcode("LD A,(DE)   ", 2, 1, 2, 0);
	opcodes[27] = Opcode("DEC DE      ", 2, 1, 0, 0);
	opcodes[28] = Opcode("INC E       ", 1, 1, 0, 0);
	opcodes[29] = Opcode("DEC E       ", 1, 1, 0, 0);
	opcodes[30] = Opcode("LD E,d8     ", 2, 2, 0, 0);
	opcodes[31] = Opcode("RRA         ", 1, 1, 0, 0);
	opcodes[32] = Opcode("JR NZ,r8    ", 2, 2, 0, 0);
	opcodes[33] = Opcode("LD HL,d16   ", 3, 3, 0, 0);
	opcodes[34] = Opcode("LDI (HL),A  ", 2, 1, 0, 2);
	opcodes[35] = Opcode("INC HL      ", 2, 1, 0, 0);
	opcodes[36] = Opcode("INC H       ", 1, 1, 0, 0);
	opcodes[37] = Opcode("DEC H       ", 1, 1, 0, 0);
	opcodes[38] = Opcode("LD H,d8     ", 2, 2, 0, 0);
	opcodes[39] = Opcode("DAA         ", 1, 1, 0, 0);
	opcodes[40] = Opcode("JR Z,r8     ", 2, 2, 0, 0);
	opcodes[41] = Opcode("ADD HL,HL   ", 2, 1, 0, 0);
	opcodes[42] = Opcode("LDI A,(HL)  ", 2, 1, 2, 0);
	opcodes[43] = Opcode("DEC HL      ", 2, 1, 0, 0);
	opcodes[44] = Opcode("INC L       ", 1, 1, 0, 0);
	opcodes[45] = Opcode("DEC L       ", 1, 1, 0, 0);
	opcodes[46] = Opcode("LD L,d8     ", 2, 2, 0, 0);
	opcodes[47] = Opcode("CPL         ", 1, 1, 0, 0);
	opcodes[48] = Opcode("JR NC,r8    ", 2, 2, 0, 0);
	opcodes[49] = Opcode("LD SP,d16   ", 3, 3, 0, 0);
	opcodes[50] = Opcode("LDD (HL),A  ", 2, 1, 0, 2);
	opcodes[51] = Opcode("INC SP      ", 2, 1, 0, 0);
	opcodes[52] = Opcode("INC (HL)    ", 3, 1, 2, 3);
	opcodes[53] = Opcode("DEC (HL)    ", 3, 1, 2, 3);
	opcodes[54] = Opcode("LD (HL),d8  ", 3, 2, 0, 3);
	opcodes[55] = Opcode("SCF         ", 1, 1, 0, 0);
	opcodes[56] = Opcode("JR C,r8     ", 2, 2, 0, 0);
	opcodes[57] = Opcode("ADD HL,SP   ", 2, 1, 0, 0);
	opcodes[58] = Opcode("LDD A,(HL)  ", 2, 1, 2, 0);
	opcodes[59] = Opcode("DEC SP      ", 2, 1, 0, 0);
	opcodes[60] = Opcode("INC A       ", 1, 1, 0, 0);
	opcodes[61] = Opcode("DEC A       ", 1, 1, 0, 0);
	opcodes[62] = Opcode("LD A,d8     ", 2, 2, 0, 0);
	opcodes[63] = Opcode("CCF         ", 1, 1, 0, 0);
	opcodes[64] = Opcode("LD B,B      ", 1, 1, 0, 0);
	opcodes[65] = Opcode("LD B,C      ", 1, 1, 0, 0);
	opcodes[66] = Opcode("LD B,D      ", 1, 1, 0, 0);
	opcodes[67] = Opcode("LD B,E      ", 1, 1, 0, 0);
	opcodes[68] = Opcode("LD B,H      ", 1, 1, 0, 0);
	opcodes[69] = Opcode("LD B,L      ", 1, 1, 0, 0);
	opcodes[70] = Opcode("LD B,(HL)   ", 2, 1, 2, 0);
	opcodes[71] = Opcode("LD B,A      ", 1, 1, 0, 0);
	opcodes[72] = Opcode("LD C,B      ", 1, 1, 0, 0);
	opcodes[73] = Opcode("LD C,C      ", 1, 1, 0, 0);
	opcodes[74] = Opcode("LD C,D      ", 1, 1, 0, 0);
	opcodes[75] = Opcode("LD C,E      ", 1, 1, 0, 0);
	opcodes[76] = Opcode("LD C,H      ", 1, 1, 0, 0);
	opcodes[77] = Opcode("LD C,L      ", 1, 1, 0, 0);
	opcodes[78] = Opcode("LD C,(HL)   ", 2, 1, 2, 0);
	opcodes[79] = Opcode("LD C,A      ", 1, 1, 0, 0);
	opcodes[80] = Opcode("LD D,B      ", 1, 1, 0, 0);
	opcodes[81] = Opcode("LD D,C      ", 1, 1, 0, 0);
	opcodes[82] = Opcode("LD D,D      ", 1, 1, 0, 0);
	opcodes[83] = Opcode("LD D,E      ", 1, 1, 0, 0);
	opcodes[84] = Opcode("LD D,H      ", 1, 1, 0, 0);
	opcodes[85] = Opcode("LD D,L      ", 1, 1, 0, 0);
	opcodes[86] = Opcode("LD D,(HL)   ", 2, 1, 2, 0);
	opcodes[87] = Opcode("LD D,A      ", 1, 1, 0, 0);
	opcodes[88] = Opcode("LD E,B      ", 1, 1, 0, 0);
	opcodes[89] = Opcode("LD E,C      ", 1, 1, 0, 0);
	opcodes[90] = Opcode("LD E,D      ", 1, 1, 0, 0);
	opcodes[91] = Opcode("LD E,E      ", 1, 1, 0, 0);
	opcodes[92] = Opcode("LD E,H      ", 1, 1, 0, 0);
	opcodes[93] = Opcode("LD E,L      ", 1, 1, 0, 0);
	opcodes[94] = Opcode("LD E,(HL)   ", 2, 1, 2, 0);
	opcodes[95] = Opcode("LD E,A      ", 1, 1, 0, 0);
	opcodes[96] = Opcode("LD H,B      ", 1, 1, 0, 0);
	opcodes[97] = Opcode("LD H,C      ", 1, 1, 0, 0);
	opcodes[98] = Opcode("LD H,D      ", 1, 1, 0, 0);
	opcodes[99] = Opcode("LD H,E      ", 1, 1, 0, 0);
	opcodes[100] = Opcode("LD H,H      ", 1, 1, 0, 0);
	opcodes[101] = Opcode("LD H,L      ", 1, 1, 0, 0);
	opcodes[102] = Opcode("LD H,(HL)   ", 2, 1, 2, 0);
	opcodes[103] = Opcode("LD H,A      ", 1, 1, 0, 0);
	opcodes[104] = Opcode("LD L,B      ", 1, 1, 0, 0);
	opcodes[105] = Opcode("LD L,C      ", 1, 1, 0, 0);
	opcodes[106] = Opcode("LD L,D      ", 1, 1, 0, 0);
	opcodes[107] = Opcode("LD L,E      ", 1, 1, 0, 0);
	opcodes[108] = Opcode("LD L,H      ", 1, 1, 0, 0);
	opcodes[109] = Opcode("LD L,L      ", 1, 1, 0, 0);
	opcodes[110] = Opcode("LD L,(HL)   ", 2, 1, 2, 0);
	opcodes[111] = Opcode("LD L,A      ", 1, 1, 0, 0);
	opcodes[112] = Opcode("LD (HL),B   ", 2, 1, 0, 2);
	opcodes[113] = Opcode("LD (HL),C   ", 2, 1, 0, 2);
	opcodes[114] = Opcode("LD (HL),D   ", 2, 1, 0, 2);
	opcodes[115] = Opcode("LD (HL),E   ", 2, 1, 0, 2);
	opcodes[116] = Opcode("LD (HL),H   ", 2, 1, 0, 2);
	opcodes[117] = Opcode("LD (HL),L   ", 2, 1, 0, 2);
	opcodes[118] = Opcode("HALT        ", 1, 1, 0, 0);
	opcodes[119] = Opcode("LD (HL),A   ", 2, 1, 0, 2);
	opcodes[120] = Opcode("LD A,B      ", 1, 1, 0, 0);
	opcodes[121] = Opcode("LD A,C      ", 1, 1, 0, 0);
	opcodes[122] = Opcode("LD A,D      ", 1, 1, 0, 0);
	opcodes[123] = Opcode("LD A,E      ", 1, 1, 0, 0);
	opcodes[124] = Opcode("LD A,H      ", 1, 1, 0, 0);
	opcodes[125] = Opcode("LD A,L      ", 1, 1, 0, 0);
	opcodes[126] = Opcode("LD A,(HL)   ", 2, 1, 2, 0);
	opcodes[127] = Opcode("LD A,A      ", 1, 1, 0, 0);
	opcodes[128] = Opcode("ADD A,B     ", 1, 1, 0, 0);
	opcodes[129] = Opcode("ADD A,C     ", 1, 1, 0, 0);
	opcodes[130] = Opcode("ADD A,D     ", 1, 1, 0, 0);
	opcodes[131] = Opcode("ADD A,E     ", 1, 1, 0, 0);
	opcodes[132] = Opcode("ADD A,H     ", 1, 1, 0, 0);
	opcodes[133] = Opcode("ADD A,L     ", 1, 1, 0, 0);
	opcodes[134] = Opcode("ADD A,(HL)  ", 2, 1, 2, 0);
	opcodes[135] = Opcode("ADD A,A     ", 1, 1, 0, 0);
	opcodes[136] = Opcode("ADC A,B     ", 1, 1, 0, 0);
	opcodes[137] = Opcode("ADC A,C     ", 1, 1, 0, 0);
	opcodes[138] = Opcode("ADC A,D     ", 1, 1, 0, 0);
	opcodes[139] = Opcode("ADC A,E     ", 1, 1, 0, 0);
	opcodes[140] = Opcode("ADC A,H     ", 1, 1, 0, 0);
	opcodes[141] = Opcode("ADC A,L     ", 1, 1, 0, 0);
	opcodes[142] = Opcode("ADC A,(HL)  ", 2, 1, 2, 0);
	opcodes[143] = Opcode("ADC A,A     ", 1, 1, 0, 0);
	opcodes[144] = Opcode("SUB B       ", 1, 1, 0, 0);
	opcodes[145] = Opcode("SUB C       ", 1, 1, 0, 0);
	opcodes[146] = Opcode("SUB D       ", 1, 1, 0, 0);
	opcodes[147] = Opcode("SUB E       ", 1, 1, 0, 0);
	opcodes[148] = Opcode("SUB H       ", 1, 1, 0, 0);
	opcodes[149] = Opcode("SUB L       ", 1, 1, 0, 0);
	opcodes[150] = Opcode("SUB (HL)    ", 2, 1, 2, 0);
	opcodes[151] = Opcode("SUB A       ", 1, 1, 0, 0);
	opcodes[152] = Opcode("SBC A,B     ", 1, 1, 0, 0);
	opcodes[153] = Opcode("SBC A,C     ", 1, 1, 0, 0);
	opcodes[154] = Opcode("SBC A,D     ", 1, 1, 0, 0);
	opcodes[155] = Opcode("SBC A,E     ", 1, 1, 0, 0);
	opcodes[156] = Opcode("SBC A,H     ", 1, 1, 0, 0);
	opcodes[157] = Opcode("SBC A,L     ", 1, 1, 0, 0);
	opcodes[158] = Opcode("SBC A,(HL)  ", 2, 1, 2, 0);
	opcodes[159] = Opcode("SBC A,A     ", 1, 1, 0, 0);
	opcodes[160] = Opcode("AND B       ", 1, 1, 0, 0);
	opcodes[161] = Opcode("AND C       ", 1, 1, 0, 0);
	opcodes[162] = Opcode("AND D       ", 1, 1, 0, 0);
	opcodes[163] = Opcode("AND E       ", 1, 1, 0, 0);
	opcodes[164] = Opcode("AND H       ", 1, 1, 0, 0);
	opcodes[165] = Opcode("AND L       ", 1, 1, 0, 0);
	opcodes[166] = Opcode("AND (HL)    ", 2, 1, 2, 0);
	opcodes[167] = Opcode("AND A       ", 1, 1, 0, 0);
	opcodes[168] = Opcode("XOR B       ", 1, 1, 0, 0);
	opcodes[169] = Opcode("XOR C       ", 1, 1, 0, 0);
	opcodes[170] = Opcode("XOR D       ", 1, 1, 0, 0);
	opcodes[171] = Opcode("XOR E       ", 1, 1, 0, 0);
	opcodes[172] = Opcode("XOR H       ", 1, 1, 0, 0);
	opcodes[173] = Opcode("XOR L       ", 1, 1, 0, 0);
	opcodes[174] = Opcode("XOR (HL)    ", 2, 1, 2, 0);
	opcodes[175] = Opcode("XOR A       ", 1, 1, 0, 0);
	opcodes[176] = Opcode("OR B        ", 1, 1, 0, 0);
	opcodes[177] = Opcode("OR C        ", 1, 1, 0, 0);
	opcodes[178] = Opcode("OR D        ", 1, 1, 0, 0);
	opcodes[179] = Opcode("OR E        ", 1, 1, 0, 0);
	opcodes[180] = Opcode("OR H        ", 1, 1, 0, 0);
	opcodes[181] = Opcode("OR L        ", 1, 1, 0, 0);
	opcodes[182] = Opcode("OR (HL)     ", 2, 1, 2, 0);
	opcodes[183] = Opcode("OR A        ", 1, 1, 0, 0);
	opcodes[184] = Opcode("CP B        ", 1, 1, 0, 0);
	opcodes[185] = Opcode("CP C        ", 1, 1, 0, 0);
	opcodes[186] = Opcode("CP D        ", 1, 1, 0, 0);
	opcodes[187] = Opcode("CP E        ", 1, 1, 0, 0);
	opcodes[188] = Opcode("CP H        ", 1, 1, 0, 0);
	opcodes[189] = Opcode("CP L        ", 1, 1, 0, 0);
	opcodes[190] = Opcode("CP (HL)     ", 2, 1, 2, 0);
	opcodes[191] = Opcode("CP A        ", 1, 1, 0, 0);
	opcodes[192] = Opcode("RET NZ      ", 2, 1, 0, 0);
	opcodes[193] = Opcode("POP BC      ", 3, 1, 0, 0);
	opcodes[194] = Opcode("JP NZ,a16   ", 3, 3, 0, 0);
	opcodes[195] = Opcode("JP a16      ", 4, 3, 0, 0);
	opcodes[196] = Opcode("CALL NZ,a16 ", 3, 3, 0, 0);
	opcodes[197] = Opcode("PUSH BC     ", 4, 1, 0, 0);
	opcodes[198] = Opcode("ADD A,d8    ", 2, 2, 0, 0);
	opcodes[199] = Opcode("RST 00H     ", 4, 1, 0, 0);
	opcodes[200] = Opcode("RET Z       ", 2, 1, 0, 0);
	opcodes[201] = Opcode("RET         ", 4, 1, 0, 0);
	opcodes[202] = Opcode("JP Z,a16    ", 3, 3, 0, 0);
	opcodes[203] = Opcode("PREFIX CB   ", 0, 1, 0, 0);
	opcodes[204] = Opcode("CALL Z,a16  ", 3, 3, 0, 0);
	opcodes[205] = Opcode("CALL a16    ", 6, 3, 0, 0);
	opcodes[206] = Opcode("ADC A,d8    ", 2, 2, 0, 0);
	opcodes[207] = Opcode("RST 08H     ", 4, 1, 0, 0);
	opcodes[208] = Opcode("RET NC      ", 2, 1, 0, 0);
	opcodes[209] = Opcode("POP DE      ", 3, 1, 0, 0);
	opcodes[210] = Opcode("JP NC,a16   ", 3, 3, 0, 0);
	opcodes[211] = Opcode("            ", 0, 1, 0, 0);
	opcodes[212] = Opcode("CALL NC,a16 ", 3, 3, 0, 0);
	opcodes[213] = Opcode("PUSH DE     ", 4, 1, 0, 0);
	opcodes[214] = Opcode("SUB d8      ", 2, 2, 0, 0);
	opcodes[215] = Opcode("RST 10H     ", 4, 1, 0, 0);
	opcodes[216] = Opcode("RET C       ", 2, 1, 0, 0);
	opcodes[217] = Opcode("RETI        ", 4, 1, 0, 0);
	opcodes[218] = Opcode("JP C,a16    ", 3, 3, 0, 0);
	opcodes[219] = Opcode("            ", 0, 1, 0, 0);
	opcodes[220] = Opcode("CALL C,a16  ", 3, 3, 0, 0);
	opcodes[221] = Opcode("            ", 0, 1, 0, 0);
	opcodes[222] = Opcode("SBC A,d8    ", 2, 2, 0, 0);
	opcodes[223] = Opcode("RST 18H     ", 4, 1, 0, 0);
	opcodes[224] = Opcode("LDH (a8),A  ", 3, 2, 0, 3);
	opcodes[225] = Opcode("POP HL      ", 3, 1, 0, 0);
	opcodes[226] = Opcode("LD (C),A    ", 2, 1, 0, 2);
	opcodes[227] = Opcode("            ", 0, 1, 0, 0);
	opcodes[228] = Opcode("            ", 0, 1, 0, 0);
	opcodes[229] = Opcode("PUSH HL     ", 4, 1, 0, 0);
	opcodes[230] = Opcode("AND d8      ", 2, 2, 0, 0);
	opcodes[231] = Opcode("RST 20H     ", 4, 1, 0, 0);
	opcodes[232] = Opcode("ADD SP,r8   ", 4, 2, 0, 0);
	opcodes[233] = Opcode("JP (HL)     ", 1, 1, 0, 0);
	opcodes[234] = Opcode("LD (a16),A  ", 4, 3, 0, 4);
	opcodes[235] = Opcode("            ", 0, 1, 0, 0);
	opcodes[236] = Opcode("            ", 0, 1, 0, 0);
	opcodes[237] = Opcode("            ", 0, 1, 0, 0);
	opcodes[238] = Opcode("XOR d8      ", 2, 2, 0, 0);
	opcodes[239] = Opcode("RST 28H     ", 4, 1, 0, 0);
	opcodes[240] = Opcode("LDH A,(a8)  ", 3, 2, 3, 0);
	opcodes[241] = Opcode("POP AF      ", 3, 1, 0, 0);
	opcodes[242] = Opcode("LD A,(C)    ", 2, 1, 2, 0);
	opcodes[243] = Opcode("DI          ", 1, 1, 0, 0);
	opcodes[244] = Opcode("            ", 0, 1, 0, 0);
	opcodes[245] = Opcode("PUSH AF     ", 4, 1, 0, 0);
	opcodes[246] = Opcode("OR d8       ", 2, 2, 0, 0);
	opcodes[247] = Opcode("RST 30H     ", 4, 1, 0, 0);
	opcodes[248] = Opcode("LD HL,SP+r8 ", 3, 2, 0, 0);
	opcodes[249] = Opcode("LD SP,HL    ", 2, 1, 0, 0);
	opcodes[250] = Opcode("LD A,(a16)  ", 4, 3, 4, 0);
	opcodes[251] = Opcode("EI          ", 1, 1, 0, 0);
	opcodes[252] = Opcode("            ", 0, 1, 0, 0);
	opcodes[253] = Opcode("            ", 0, 1, 0, 0);
	opcodes[254] = Opcode("CP d8       ", 2, 2, 0, 0);
	opcodes[255] = Opcode("RST 38H     ", 4, 1, 0, 0);

	// Add opcode aliases
	aliases.push_back(Opcode("LD (HL+),A  ", 2, 1, 0, 2));
	aliases.push_back(Opcode("LD A,(HL+)  ", 2, 1, 2, 0));	
	aliases.push_back(Opcode("LD (HL-),A  ", 2, 1, 0, 2));
	aliases.push_back(Opcode("LD A,(HL-)  ", 2, 1, 2, 0));
	aliases.push_back(Opcode("ADD d8      ", 2, 2, 0, 0));
	aliases.push_back(Opcode("ADC d8      ", 2, 2, 0, 0));

	// CB prefix opcodes
	//                    Mnemonic        C  L  R  Wr
	opcodesCB[0] = Opcode("RLC B       ", 2, 1, 0, 0);
	opcodesCB[1] = Opcode("RLC C       ", 2, 1, 0, 0);
	opcodesCB[2] = Opcode("RLC D       ", 2, 1, 0, 0);
	opcodesCB[3] = Opcode("RLC E       ", 2, 1, 0, 0);
	opcodesCB[4] = Opcode("RLC H       ", 2, 1, 0, 0);
	opcodesCB[5] = Opcode("RLC L       ", 2, 1, 0, 0);
	opcodesCB[6] = Opcode("RLC (HL)    ", 4, 1, 3, 4);
	opcodesCB[7] = Opcode("RLC A       ", 2, 1, 0, 0);
	opcodesCB[8] = Opcode("RRC B       ", 2, 1, 0, 0);
	opcodesCB[9] = Opcode("RRC C       ", 2, 1, 0, 0);
	opcodesCB[10] = Opcode("RRC D       ", 2, 1, 0, 0);
	opcodesCB[11] = Opcode("RRC E       ", 2, 1, 0, 0);
	opcodesCB[12] = Opcode("RRC H       ", 2, 1, 0, 0);
	opcodesCB[13] = Opcode("RRC L       ", 2, 1, 0, 0);
	opcodesCB[14] = Opcode("RRC (HL)    ", 4, 1, 3, 4);
	opcodesCB[15] = Opcode("RRC A       ", 2, 1, 0, 0);
	opcodesCB[16] = Opcode("RL B        ", 2, 1, 0, 0);
	opcodesCB[17] = Opcode("RL C        ", 2, 1, 0, 0);
	opcodesCB[18] = Opcode("RL D        ", 2, 1, 0, 0);
	opcodesCB[19] = Opcode("RL E        ", 2, 1, 0, 0);
	opcodesCB[20] = Opcode("RL H        ", 2, 1, 0, 0);
	opcodesCB[21] = Opcode("RL L        ", 2, 1, 0, 0);
	opcodesCB[22] = Opcode("RL (HL)     ", 4, 1, 3, 4);
	opcodesCB[23] = Opcode("RL A        ", 2, 1, 0, 0);
	opcodesCB[24] = Opcode("RR B        ", 2, 1, 0, 0);
	opcodesCB[25] = Opcode("RR C        ", 2, 1, 0, 0);
	opcodesCB[26] = Opcode("RR D        ", 2, 1, 0, 0);
	opcodesCB[27] = Opcode("RR E        ", 2, 1, 0, 0);
	opcodesCB[28] = Opcode("RR H        ", 2, 1, 0, 0);
	opcodesCB[29] = Opcode("RR L        ", 2, 1, 0, 0);
	opcodesCB[30] = Opcode("RR (HL)     ", 4, 1, 3, 4);
	opcodesCB[31] = Opcode("RR A        ", 2, 1, 0, 0);
	opcodesCB[32] = Opcode("SLA B       ", 2, 1, 0, 0);
	opcodesCB[33] = Opcode("SLA C       ", 2, 1, 0, 0);
	opcodesCB[34] = Opcode("SLA D       ", 2, 1, 0, 0);
	opcodesCB[35] = Opcode("SLA E       ", 2, 1, 0, 0);
	opcodesCB[36] = Opcode("SLA H       ", 2, 1, 0, 0);
	opcodesCB[37] = Opcode("SLA L       ", 2, 1, 0, 0);
	opcodesCB[38] = Opcode("SLA (HL)    ", 4, 1, 3, 4);
	opcodesCB[39] = Opcode("SLA A       ", 2, 1, 0, 0);
	opcodesCB[40] = Opcode("SRA B       ", 2, 1, 0, 0);
	opcodesCB[41] = Opcode("SRA C       ", 2, 1, 0, 0);
	opcodesCB[42] = Opcode("SRA D       ", 2, 1, 0, 0);
	opcodesCB[43] = Opcode("SRA E       ", 2, 1, 0, 0);
	opcodesCB[44] = Opcode("SRA H       ", 2, 1, 0, 0);
	opcodesCB[45] = Opcode("SRA L       ", 2, 1, 0, 0);
	opcodesCB[46] = Opcode("SRA (HL)    ", 4, 1, 3, 4);
	opcodesCB[47] = Opcode("SRA A       ", 2, 1, 0, 0);
	opcodesCB[48] = Opcode("SWAP B      ", 2, 1, 0, 0);
	opcodesCB[49] = Opcode("SWAP C      ", 2, 1, 0, 0);
	opcodesCB[50] = Opcode("SWAP D      ", 2, 1, 0, 0);
	opcodesCB[51] = Opcode("SWAP E      ", 2, 1, 0, 0);
	opcodesCB[52] = Opcode("SWAP H      ", 2, 1, 0, 0);
	opcodesCB[53] = Opcode("SWAP L      ", 2, 1, 0, 0);
	opcodesCB[54] = Opcode("SWAP (HL)   ", 4, 1, 3, 4);
	opcodesCB[55] = Opcode("SWAP A      ", 2, 1, 0, 0);
	opcodesCB[56] = Opcode("SRL B       ", 2, 1, 0, 0);
	opcodesCB[57] = Opcode("SRL C       ", 2, 1, 0, 0);
	opcodesCB[58] = Opcode("SRL D       ", 2, 1, 0, 0);
	opcodesCB[59] = Opcode("SRL E       ", 2, 1, 0, 0);
	opcodesCB[60] = Opcode("SRL H       ", 2, 1, 0, 0);
	opcodesCB[61] = Opcode("SRL L       ", 2, 1, 0, 0);
	opcodesCB[62] = Opcode("SRL (HL)    ", 4, 1, 3, 4);
	opcodesCB[63] = Opcode("SRL A       ", 2, 1, 0, 0);
	opcodesCB[64] = Opcode("BIT 0,B     ", 2, 1, 0, 0);
	opcodesCB[65] = Opcode("BIT 0,C     ", 2, 1, 0, 0);
	opcodesCB[66] = Opcode("BIT 0,D     ", 2, 1, 0, 0);
	opcodesCB[67] = Opcode("BIT 0,E     ", 2, 1, 0, 0);
	opcodesCB[68] = Opcode("BIT 0,H     ", 2, 1, 0, 0);
	opcodesCB[69] = Opcode("BIT 0,L     ", 2, 1, 0, 0);
	opcodesCB[70] = Opcode("BIT 0,(HL)  ", 3, 1, 3, 0);
	opcodesCB[71] = Opcode("BIT 0,A     ", 2, 1, 0, 0);
	opcodesCB[72] = Opcode("BIT 1,B     ", 2, 1, 0, 0);
	opcodesCB[73] = Opcode("BIT 1,C     ", 2, 1, 0, 0);
	opcodesCB[74] = Opcode("BIT 1,D     ", 2, 1, 0, 0);
	opcodesCB[75] = Opcode("BIT 1,E     ", 2, 1, 0, 0);
	opcodesCB[76] = Opcode("BIT 1,H     ", 2, 1, 0, 0);
	opcodesCB[77] = Opcode("BIT 1,L     ", 2, 1, 0, 0);
	opcodesCB[78] = Opcode("BIT 1,(HL)  ", 3, 1, 3, 0);
	opcodesCB[79] = Opcode("BIT 1,A     ", 2, 1, 0, 0);
	opcodesCB[80] = Opcode("BIT 2,B     ", 2, 1, 0, 0);
	opcodesCB[81] = Opcode("BIT 2,C     ", 2, 1, 0, 0);
	opcodesCB[82] = Opcode("BIT 2,D     ", 2, 1, 0, 0);
	opcodesCB[83] = Opcode("BIT 2,E     ", 2, 1, 0, 0);
	opcodesCB[84] = Opcode("BIT 2,H     ", 2, 1, 0, 0);
	opcodesCB[85] = Opcode("BIT 2,L     ", 2, 1, 0, 0);
	opcodesCB[86] = Opcode("BIT 2,(HL)  ", 3, 1, 3, 0);
	opcodesCB[87] = Opcode("BIT 2,A     ", 2, 1, 0, 0);
	opcodesCB[88] = Opcode("BIT 3,B     ", 2, 1, 0, 0);
	opcodesCB[89] = Opcode("BIT 3,C     ", 2, 1, 0, 0);
	opcodesCB[90] = Opcode("BIT 3,D     ", 2, 1, 0, 0);
	opcodesCB[91] = Opcode("BIT 3,E     ", 2, 1, 0, 0);
	opcodesCB[92] = Opcode("BIT 3,H     ", 2, 1, 0, 0);
	opcodesCB[93] = Opcode("BIT 3,L     ", 2, 1, 0, 0);
	opcodesCB[94] = Opcode("BIT 3,(HL)  ", 3, 1, 3, 0);
	opcodesCB[95] = Opcode("BIT 3,A     ", 2, 1, 0, 0);
	opcodesCB[96] = Opcode("BIT 4,B     ", 2, 1, 0, 0);
	opcodesCB[97] = Opcode("BIT 4,C     ", 2, 1, 0, 0);
	opcodesCB[98] = Opcode("BIT 4,D     ", 2, 1, 0, 0);
	opcodesCB[99] = Opcode("BIT 4,E     ", 2, 1, 0, 0);
	opcodesCB[100] = Opcode("BIT 4,H     ", 2, 1, 0, 0);
	opcodesCB[101] = Opcode("BIT 4,L     ", 2, 1, 0, 0);
	opcodesCB[102] = Opcode("BIT 4,(HL)  ", 3, 1, 3, 0);
	opcodesCB[103] = Opcode("BIT 4,A     ", 2, 1, 0, 0);
	opcodesCB[104] = Opcode("BIT 5,B     ", 2, 1, 0, 0);
	opcodesCB[105] = Opcode("BIT 5,C     ", 2, 1, 0, 0);
	opcodesCB[106] = Opcode("BIT 5,D     ", 2, 1, 0, 0);
	opcodesCB[107] = Opcode("BIT 5,E     ", 2, 1, 0, 0);
	opcodesCB[108] = Opcode("BIT 5,H     ", 2, 1, 0, 0);
	opcodesCB[109] = Opcode("BIT 5,L     ", 2, 1, 0, 0);
	opcodesCB[110] = Opcode("BIT 5,(HL)  ", 3, 1, 3, 0);
	opcodesCB[111] = Opcode("BIT 5,A     ", 2, 1, 0, 0);
	opcodesCB[112] = Opcode("BIT 6,B     ", 2, 1, 0, 0);
	opcodesCB[113] = Opcode("BIT 6,C     ", 2, 1, 0, 0);
	opcodesCB[114] = Opcode("BIT 6,D     ", 2, 1, 0, 0);
	opcodesCB[115] = Opcode("BIT 6,E     ", 2, 1, 0, 0);
	opcodesCB[116] = Opcode("BIT 6,H     ", 2, 1, 0, 0);
	opcodesCB[117] = Opcode("BIT 6,L     ", 2, 1, 0, 0);
	opcodesCB[118] = Opcode("BIT 6,(HL)  ", 3, 1, 3, 0);
	opcodesCB[119] = Opcode("BIT 6,A     ", 2, 1, 0, 0);
	opcodesCB[120] = Opcode("BIT 7,B     ", 2, 1, 0, 0);
	opcodesCB[121] = Opcode("BIT 7,C     ", 2, 1, 0, 0);
	opcodesCB[122] = Opcode("BIT 7,D     ", 2, 1, 0, 0);
	opcodesCB[123] = Opcode("BIT 7,E     ", 2, 1, 0, 0);
	opcodesCB[124] = Opcode("BIT 7,H     ", 2, 1, 0, 0);
	opcodesCB[125] = Opcode("BIT 7,L     ", 2, 1, 0, 0);
	opcodesCB[126] = Opcode("BIT 7,(HL)  ", 3, 1, 3, 0);
	opcodesCB[127] = Opcode("BIT 7,A     ", 2, 1, 0, 0);
	opcodesCB[128] = Opcode("RES 0,B     ", 2, 1, 0, 0);
	opcodesCB[129] = Opcode("RES 0,C     ", 2, 1, 0, 0);
	opcodesCB[130] = Opcode("RES 0,D     ", 2, 1, 0, 0);
	opcodesCB[131] = Opcode("RES 0,E     ", 2, 1, 0, 0);
	opcodesCB[132] = Opcode("RES 0,H     ", 2, 1, 0, 0);
	opcodesCB[133] = Opcode("RES 0,L     ", 2, 1, 0, 0);
	opcodesCB[134] = Opcode("RES 0,(HL)  ", 4, 1, 3, 4);
	opcodesCB[135] = Opcode("RES 0,A     ", 2, 1, 0, 0);
	opcodesCB[136] = Opcode("RES 1,B     ", 2, 1, 0, 0);
	opcodesCB[137] = Opcode("RES 1,C     ", 2, 1, 0, 0);
	opcodesCB[138] = Opcode("RES 1,D     ", 2, 1, 0, 0);
	opcodesCB[139] = Opcode("RES 1,E     ", 2, 1, 0, 0);
	opcodesCB[140] = Opcode("RES 1,H     ", 2, 1, 0, 0);
	opcodesCB[141] = Opcode("RES 1,L     ", 2, 1, 0, 0);
	opcodesCB[142] = Opcode("RES 1,(HL)  ", 4, 1, 3, 4);
	opcodesCB[143] = Opcode("RES 1,A     ", 2, 1, 0, 0);
	opcodesCB[144] = Opcode("RES 2,B     ", 2, 1, 0, 0);
	opcodesCB[145] = Opcode("RES 2,C     ", 2, 1, 0, 0);
	opcodesCB[146] = Opcode("RES 2,D     ", 2, 1, 0, 0);
	opcodesCB[147] = Opcode("RES 2,E     ", 2, 1, 0, 0);
	opcodesCB[148] = Opcode("RES 2,H     ", 2, 1, 0, 0);
	opcodesCB[149] = Opcode("RES 2,L     ", 2, 1, 0, 0);
	opcodesCB[150] = Opcode("RES 2,(HL)  ", 4, 1, 3, 4);
	opcodesCB[151] = Opcode("RES 2,A     ", 2, 1, 0, 0);
	opcodesCB[152] = Opcode("RES 3,B     ", 2, 1, 0, 0);
	opcodesCB[153] = Opcode("RES 3,C     ", 2, 1, 0, 0);
	opcodesCB[154] = Opcode("RES 3,D     ", 2, 1, 0, 0);
	opcodesCB[155] = Opcode("RES 3,E     ", 2, 1, 0, 0);
	opcodesCB[156] = Opcode("RES 3,H     ", 2, 1, 0, 0);
	opcodesCB[157] = Opcode("RES 3,L     ", 2, 1, 0, 0);
	opcodesCB[158] = Opcode("RES 3,(HL)  ", 4, 1, 3, 4);
	opcodesCB[159] = Opcode("RES 3,A     ", 2, 1, 0, 0);
	opcodesCB[160] = Opcode("RES 4,B     ", 2, 1, 0, 0);
	opcodesCB[161] = Opcode("RES 4,C     ", 2, 1, 0, 0);
	opcodesCB[162] = Opcode("RES 4,D     ", 2, 1, 0, 0);
	opcodesCB[163] = Opcode("RES 4,E     ", 2, 1, 0, 0);
	opcodesCB[164] = Opcode("RES 4,H     ", 2, 1, 0, 0);
	opcodesCB[165] = Opcode("RES 4,L     ", 2, 1, 0, 0);
	opcodesCB[166] = Opcode("RES 4,(HL)  ", 4, 1, 3, 4);
	opcodesCB[167] = Opcode("RES 4,A     ", 2, 1, 0, 0);
	opcodesCB[168] = Opcode("RES 5,B     ", 2, 1, 0, 0);
	opcodesCB[169] = Opcode("RES 5,C     ", 2, 1, 0, 0);
	opcodesCB[170] = Opcode("RES 5,D     ", 2, 1, 0, 0);
	opcodesCB[171] = Opcode("RES 5,E     ", 2, 1, 0, 0);
	opcodesCB[172] = Opcode("RES 5,H     ", 2, 1, 0, 0);
	opcodesCB[173] = Opcode("RES 5,L     ", 2, 1, 0, 0);
	opcodesCB[174] = Opcode("RES 5,(HL)  ", 4, 1, 3, 4);
	opcodesCB[175] = Opcode("RES 5,A     ", 2, 1, 0, 0);
	opcodesCB[176] = Opcode("RES 6,B     ", 2, 1, 0, 0);
	opcodesCB[177] = Opcode("RES 6,C     ", 2, 1, 0, 0);
	opcodesCB[178] = Opcode("RES 6,D     ", 2, 1, 0, 0);
	opcodesCB[179] = Opcode("RES 6,E     ", 2, 1, 0, 0);
	opcodesCB[180] = Opcode("RES 6,H     ", 2, 1, 0, 0);
	opcodesCB[181] = Opcode("RES 6,L     ", 2, 1, 0, 0);
	opcodesCB[182] = Opcode("RES 6,(HL)  ", 4, 1, 3, 4);
	opcodesCB[183] = Opcode("RES 6,A     ", 2, 1, 0, 0);
	opcodesCB[184] = Opcode("RES 7,B     ", 2, 1, 0, 0);
	opcodesCB[185] = Opcode("RES 7,C     ", 2, 1, 0, 0);
	opcodesCB[186] = Opcode("RES 7,D     ", 2, 1, 0, 0);
	opcodesCB[187] = Opcode("RES 7,E     ", 2, 1, 0, 0);
	opcodesCB[188] = Opcode("RES 7,H     ", 2, 1, 0, 0);
	opcodesCB[189] = Opcode("RES 7,L     ", 2, 1, 0, 0);
	opcodesCB[190] = Opcode("RES 7,(HL)  ", 4, 1, 3, 4);
	opcodesCB[191] = Opcode("RES 7,A     ", 2, 1, 0, 0);
	opcodesCB[192] = Opcode("SET 0,B     ", 2, 1, 0, 0);
	opcodesCB[193] = Opcode("SET 0,C     ", 2, 1, 0, 0);
	opcodesCB[194] = Opcode("SET 0,D     ", 2, 1, 0, 0);
	opcodesCB[195] = Opcode("SET 0,E     ", 2, 1, 0, 0);
	opcodesCB[196] = Opcode("SET 0,H     ", 2, 1, 0, 0);
	opcodesCB[197] = Opcode("SET 0,L     ", 2, 1, 0, 0);
	opcodesCB[198] = Opcode("SET 0,(HL)  ", 4, 1, 3, 4);
	opcodesCB[199] = Opcode("SET 0,A     ", 2, 1, 0, 0);
	opcodesCB[200] = Opcode("SET 1,B     ", 2, 1, 0, 0);
	opcodesCB[201] = Opcode("SET 1,C     ", 2, 1, 0, 0);
	opcodesCB[202] = Opcode("SET 1,D     ", 2, 1, 0, 0);
	opcodesCB[203] = Opcode("SET 1,E     ", 2, 1, 0, 0);
	opcodesCB[204] = Opcode("SET 1,H     ", 2, 1, 0, 0);
	opcodesCB[205] = Opcode("SET 1,L     ", 2, 1, 0, 0);
	opcodesCB[206] = Opcode("SET 1,(HL)  ", 4, 1, 3, 4);
	opcodesCB[207] = Opcode("SET 1,A     ", 2, 1, 0, 0);
	opcodesCB[208] = Opcode("SET 2,B     ", 2, 1, 0, 0);
	opcodesCB[209] = Opcode("SET 2,C     ", 2, 1, 0, 0);
	opcodesCB[210] = Opcode("SET 2,D     ", 2, 1, 0, 0);
	opcodesCB[211] = Opcode("SET 2,E     ", 2, 1, 0, 0);
	opcodesCB[212] = Opcode("SET 2,H     ", 2, 1, 0, 0);
	opcodesCB[213] = Opcode("SET 2,L     ", 2, 1, 0, 0);
	opcodesCB[214] = Opcode("SET 2,(HL)  ", 4, 1, 3, 4);
	opcodesCB[215] = Opcode("SET 2,A     ", 2, 1, 0, 0);
	opcodesCB[216] = Opcode("SET 3,B     ", 2, 1, 0, 0);
	opcodesCB[217] = Opcode("SET 3,C     ", 2, 1, 0, 0);
	opcodesCB[218] = Opcode("SET 3,D     ", 2, 1, 0, 0);
	opcodesCB[219] = Opcode("SET 3,E     ", 2, 1, 0, 0);
	opcodesCB[220] = Opcode("SET 3,H     ", 2, 1, 0, 0);
	opcodesCB[221] = Opcode("SET 3,L     ", 2, 1, 0, 0);
	opcodesCB[222] = Opcode("SET 3,(HL)  ", 4, 1, 3, 4);
	opcodesCB[223] = Opcode("SET 3,A     ", 2, 1, 0, 0);
	opcodesCB[224] = Opcode("SET 4,B     ", 2, 1, 0, 0);
	opcodesCB[225] = Opcode("SET 4,C     ", 2, 1, 0, 0);
	opcodesCB[226] = Opcode("SET 4,D     ", 2, 1, 0, 0);
	opcodesCB[227] = Opcode("SET 4,E     ", 2, 1, 0, 0);
	opcodesCB[228] = Opcode("SET 4,H     ", 2, 1, 0, 0);
	opcodesCB[229] = Opcode("SET 4,L     ", 2, 1, 0, 0);
	opcodesCB[230] = Opcode("SET 4,(HL)  ", 4, 1, 3, 4);
	opcodesCB[231] = Opcode("SET 4,A     ", 2, 1, 0, 0);
	opcodesCB[232] = Opcode("SET 5,B     ", 2, 1, 0, 0);
	opcodesCB[233] = Opcode("SET 5,C     ", 2, 1, 0, 0);
	opcodesCB[234] = Opcode("SET 5,D     ", 2, 1, 0, 0);
	opcodesCB[235] = Opcode("SET 5,E     ", 2, 1, 0, 0);
	opcodesCB[236] = Opcode("SET 5,H     ", 2, 1, 0, 0);
	opcodesCB[237] = Opcode("SET 5,L     ", 2, 1, 0, 0);
	opcodesCB[238] = Opcode("SET 5,(HL)  ", 4, 1, 3, 4);
	opcodesCB[239] = Opcode("SET 5,A     ", 2, 1, 0, 0);
	opcodesCB[240] = Opcode("SET 6,B     ", 2, 1, 0, 0);
	opcodesCB[241] = Opcode("SET 6,C     ", 2, 1, 0, 0);
	opcodesCB[242] = Opcode("SET 6,D     ", 2, 1, 0, 0);
	opcodesCB[243] = Opcode("SET 6,E     ", 2, 1, 0, 0);
	opcodesCB[244] = Opcode("SET 6,H     ", 2, 1, 0, 0);
	opcodesCB[245] = Opcode("SET 6,L     ", 2, 1, 0, 0);
	opcodesCB[246] = Opcode("SET 6,(HL)  ", 4, 1, 3, 4);
	opcodesCB[247] = Opcode("SET 6,A     ", 2, 1, 0, 0);
	opcodesCB[248] = Opcode("SET 7,B     ", 2, 1, 0, 0);
	opcodesCB[249] = Opcode("SET 7,C     ", 2, 1, 0, 0);
	opcodesCB[250] = Opcode("SET 7,D     ", 2, 1, 0, 0);
	opcodesCB[251] = Opcode("SET 7,E     ", 2, 1, 0, 0);
	opcodesCB[252] = Opcode("SET 7,H     ", 2, 1, 0, 0);
	opcodesCB[253] = Opcode("SET 7,L     ", 2, 1, 0, 0);
	opcodesCB[254] = Opcode("SET 7,(HL)  ", 4, 1, 3, 4);
	opcodesCB[255] = Opcode("SET 7,A     ", 2, 1, 0, 0);
}
