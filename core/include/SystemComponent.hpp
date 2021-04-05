#ifndef SYSTEM_COMPONENT_HPP
#define SYSTEM_COMPONENT_HPP

#include <vector>
#include <fstream>

class SystemGBC;

class ConfigFile;

class SystemComponent{
public:
	/** Default constructor
	  * No associated RAM banks.
	  */
	SystemComponent(const std::string name="") : 
		sys(0x0), 
		nComponentID(0),
		sName(name), 
		readOnly(0), 
		debugMode(0), 
		verboseMode(0),
		bSaveRAM(1),
		offset(0), 
		nBytes(0), 
		nBanks(0), 
		bs(0), 
		size(0),
		writeLoc(0),
		writeBank(0),
		writeVal(0),
		readLoc(0),
		readBank(0),
		mem(),
		userValues()
	{ 
	}

	/** Named component
	  * @param name Human readable name of system component
	  * @param id System component ID number
	  */
	SystemComponent(const std::string name, const unsigned int& id) : 
		sys(0x0), 
		nComponentID(id),
		sName(name), 
		readOnly(0), 
		debugMode(0), 
		verboseMode(0),
		bSaveRAM(1),
		offset(0), 
		nBytes(0), 
		nBanks(0), 
		bs(0), 
		size(0),
		writeLoc(0),
		writeBank(0),
		writeVal(0),
		readLoc(0),
		readBank(0),
		mem(),
		userValues()
	{ 
	}

	/** Named component with component RAM
	  * @param name Human readable name of system component
	  * @param id System component ID number
	  * @param nB Number of bytes of associated RAM per bank
	  * @param N Number of banks of associated RAM (should be > 1 if component supports bank swapping)
	  * @param off Address offset in the 16-bit system memory map
	  */
	SystemComponent(const std::string name, const unsigned int& id, const unsigned short &nB, const unsigned short &N, const unsigned short& off=0) : 
		sys(0x0), 
		nComponentID(id),
		sName(name), 
		readOnly(0),
		debugMode(0), 
		verboseMode(0),
		bSaveRAM(1), 
		offset(off), 
		nBytes(nB), 
		nBanks(N), 
		bs(0), 
		size(nB*N),
		writeLoc(0),
		writeBank(0),
		writeVal(0),
		readLoc(0),
		readBank(0),
		mem(N, std::vector<unsigned char>(nB, 0x0)),
		userValues()
	{
	}

	/** Destructor
	  */
	~SystemComponent();

	/** Method called when emulator system is quitting
	  * Does nothing by default.
	  */
	virtual void onExit(){ }

	/** Method called once per 1 MHz system clock tick
	  * Should return true if clocking the component triggers additional actions (e.g. component state changes).
	  * Returns false by default.
	  */
	virtual bool onClockUpdate(){ 
		return false; 
	}

	/** Set a pointer to the emulator system bus , define associated system registers, and add all savestate input/output values
	  */
	void connectSystemBus(SystemGBC *bus);
	
	/** Initialize component RAM banks
	  * Banks are initially filled with zeros. 
	  */
	void initialize(const unsigned short &nB, const unsigned short &N=1);
	
	/** Return true if component has no associated RAM
	  */
	bool empty() const { 
		return mem.empty(); 
	}
	
	/** Attempt to write to an address in component RAM (using the current memory bank select, if enabled)
	  * If memory is set to read only or if preWriteAction() fails, return false and do not write.
	  * @note If NOT built with debugging flag set, no validity checking is performed and undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param src Pointer to value to write
	  * @return True if the value was successfully written and return false otherwise
	  */
	bool write(const unsigned short &loc, const unsigned char *src);

	/** Attempt to write to an address in component RAM (using the current memory bank select, if enabled)
	  * If memory is set to read only or if preWriteAction() fails, return false and do not write.
	  * @note If NOT built with debugging flag set, no validity checking is performed and undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param src Value to write
	  * @return True if the value was successfully written and return false otherwise
	  */	
	bool write(const unsigned short &loc, const unsigned char &src);

