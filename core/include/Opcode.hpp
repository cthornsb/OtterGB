#ifndef OPCODE_HPP
#define OPCODE_HPP

#include <string>
#include <vector>

extern const unsigned char FLAG_Z_BIT; ///< LR35902 CPU (Z)ero flag bit
extern const unsigned char FLAG_S_BIT; ///< LR35902 CPU (S)ubtract flag bit
extern const unsigned char FLAG_H_BIT; ///< LR35902 CPU (H)alf-carry flag bit
extern const unsigned char FLAG_C_BIT; ///< LR35902 CPU full-(C)arry flag bit

extern const unsigned char FLAG_Z_MASK; ///< LR35902 CPU (Z)ero flag bitmask
extern const unsigned char FLAG_S_MASK; ///< LR35902 CPU (S)ubtract flag bitmask
extern const unsigned char FLAG_H_MASK; ///< LR35902 CPU (H)alf-carry flag bitmask
extern const unsigned char FLAG_C_MASK; ///< LR35902 CPU full-(C)arry flag bitmask

class LR35902;

typedef unsigned short (LR35902::*addrGetFunc)() const; ///< LR35902 memory address getter function

class Opcode {
public:
	unsigned char nType; ///< Opcode type

	unsigned short nCycles; ///< Length of clock cycles required.
	
	unsigned short nBytes; ///< Length of instruction in bytes.

	unsigned short nReadCycles; ///< Flag indicating this instruction reads from memory.
	
	unsigned short nWriteCycles; ///< Flag indicating this instruction writes to memory.

	std::string sName; ///< The full instruction mnemonic.

	std::string sPrefix; ///< Sub-string of mnemonic before immediate data target
	
	std::string sSuffix; ///< Sub-string of mnemonic after immediate data target

	std::string sOpname; ///< Opcode name.
	
	std::string sOperandsLeft; ///< Left hand operand name
	
	std::string sOperandsRight; ///< Right hand operand name

	void (LR35902::*ptr)(); ///< Pointer to the instruction code.

	addrGetFunc addrptr; ///< Pointer to the function which returns the memory address.

	/** Default LR35902 opcode constructor
	  */
	Opcode() : 
		nType(0),
		nCycles(0), 
		nBytes(0), 
		nReadCycles(0), 
		nWriteCycles(0), 
		sName(),
		sPrefix(),
		sSuffix(),
		sOpname(),
		sOperandsLeft(),
		sOperandsRight(),
		ptr(0x0),
		addrptr(0x0)
	{ 
	}
	
	/** LR35902 assembly mnemonic constructor
	  */
	Opcode(const std::string& mnemonic, const unsigned short& cycles, const unsigned short& bytes, const unsigned short& read, const unsigned short& write);

	/** LR35902 emulator constructor
	  */	
	Opcode(LR35902 *cpu, const std::string &mnemonic, const unsigned short &cycles, const unsigned short &bytes, const unsigned short &read, const unsigned short &write, void (LR35902::*p)());

	/** Return true if opcode argument 1 is an immediate value
	  */
	bool hasImmediateDataLeft() const ;

	/** Return true if opcode argument 2 is an immediate value
	  */
	bool hasImmediateDataRight() const ;

	/** Return true if opcode argument 1 is a memory address
	  */
	bool hasAddressLeft() const ;

	/** Return true if opcode argument 2 is a memory address
	  */
	bool hasAddressRight() const ;

	/** Return true if opcode argument 1 is a CPU register
	  */
	bool hasRegisterLeft() const ;

	/** Return true if opcode argument 2 is a CPU register
	  */
	bool hasRegisterRight() const ;
	
	/** Check whether or not this opcode matches a specified opcode
	  */
	bool check(const std::string& op, const unsigned char& type, const std::string& arg1="", const std::string& arg2="") const ;
};

class OpcodeData{
public:
	Opcode *op; ///< Pointer to the opcode instruction.
	
	unsigned char nIndex; ///< Opcode index
	
	unsigned short nData; ///< Immediate data
	
	unsigned short nPC; ///< Program counter
	
	unsigned short nCycles; ///< Clock cycles since start of instruction
	
	unsigned short nExtraCycles; ///< Number of extra clock cycles added by a conditional operation
	
	unsigned short nReadCycle; ///< Clock cycle on which to read from memory
	
	unsigned short nWriteCycle; ///< Clock cycle on which to write to memory
	
	unsigned short nExecuteCycle; ///< Clock cycle on which to execute instruction
	
	std::string sLabel; ///< String representing an assembler label or variable name

	bool cbPrefix; ///< CB Prefix opcodes.
	
	/** Default constructor
	  */
	OpcodeData();
	
	/** Get pointer to currently executing opcode instruction
	  */
	Opcode * operator () () {
		return op;
	}
	
	/** Return true if the current instruction has at least one clock cycle remaining
	  */
	bool executing() const {
		return (nCycles < (nExecuteCycle + nExtraCycles));
	}

