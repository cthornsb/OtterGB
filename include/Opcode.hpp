#ifndef OPCODE_HPP
#define OPCODE_HPP

#include <string>

extern const unsigned char FLAG_Z_BIT;
extern const unsigned char FLAG_S_BIT;
extern const unsigned char FLAG_H_BIT;
extern const unsigned char FLAG_C_BIT;

extern const unsigned char FLAG_Z_MASK;
extern const unsigned char FLAG_S_MASK;
extern const unsigned char FLAG_H_MASK;
extern const unsigned char FLAG_C_MASK;
#ifdef PROJECT_GBC
class LR35902;

typedef unsigned short (LR35902::*addrGetFunc)() const;

class Opcode{
public:
	void (LR35902::*ptr)(); ///< Pointer to the instruction code.

	addrGetFunc addrptr; ///< Pointer to the function which returns the memory address.
#else
class Opcode {
public:
#endif // ifdef PROJECT_GBC	
	unsigned char nType;

	unsigned short nCycles; ///< Length of clock cycles required.
	unsigned short nBytes; ///< Length of instruction in bytes.

	unsigned short nReadCycles; ///< Flag indicating this instruction reads from memory.
	unsigned short nWriteCycles; ///< Flag indicating this instruction writes to memory.

	std::string sName; ///< The full instruction mnemonic.

	std::string sPrefix; ///< 
	std::string sSuffix; ///< 

	std::string sOpname; ///< Opcode name.
	std::string sOperands[2]; ///< Operand names.
	
	Opcode() : 
#ifdef PROJECT_GBC	
		ptr(0x0), 
		addrptr(0x0), 
#endif // ifdef PROJECT_GBC	
		nType(0),
		nCycles(0), 
		nBytes(0), 
		nReadCycles(0), 
		nWriteCycles(0), 
		sName() 
	{ 
	}
#ifdef PROJECT_GBC
	Opcode(LR35902 *cpu, const std::string &mnemonic, const unsigned short &cycles, const unsigned short &bytes, const unsigned short &read, const unsigned short &write, void (LR35902::*p)());
#else
	Opcode(const std::string& mnemonic, const unsigned short& cycles, const unsigned short& bytes, const unsigned short& read, const unsigned short& write);
#endif // ifdef PROJECT_GBC	
	bool check(const std::string& op, const unsigned char& type, const std::string& arg1="", const std::string& arg2="") const ;
};

class OpcodeData{
public:
	Opcode *op; ///< Pointer to the opcode instruction.
	
	unsigned char nIndex; ///< Opcode index.
	unsigned short nData; ///< Immediate data.
	unsigned short nPC; ///< Program counter.
	unsigned short nCycles; ///< Clock cycles since start of instruction
	unsigned short nExtraCycles; ///< 
	unsigned short nReadCycle;
	unsigned short nWriteCycle;
	unsigned short nExecuteCycle; ///< Clock cycle on which to execute instruction
	
	std::string sLabel; ///< String representing an assembler label or variable name

	bool cbPrefix; ///< CB Prefix opcodes.
	
	OpcodeData();
	
	Opcode * operator () () { return op; }
	
	bool executing() const { return (nCycles < (nExecuteCycle + nExtraCycles)); }
#ifdef PROJECT_GBC		
	bool clock(LR35902 *cpu);
#endif // ifdef PROJECT_GBC	
	bool onRead() const { return (nCycles == nReadCycle); }
	
	bool onWrite() const { return (nCycles == nWriteCycle); }

	bool onExecute() const { return (nCycles == nExecuteCycle); }

	bool onOvertime() const { return (nCycles > nExecuteCycle); }

	bool memoryAccess() const { return (nReadCycle || nWriteCycle); }

	unsigned short cyclesRemaining() const { return (nExecuteCycle + nExtraCycles - nCycles); }

	std::string getInstruction() const ;
	
	std::string getShortInstruction() const ; 
	
	void addCycles(const unsigned short &extra){ nExtraCycles = extra; }
	
	unsigned char getd8() const { return (unsigned char)nData; }
	
	unsigned short getd16() const { return nData; }
	
	void set(Opcode *opcodes_, const unsigned char &index_, const unsigned short &pc_);
	
	void set(Opcode *op_);
	
	void setCB(Opcode *opcodes_, const unsigned char &index_, const unsigned short &pc_);

	void setCB(Opcode *op_);

	void setImmediateData(const unsigned char& d8);
	
	void setImmediateData(const unsigned short& d16);

	void setImmediateData(const std::string &str);

	void setLabel(const std::string& str) { sLabel = str; }
};

class OpcodeHandler {
public:
	OpcodeHandler();

	Opcode* getOpcodes() { return opcodes; }

	Opcode* getOpcodesCB() { return opcodesCB; }

	bool findOpcode(const std::string& mnemonic, OpcodeData& data);

private:
	Opcode opcodes[256]; ///< Opcode functions for LR35902 processor
	Opcode opcodesCB[256]; ///< CB-Prefix opcode functions for LR35902 processor

	std::vector<Opcode> aliases; ///< List of opcode aliases 

	/** Initialize opcodes
	  */
	void initialize();
};

#endif