	/** Attempt to write to an address in component RAM using explicit memory bank select
	  * If memory is set to read only or if preWriteAction() fails, return false and do not write.
	  * @note If NOT built with debugging flag set, no validity checking is performed and undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param bank Component RAM bank select number (indexed from zero)
	  * @param src Pointer to value to write
	  * @return True if the value was successfully written and return false otherwise
	  */
	bool write(const unsigned short &loc, const unsigned short &bank, const unsigned char *src); 

	/** Attempt to write to an address in component RAM using explicit memory bank select
	  * If memory is set to read only or if preWriteAction() fails, return false and do not write.
	  * @note If NOT built with debugging flag set, no validity checking is performed and undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param bank Component RAM bank select number (indexed from zero)
	  * @param src Value to write
	  * @return True if the value was successfully written and return false otherwise
	  */
	bool write(const unsigned short &loc, const unsigned short &bank, const unsigned char &src);

	/** Blindly write to an address in component RAM (using the current memory bank select, if enabled)
	  * If memory is set to read only, do not write. No other validity checks are performed, undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param src Value to write
	  */
	void writeFast(const unsigned short &loc, const unsigned char &src);

	/** Blindly write to an address in component RAM bank 0
	  * If memory is set to read only, do not write. No other validity checks are performed, undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param src Value to write
	  */
	void writeFastBank0(const unsigned short &loc, const unsigned char &src);

	/** Attempt to read from an address in component RAM (using the current memory bank select, if enabled)
	  * If preReadAction() fails, return false and do not read.
	  * @note If NOT built with debugging flag set, no validity checking is performed and undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param dest Pointer to read value into
	  * @return True if the value was successfully read and return false otherwise
	  */
	bool read(const unsigned short &loc, unsigned char *dest);

	/** Attempt to read from an address in component RAM (using the current memory bank select, if enabled)
	  * If preReadAction() fails, return false and do not read.
	  * @note If NOT built with debugging flag set, no validity checking is performed and undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param dest Reference to read value into
	  * @return True if the value was successfully read and return false otherwise
	  */
	bool read(const unsigned short &loc, unsigned char &dest);

	/** Attempt to read from an address in component RAM using explicit memory bank select
	  * If preReadAction() fails, return false and do not read.
	  * @note If NOT built with debugging flag set, no validity checking is performed and undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param bank Component RAM bank select number (indexed from zero)
	  * @param dest Pointer to read value into
	  * @return True if the value was successfully read and return false otherwise
	  */
	bool read(const unsigned short &loc, const unsigned short &bank, unsigned char *dest);

	/** Attempt to read from an address in component RAM using explicit memory bank select
	  * If preReadAction() fails, return false and do not read.
	  * @note If NOT built with debugging flag set, no validity checking is performed and undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param bank Component RAM bank select number (indexed from zero)
	  * @param dest Reference to read value into
	  * @return True if the value was successfully read and return false otherwise
	  */
	bool read(const unsigned short &loc, const unsigned short &bank, unsigned char &dest);

	/** Blindly read an address in component RAM (using the current memory bank select, if enabled)
	  * No validity checks are performed, undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param src Reference to read into
	  */
	void readFast(const unsigned short &loc, unsigned char &dest);

	/** Blindly read an address in component RAM bank 0
	  * No validity checks are performed, undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param src Reference to read into
	  */
	void readFastBank0(const unsigned short &loc, unsigned char &dest);

	/** Select the RAM bank which will be used for following calls to read / write
	  */
	void setBank(const unsigned short &b){ 
		bs = (b < nBanks ? b : b-1); 
	}

	/** Set the address component memory offset in the 16-bit system memory map
	  */
	void setOffset(const unsigned short &off){ 
		offset = off; 
	}

	/** Set the name of the component
	  */
	void setName(const std::string &name){ 
		sName = name; 
	}

