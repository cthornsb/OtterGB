#ifndef OPCODE_LR35902_HPP
#define OPCODE_LR35902_HPP

#include <string>
#include <vector>

#include "Opcode.hpp"

class OpcodeHandlerLR35902 : public OpcodeHandler {
public:
	/** Default constructor
	  */
	OpcodeHandlerLR35902() :
		OpcodeHandler()
	{
	}

	/** Set LR35902 class pointer for all 512 LR35902 opcodes
	  */
	void setMemoryAccess(LR35902* cpu);

	/** Set opcode execute function pointer
	  */
	void setOpcodePointer(const unsigned char& index, void (LR35902::*p)());

	/** Set CB-prefix opcode execute function pointer
	  */
	void setOpcodePointerCB(const unsigned char& index, void (LR35902::*p)());

	/** Clock the currently executing opcode
	  * If clocking the opcode ticks to its read cycle, the memory address will be read.
	  * Similarly, if clocking ticks to its write cycle, the memory address will be written to.
	  * @return True if clocking the current opcode causes it to tick its execute cycle
	  */
	bool clock(LR35902* cpu) override;
	
protected:
	/** Set memory address getter function pointer for all opcodes which read or write to memory
	  */
	void setOpcodeMemAddressGetter(Opcode* op, LR35902* cpu);
};

#endif
