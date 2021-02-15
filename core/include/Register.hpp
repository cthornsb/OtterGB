#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <string>

class SystemComponent;

class Register{
public:
	Register() : value(0), valueBuffer(0), readBits(0), writeBits(0), sName(), address(0), comp(0x0) { }

	Register(const std::string &name, const unsigned char &rb, const unsigned char &wb) : value(0), valueBuffer(0), readBits(rb), writeBits(wb), sName(name), address(0), comp(0x0) { }

	Register(const std::string &name, const std::string &bits);

	bool operator == (const unsigned char &other) const { return (value == other); }

	bool operator == (const Register &other) const { return (value == other.getValue()); }

	bool operator != (const unsigned char &other) const { return (value != other); }

	bool operator != (const Register &other) const { return (value != other.getValue()); }

	unsigned char operator | (const Register &other) const { return (value | other.getValue()); }

	unsigned char operator |= (const unsigned char &other) { return (value |= other); }

	unsigned char operator & (const Register &other) const { return (value & other.getValue()); }

	unsigned char operator &= (const unsigned char &other) { return (value &= other); }

	unsigned char operator = (const unsigned char &other) { return (value = other); }

	/** Prefix operator
	  */
	unsigned char operator ++ (){ return (value += 1); }

	/** Postfix operator
	  */
	unsigned char operator ++ (int){ value += 1; return (value-1); }

	unsigned char operator () () const { return (value & readBits); }

	void read(unsigned char &output) const { output = (value & readBits); }
	
	unsigned char read() const { return (value & readBits); }
	
	/** Write a new register value immediately, ignoring the value buffer.
	  * @param input The new register value.
	  * @return The new write-masked register value.
	  */ 
	unsigned char write(const unsigned char &input){ return (value = (input & writeBits)); }

	/** Prepare the register for a new value by writing to the value buffer.
	  * The buffered value will not be written to the register until write()
	  * is called.
	  * @param input The value to write to the register buffer.
	  * @return The new write-masked register buffer value.
	  */
	unsigned char buffer(const unsigned char &input){ return (valueBuffer = (input & writeBits)); }

	/** Write the value buffer to the register.
	  * @return The new register value.
	  */
	unsigned char write(){ return (value = valueBuffer); }

	bool getBit(const unsigned char &bit) const { return ((value & (0x1 << bit)) == (0x1 << bit)); }

	unsigned char getBits(const unsigned char &lowBit, const unsigned char &highBit) const ;

	unsigned char getValue() const { return value; }

	unsigned char *getPtr(){ return &value; }

	const unsigned char *getConstPtr() const { return (const unsigned char *)&value; }

	unsigned char getReadMask() const { return readBits; }
	
	unsigned char getWriteMask() const { return writeBits; }
	
	unsigned short getAddress() const { return address; }
	
	std::string getName() const { return sName; }
	
	SystemComponent *getSystemComponent(){ return comp; }

	void setAddress(const unsigned short &addr){ address = addr; }

	void setName(const std::string &name){ sName = name; }
	
	void setMasks(const std::string &masks);

	void setBit(const unsigned char &bit);

	void setBits(const unsigned char &lowBit, const unsigned char &highBit);

	void setBits(const unsigned char &lowBit, const unsigned char &highBit, const unsigned char &value);
	
	void resetBit(const unsigned char &bit);
	
	void resetBits(const unsigned char &lowBit, const unsigned char &highBit);
	
	void setValue(const unsigned char &val){ value = val; }

	void setSystemComponent(SystemComponent *component){ comp = component; }
	
	void clear(){ value = 0x0; }

	bool zero() const { return (value == 0); }

	bool set() const { return (comp != 0x0); }
	
	std::string dump() const ;

private:
	unsigned char value;
	unsigned char valueBuffer;

	unsigned char readBits;
	unsigned char writeBits;
	
	std::string sName;
	
	unsigned short address;
	
	SystemComponent *comp;
};

#endif

