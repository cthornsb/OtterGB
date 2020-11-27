#ifndef SYSTEM_COMPONENT_HPP
#define SYSTEM_COMPONENT_HPP

#include <vector>
#include <fstream>

class SystemGBC;

class SystemComponent{
public:
	SystemComponent() : sys(0x0), readOnly(0), debugMode(0), offset(0), nBytes(0), nBanks(0), bs(0), size(0) { }

	SystemComponent(const unsigned short &nB, const unsigned short &N=1) : sys(0x0), readOnly(0), offset(0), nBytes(nB), nBanks(N), bs(0), size(nB*N) {
		initialize(nB, N);
	}

	SystemComponent(const unsigned short &nB, const unsigned short &off, const unsigned short &N) : sys(0x0), readOnly(0), offset(off), nBytes(nB), nBanks(N), bs(0), size(nB*N) {
		initialize(nB, N);
	}

	~SystemComponent();

	virtual void onExit(){ }

	virtual bool onClockUpdate(){ return false; }

	virtual void defineRegisters(){ }

	void connectSystemBus(SystemGBC *bus){ sys = bus; }
	
	void initialize(const unsigned short &nB, const unsigned short &N=1);
	
	bool write(const unsigned short &loc, unsigned char *src);
	
	bool write(const unsigned short &loc, const unsigned char &src);

	bool write(const unsigned short &loc, const unsigned short &bank, const unsigned char *src); 

	bool write(const unsigned short &loc, const unsigned short &bank, const unsigned char &src);

	bool read(const unsigned short &loc, unsigned char *dest);

	bool read(const unsigned short &loc, unsigned char &dest);

	bool read(const unsigned short &loc, const unsigned short &bank, unsigned char *dest);

	bool read(const unsigned short &loc, const unsigned short &bank, unsigned char &dest);

	void setBank(const unsigned short &b){ bs = (b < nBanks ? b : b-1); }

	void setOffset(const unsigned short &off){ offset = off; }

	void print(const unsigned short bytesPerRow=10);

	unsigned int writeMemoryToFile(std::ofstream &f);

	unsigned int readMemoryFromFile(std::ifstream &f);

	virtual unsigned int writeSavestate(std::ofstream &f);

	virtual unsigned int readSavestate(std::ifstream &f);

	/** Check that this system component is sensitive to the specified
	  * register address. May be used to prevent certain registers being
	  * written to (e.g. if the component is currently powered off).
	  * @param reg Register address.
	  * @return True if the register location is valid and return false otherwise.
	  */
	virtual bool checkRegister(const unsigned short &reg){ return true; }

	// Return true if this component is sensitive to the register (reg)
	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val){ return false; }
	
	// Return true if this component is sensitive to the register (reg)
	virtual bool readRegister(const unsigned short &reg, unsigned char &val){ return false; }

	// Do not allow direct write access if in read-only mode.
	virtual unsigned char *getPtr(const unsigned short &loc){ return (!readOnly ? &mem[bs][loc-offset] : 0x0); }

	virtual const unsigned char *getConstPtr(const unsigned short &loc){ return (const unsigned char *)&mem[bs][loc-offset]; }

	unsigned char *getPtr(const unsigned short &loc, const unsigned short &b){ return &mem[b][loc-offset]; }

	unsigned char *getPtrToBank(const unsigned short &b){ return (b < nBanks ? mem[b].data() : 0x0); }

	unsigned int getSize() const { return size; }

	unsigned short getBankSelect() const { return bs; }

	void setDebugMode(bool state=true){ debugMode = state; }

protected:
	SystemGBC *sys; // Pointer to the system bus
	
	bool readOnly; // Read-only flag
	bool debugMode; // Debug flag

	unsigned short offset; // Memory address offset
	unsigned short nBytes; // Number of bytes per bank
	unsigned short nBanks; // Number of banks
	unsigned short bs; // Bank select

	unsigned int size; // Total size of memory block

	unsigned short writeLoc; //  Location in memory to be written to
	unsigned short writeBank; // Bank of memory to be written to
	unsigned char writeVal; // Value to be written to location (writeLoc)

	unsigned short readLoc; // Location in memory to be read from
	unsigned short readBank; // Bank of memory to be read from

	std::vector<std::vector<unsigned char> > mem; // Memory blocks

	bool setReadOnly(bool state=true){ return (readOnly = state); }

	bool toggleReadOnly(){ return (readOnly = !readOnly); }

	virtual bool preWriteAction(){ return true; }
	
	virtual bool preReadAction(){ return true; }

	virtual void postWriteAction(){ }

	virtual void postReadAction(){ }
	
	void writeSavestateHeader(std::ofstream &f);

	void readSavestateHeader(std::ifstream &f);
};

#endif