	/** Return true if the current clock cycle is the current instruction's 
	  * read cycle, when a value is read from memory (if applicable)
	  */
	bool onRead() const {
		return (nCycles == nReadCycle);
	}
	
	/** Return true if the current clock cycle is the current instruction's 
	  * write cycle, when a value is written to memory (if applicable)
	  */
	bool onWrite() const {
		return (nCycles == nWriteCycle);
	}

	/** Return true if the current clock cycle is the current instructions's 
	  * execute cycle, when its associated instruction is executed
	  */
	bool onExecute() const {
		return (nCycles == nExecuteCycle);
	}

	/** Return true if the current instruction is in overtime, i.e. extra clock
	  * cycles for a conditional instruction
	  */
	bool onOvertime() const {
		return (nCycles > nExecuteCycle);
	}

	/** Return true if the current instruction either writes to or reads from memory
	  */
	bool memoryAccess() const {
		return (nReadCycle || nWriteCycle);
	}

	/** Return the number of clock cycles remaining for the current instruction
	  * including any extra cycles due to a conditional instruction
	  */
	unsigned short cyclesRemaining() const {
		return (nExecuteCycle + nExtraCycles - nCycles);
	}

	/** Get the current program counter (in hex) and the current instruction string
	  */
	std::string getInstruction() const ;
	
	/** Get the current instruction string
	  */
	std::string getShortInstruction() const ; 
	
	/** Add extra cycles to the current instruction due to a conditional instruction
	  */
	void addCycles(const unsigned short &extra){ 
		nExtraCycles = extra;
	}
	
	/** Get the current instruction's immediate 8-bit data (if applicable)
	  */
	unsigned char getd8() const {
		return (unsigned char)nData;
	}
	
	/** Get the current instruction's immediate 16-bit data (if applicable)
	  */
	unsigned short getd16() const {
		return nData;
	}
	
	/** Set a new opcode instruction
	  * @param opcodes_ Array of all LR35902 CPU opcodes
	  * @param index_ Opcode index (0-255)
	  * @param pc_ Current 16-bit program counter
	  */
	void set(Opcode *opcodes_, const unsigned char &index_, const unsigned short &pc_);
	
	/** Set a new opcode instruction
	  * @param op_ Pointer to the new opcode
	  */
	void set(Opcode *op_);
	
	/** Set a new CB-prefix opcode instruction
	  * @param opcodes_ Array of all CB-prefix LR35902 CPU opcodes
	  * @param index_ Opcode index (0-255)
	  * @param pc_ Current 16-bit program counter
	  */
	void setCB(Opcode *opcodes_, const unsigned char &index_, const unsigned short &pc_);

	/** Set a new CB-prefix opcode instruction
	  * @param op_ Pointer to the new opcode
	  */
	void setCB(Opcode *op_);

	/** Set the current instruction's 8-bit immediate data value
	  */
	void setImmediateData(const unsigned char& d8);
	
	/** Set the current instruction's 16-bit immediate data value
	  */
	void setImmediateData(const unsigned short& d16);

	/** Set the current instruction's immediate data value from an input string.
	  * String is converted to a UChar if instruction expects an 8-bit value and to a UShort if it expects a 16-bit value.
	  */
	void setImmediateData(const std::string &str);

	/** Set opcode label (not used)
	  */
	void setLabel(const std::string& str) {
		sLabel = str;
	}
	
	/** Clock the opcode instruction
	  */
	void clock(){
		nCycles++;
	}
};

class OpcodeHandler {
public:
	OpcodeData lastOpcode; ///< Currently executing opcode

	/** Default constructor
	  */
	OpcodeHandler();

	/** Get a pointer to the currently executing opcode
	  */
	OpcodeData* operator () () {
		return &lastOpcode;
	}

	/** Get a const pointer to the currently executing opcode
	  */
	const OpcodeData* operator () () const {
		return &lastOpcode;
	}

	/** Get the array of all cpu opcodes
	  */
	Opcode* getOpcodes() {
		return opcodes;
	}

	/** Get the array of all cpu opcodes
	  */
	Opcode* getOpcodesCB() {
		return opcodesCB;
	}

	/** Get a pointer to the currently executing opcode
	  */
	Opcode* getCurrentOpcode() {
		return lastOpcode();
	}

	/** Find an opcode with a specified mnemonic
	  * @return True if an opcode with matching mnemonic was found
	  */
	bool findOpcode(const std::string& mnemonic, OpcodeData& data);

	/** Reset the currently executing opcode
	  */
	void resetOpcode(){
		lastOpcode = OpcodeData();
	}

	/** Clock the currently executing opcode
	  * @return True if clocking the current opcode causes it to tick its execute cycle
	  */
	virtual bool clock(LR35902*);

protected:
	std::vector<Opcode> aliases; ///< List of opcode aliases

	Opcode opcodes[256]; ///< Opcode functions for LR35902 processor
	
	Opcode opcodesCB[256]; ///< CB-Prefix opcode functions for LR35902 processor
};

#endif
