#include <stdlib.h>
#include <algorithm>

#include "OpcodeLR35902.hpp"
#include "LR35902.hpp"

void OpcodeHandlerLR35902::setMemoryAccess(LR35902* cpu){
	for (unsigned short i = 0; i < 256; i++) {
		setOpcodeMemAddressGetter(&opcodes[i], cpu);
		setOpcodeMemAddressGetter(&opcodesCB[i], cpu);
	}
}

void OpcodeHandlerLR35902::setOpcodePointer(const unsigned char& index, void (LR35902::*p)()){
	opcodes[index].ptr = p;
}

void OpcodeHandlerLR35902::setOpcodePointerCB(const unsigned char& index, void (LR35902::*p)()){
	opcodesCB[index].ptr = p;
}

bool OpcodeHandlerLR35902::clock(){ 
	lastOpcode.clock();
	bool retval = false;
	if(lastOpcode.onRead())
		cpu->readMemory();
	if(lastOpcode.onExecute()){
		(cpu->*lastOpcode()->ptr)();
		retval = true;
	}
	if(lastOpcode.onWrite())
		cpu->writeMemory();
	return retval;
}

void OpcodeHandlerLR35902::execute(OpcodeData* instruction){
	// Save current CPU state
	LR35902::StateOfCPU prevState = cpu->getCpuState();

	// Get CPU opcode
	Opcode* op = instruction->op;

	// Set the memory read/write address (if any)
	if(op->addrptr)
		cpu->setMemoryAddress((cpu->*op->addrptr)());

	// Set immediate data (if any)
	if(op->nBytes == 2) // 8-bit immediate
		cpu->setd8((unsigned char)(instruction->nData & 0xff));
	else if(op->nBytes == 3) // 16-bit immediate
		cpu->setd16(instruction->nData);

	// Read from system memory
	if(instruction->nReadCycle)
		cpu->readMemory();
		
	// Execute instruction
	(cpu->*op->ptr)();

	// Write to system memory
	if(instruction->nWriteCycle) 
		cpu->writeMemory();
	
	// Reset previous opcode instruction state (but not CPU state)
	cpu->setMemoryAddress(prevState.addr);
	cpu->setMemoryValue(prevState.value);
	cpu->setd8(prevState.d8);
	cpu->setd16((prevState.d16h << 8) + prevState.d16l);
}

void OpcodeHandlerLR35902::setOpcodeMemAddressGetter(Opcode* op, LR35902* cpu){
	if(op->hasAddressLeft()) // Memory write
		op->addrptr = cpu->getMemoryAddressFunction(op->sOperandsLeft);	
	else if(op->hasAddressRight()) // Memory read
		op->addrptr = cpu->getMemoryAddressFunction(op->sOperandsRight);
	else
		op->addrptr = 0x0;
}

