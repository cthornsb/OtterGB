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

bool OpcodeHandlerLR35902::clock(LR35902 *cpu){ 
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

void OpcodeHandlerLR35902::setOpcodeMemAddressGetter(Opcode* op, LR35902* cpu){
	if(op->hasAddressLeft()) // Memory write
		op->addrptr = cpu->getMemoryAddressFunction(op->sOperandsLeft);	
	else if(op->hasAddressRight()) // Memory read
		op->addrptr = cpu->getMemoryAddressFunction(op->sOperandsRight);
	else
		op->addrptr = 0x0;
}