	/** Enable writing (reading) component RAM contents to quicksave
	  */
	void enableSaveRAM(){
		bSaveRAM = true;
	}
	
	/** Disable writing (reading) component RAM contents to quicksave
	  */
	void disableSaveRAM(){
		bSaveRAM = false;
	}

	/** Print component RAM contents to stdout
	  */
	void print(const unsigned short bytesPerRow=10);

	/** Write component RAM contents to output stream
	  * A total of (nBanks * nBytes) will be written to the output stream.
	  * @return The number of bytes written to output stream
	  */
	unsigned int writeMemoryToFile(std::ofstream &f);

	/** Read component RAM contents from input stream
	  * Attempt to read a total of (nBanks * nBytes) from the input stream.
	  * @return The number of bytes read from input stream
	  */
	unsigned int readMemoryFromFile(std::ifstream &f);

	/** Write component state to output savestate file
	  * Peform the following actions
	  *  Write component header containing length of component RAM and current bank select.
	  *  writeUserSavestate() is called to write extra state information for derived classes.
	  *  If bSaveRAM is true, entire component RAM map is written.
	  * @return The number of bytes written to output stream.
	  */
	unsigned int writeSavestate(std::ofstream &f);

	/** Read component state from output savestate file
	  * Peform the following actions
	  *  Read component header containing length of component RAM and current bank select.
	  *  readUserSavestate() is called to read extra state information for derived classes.
	  *  If bSaveRAM is true, entire component RAM map is read.
	  * @return The number of bytes read from input stream.
	  */
	unsigned int readSavestate(std::ifstream &f);

	/** Check that this system component is sensitive to the specified register address
	  * May be used to prevent certain registers being written to (e.g. if the component is currently powered off).
	  * Should return true if register may be written to or read from.
	  * @param reg 16-bit register address (will always be in the range 0xff00 to 0xff80)
	  * @return True by default (register may be written to or read from)
	  */
	virtual bool checkRegister(const unsigned short &reg){ 
		return true; 
	}
	
	/** Write to a component register
	  * Should return true if component is sensitive to the register.
	  * @param reg 16-bit register address (will always be in the range 0xff00 to 0xff80)
	  * @param val New register value being written
	  * @return False by default
	  */
	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val){ 
		return false; 
	}
	
	/** Read a component register
	  * Should return true if component is sensitive to the register.
	  * @param reg 16-bit register address (will always be in the range 0xff00 to 0xff80)
	  * @param val Reference to read register value into
	  * @return False by default
	  */
	virtual bool readRegister(const unsigned short &reg, unsigned char &val){ 
		return false; 
	}

	/** Get pointer to a location in component RAM (using the current memory bank select, if enabled)
	  * If component RAM is read-only, null will be returned. No other validity checks performed, undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  */
	virtual unsigned char *getPtr(const unsigned short &loc){ 
		return (!readOnly ? &mem[bs][loc-offset] : 0x0); 
	}

	/** Get a const pointer to a location in component RAM (using the current memory bank select, if enabled)
	  * No validity checks are performed, undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  */
	virtual const unsigned char *getConstPtr(const unsigned short &loc){ 
		return (const unsigned char *)&mem[bs][loc-offset]; 
	}

	/** Read settings from an input user configuration file
	  */
	virtual void readConfigFile(ConfigFile*) { }

	/** Get pointer to a location in component RAM
	  * No validity checks are performed, undefined behavior may result.
	  * @param loc 16-bit Address in system memory map. Memory offset is automatically subtracted from input address
	  * @param bank Component RAM bank select number (indexed from zero)
	  */
	unsigned char *getPtr(const unsigned short &loc, const unsigned short &bank){ 
		return &mem[bank][loc-offset]; 
	}

	/** Get pointer to the beginning of the specified RAM bank or null pointer if the bank doesn't exist
	  * @param bank Component RAM bank select number (indexed from zero)
	  */
	unsigned char *getPtrToBank(const unsigned short &bank){ 
		return (bank < nBanks ? mem[bank].data() : 0x0); 
	}

	/** Get the size of associated component RAM (in bytes)
	  */
	unsigned int getSize() const { 
		return size; 
	}

	/** Get the current RAM bank select
	  * Always returns zero if component contains only one RAM bank.
	  */
	unsigned short getBankSelect() const { 
		return bs; 
	}

	/** Get the human-readable name of the system component
	  */
	std::string getName() const { 
		return sName; 
	}

	/** Enable or disable debug mode
	  */ 
	void setDebugMode(bool state=true){ 
		debugMode = state; 
	}
	
	/** Enable or disable verbose output mode
	  */
	void setVerboseMode(bool state=true){ 
		verboseMode = state; 
	}

	/** Reset system component to initial conditions
	  * Clear component memory and call onUserReset().
	  */
	void reset();

