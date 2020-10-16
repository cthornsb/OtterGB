#ifndef SYSTEM_COMPONENT_HPP
#define SYSTEM_COMPONENT_HPP

#include <vector>

class SystemGBC;

class SystemComponent{
public:
	SystemComponent() : sys(0x0), readOnly(0), debugMode(false), offset(0), nBytes(0), nBanks(0), bs(0), size(0), labs(0), lrel(0) { }

	SystemComponent(const unsigned int &nB, const unsigned int &N=1) : readOnly(0), offset(0), nBytes(nB), nBanks(N), bs(0) {
		initialize(nB, N);
	}

	SystemComponent(const unsigned int &nB, const unsigned int &off, const unsigned int &N) : readOnly(0), offset(off), nBytes(nB), nBanks(N), bs(0) {
		initialize(nB, N);
	}

	void connectSystemBus(SystemGBC *bus){ sys = bus; }
	
	void initialize(const unsigned int &nB, const unsigned int &N=1);
	
	~SystemComponent();
	
	bool write(const unsigned int &loc, unsigned char *src);
	
	bool write(const unsigned int &loc, const unsigned char &src);

	bool write(const unsigned int &loc, const unsigned int &bank, const unsigned char *src); 

	bool write(const unsigned int &loc, const unsigned int &bank, const unsigned char &src);

	void writeNext(unsigned char *src);
	
	void writeNext(const unsigned char &src);

	bool read(const unsigned int &loc, unsigned char *dest);

	bool read(const unsigned int &loc, unsigned char &dest);

	bool read(const unsigned int &loc, const unsigned int &bank, unsigned char *dest);

	bool read(const unsigned int &loc, const unsigned int &bank, unsigned char &dest);

	void readNext(unsigned char *src);
	
	void readNext(unsigned char &src);

	bool readABS(const unsigned int &loc, unsigned char *dest);
	
	bool readABS(const unsigned int &loc, unsigned char &dest);

	void setBank(const unsigned int &b){ bs = (b < nBanks ? b : b-1); }

	void setOffset(const unsigned int &off){ offset = off; }

	void print(const unsigned int bytesPerRow=10){ }

	// Do not allow direct write access if in read-only mode.
	virtual unsigned char *getPtr(const unsigned int &loc){ return (!readOnly ? &mem[bs][loc-offset] : 0x0); }

	virtual const unsigned char *getConstPtr(const unsigned int &loc){ return (const unsigned char *)&mem[bs][loc-offset]; }

	unsigned char *getPtr(const unsigned int &loc, const unsigned int &b){ return &mem[b][loc-offset]; }

	unsigned char *getPtrToBank(const unsigned int &b){ return (b < nBanks ? mem[b].data() : 0x0); }

	unsigned int getSize() const { return size; }

	void setDebugMode(bool state=true){ debugMode = state; }

	void incL();

	void decL();

protected:
	SystemGBC *sys; // Pointer to the system bus
	
	bool readOnly; // Read-only flag
	bool debugMode; // Debug flag

	unsigned int offset; // Memory address offset
	unsigned int nBytes; // Number of bytes per bank
	unsigned int nBanks; // Number of banks
	unsigned int bs; // Bank select

	unsigned int size; // Total size of memory block

	unsigned int labs; // Current absolute location in memory block
	unsigned int lrel; // Current location in memory bank

	unsigned int writeLoc; //  Location in memory to be written to
	unsigned int writeBank; // Bank of memory to be written to
	unsigned char writeVal; // Value to be written to location (writeLoc)

	unsigned int readLoc; // Location in memory to be read from
	unsigned int readBank; // Bank of memory to be read from

	std::vector<std::vector<unsigned char> > mem; // Memory blocks

	bool setReadOnly(bool state=true){ return (readOnly = state); }

	bool toggleReadOnly(){ return (readOnly = !readOnly); }

	virtual bool preWriteAction(){ return true; }
	
	virtual bool preReadAction(){ return true; }

	virtual void postWriteAction(){ }

	virtual void postReadAction(){ }
	
	// Return true if this component is sensitive to the register (reg)
	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val){ return false; }
	
	// Return true if this component is sensitive to the register (reg)
	virtual bool readRegister(const unsigned short &reg, unsigned char &val){ return false; }
};

#endif