protected:
	SystemGBC *sys; ///< Pointer to the system bus

	unsigned int nComponentID; ///< 4-byte component identifier

	std::string sName; ///< Human-readable name of component
	
	bool readOnly; ///< Read-only RAM flag
	
	bool debugMode; ///< Debug flag
	
	bool verboseMode; ///< Verbosity flag
	
	bool bSaveRAM; ///< If enabled, entire contents of component RAM will be written to / read from savestate file

	unsigned short offset; ///< Memory address offset
	
	unsigned short nBytes; ///< Number of bytes per bank
	
	unsigned short nBanks; ///< Number of banks
	
	unsigned short bs; ///< Bank select

	unsigned int size; ///< Total size of memory block

	unsigned short writeLoc; ///<  Location in memory to be written to
	
	unsigned short writeBank; ///< Bank of memory to be written to
	
	unsigned char writeVal; ///< Value to be written to location (writeLoc)

	unsigned short readLoc; ///< Location in memory to be read from
	
	unsigned short readBank; ///< Bank of memory to be read from

	std::vector<std::vector<unsigned char> > mem; ///< Physical memory

	std::vector<std::pair<void*, unsigned int> > userValues;

	bool setReadOnly(bool state=true){ 
		return (readOnly = state); 
	}

	bool toggleReadOnly(){ 
		return (readOnly = !readOnly); 
	}

	void addSavestateValue(void* ptr, const unsigned int& len){
		userValues.push_back(std::make_pair(ptr, len));
	}

	virtual bool preWriteAction(){ 
		return true; 
	}
	
	virtual bool preReadAction(){ 
		return true; 
	}

	/** Initialization method called when the system requests system registers from the component
	  * SystemGBC::addSystemRegister should be called here for every register related to the component.
	  * Does nothing by default
	  */
	virtual void defineRegisters(){ 
	}
	
	/** Add elements to a list of values which will be written to / read from an emulator savestate
	  */
	virtual void userAddSavestateValues(){
	}
	
	/** Write 13 byte component header 
	  * Perform the following actions
	  *  Write 4 byte system component identifier number
	  *  Write 1 byte RAM read-only flag (0xff read-only, 0x0 normal)
	  *  Write 2 byte RAM memory address offset
	  *  Write 2 byte RAM bank size (in bytes)
	  *  Write 2 byte RAM bank number
	  *  Write 2 byte RAM bank select
	  */
	unsigned int writeSavestateHeader(std::ofstream &f);

	/** Read 13 byte component header 
	  * Perform the following actions
	  *  Read 4 byte system component identifier number
	  *  Read 1 byte RAM read-only flag (0xff read-only, 0x0 normal)
	  *  Read 2 byte RAM memory address offset
	  *  Read 2 byte RAM bank size (in bytes)
	  *  Read 2 byte RAM bank number
	  *  Read 2 byte RAM bank select
	  */
	unsigned int readSavestateHeader(std::ifstream &f);
	
	/** Reset component memory
	  */
	void resetMemory();
	
	/** Handle call to reset system component
	  * Called from reset().
	  */
	virtual void onUserReset() { }
};

#endif
