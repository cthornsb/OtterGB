#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <string>

class SystemComponent;

class Register{
public:
	/** Default constructor
	  */
	Register() : 
		value(0), 
		readBits(0), 
		writeBits(0), 
		sName(), 
		address(0), 
		comp(0x0) 
	{ 
	}

	/** Named register constructor
	  * @param name Name of register
	  * @param rb Register read mask
	  * @param wb Register write mask
	  */
	Register(const std::string &name, const unsigned char &rb, const unsigned char &wb) : 
		value(0), 
		readBits(rb), 
		writeBits(wb), 
		sName(name), 
		address(0), 
		comp(0x0) 
	{ 
	}

	/** Set read and write bit masks
	  * Masks are used when reading from and writing to register value to prevent access to certain bits.
	  * Input string is expected to have at least 8 numerical characters whose accepted values are described below.
	  *  0: Not readable or writeable
	  *  1: Read-only
	  *  2: Write-only
	  *  3: Readable and writeable
	  * @param masks String containing read/write flags for each of the 8 register bits
	  */
	Register(const std::string &name, const std::string &bits);

	/** Equality operator
	  * @return True if current register value matches input value
	  */
	bool operator == (const unsigned char &rhs) const {
		return (value == rhs); 
	}

	/** Equality operator
	  * @return True if current register value matches input register value
	  */
	bool operator == (const Register &rhs) const {
		return (value == rhs.getValue()); 
	}

	/** Inequality operator
	  * @return True if current register value does not match input value
	  */
	bool operator != (const unsigned char &rhs) const {
		return (value != rhs); 
	}

	/** Inequality operator
	  * @return True if current register value does not match input register value
	  */
	bool operator != (const Register &rhs) const {
		return (value != rhs.getValue()); 
	}

	/** Bitwise OR operator
	  */
	unsigned char operator | (const Register &rhs) const {
		return (value | rhs.getValue()); 
	}
	
	/** Bitwise OR operator
	  */
	unsigned char operator | (const unsigned char& rhs) const {
		return (value | rhs);
	}

	/** Bitwise OR operator
	  */
	unsigned char& operator |= (const unsigned char &rhs) {
		value |= rhs;
		return value; 
	}

	/** Bitwise AND operator
	  */
	unsigned char operator & (const Register &rhs) const {
		return (value & rhs.getValue()); 
	}

	/** Bitwise AND operator
	  */
	unsigned char operator & (const unsigned char& rhs) const {
		return (value & rhs);
	}

	/** Bitwise AND operator
	  */
	unsigned char& operator &= (const unsigned char &rhs) {
		value &= rhs;
		return value;
	}

	/** Assignment operator
	  */
	unsigned char& operator = (const Register &rhs) {
		value = rhs.getValue();
		return value;
	}

	/** Assignment operator
	  */
	unsigned char& operator = (const unsigned char &rhs) {
		value = rhs;
		return value;
	}

	/** Prefix increment operator
	  */
	unsigned char& operator ++ (){ 
		return (++value);
	}

	/** Postfix increment operator
	  */
	unsigned char operator ++ (int){ 
		return (value++); 
	}

	/** Prefix decrement operator
	  */
	unsigned char& operator -- (){ 
		return (--value);
	}

	/** Postfix decrement operator
	  */
	unsigned char operator -- (int){ 
		return (value--);
	}

	/** Get register value masked by the read bitmask
	  */
	unsigned char operator () () const { 
		return (value & readBits); 
	}

	/** Read the value of the register masked by the read bitmask
	  */
	void read(unsigned char &output) const { 
		output = (value & readBits); 
	}
	
	/** Read the value of the register masked by the read bitmask
	  */
	unsigned char read() const { 
		return (value & readBits); 
	}
	
	/** Write a new register value masked by the write bitmask
	  */ 
	unsigned char write(const unsigned char &input){ 
		return (value = (input & writeBits)); 
	}

	/** Test the state of a register bit
	  */
	bool getBit(const unsigned char &bit) const { 
		return ((value & (0x1 << bit)) == (0x1 << bit)); 
	}

	/** Test the state of register bit 0
	  */
	bool bit0() const {
		return ((value & 0x1) == 0x1);
	}

	/** Test the state of register bit 1
	  */
	bool bit1() const {
		return ((value & 0x2) == 0x2);
	}

	/** Test the state of register bit 2
	  */	
	bool bit2() const {
		return ((value & 0x4) == 0x4);
	}

	/** Test the state of register bit 3
	  */	
	bool bit3() const {
		return ((value & 0x8) == 0x8);
	}

	/** Test the state of register bit 4
	  */	
	bool bit4() const {
		return ((value & 0x10) == 0x10);
	}

	/** Test the state of register bit 5
	  */
	bool bit5() const {
		return ((value & 0x20) == 0x20);
	}

	/** Test the state of register bit 6
	  */	
	bool bit6() const {
		return ((value & 0x40) == 0x40);
	}

	/** Test the state of register bit 7
	  */
	bool bit7() const {
		return ((value & 0x80) == 0x80);
	}	

	/** Set register bit 0
	  */	
	void setBit0() {
		value |= 0x1;
	}

	/** Set register bit 1
	  */
	void setBit1() {
		value |= 0x2;
	}

	/** Set register bit 2
	  */
	void setBit2() {
		value |= 0x4;
	}
	
	/** Set register bit 3
	  */
	void setBit3() {
		value |= 0x8;
	}

	/** Set register bit 4
	  */	
	void setBit4() {
		value |= 0x10;
	}

	/** Set register bit 5
	  */
	void setBit5() {
		value |= 0x20;
	}

	/** Set register bit 6
	  */
	void setBit6() {
		value |= 0x40;
	}

	/** Set register bit 7
	  */	
	void setBit7() {
		value |= 0x80;
	}

	/** Reset register bit 0
	  */
	void resetBit0() {
		value &= 0xfe;
	}

	/** Reset register bit 1
	  */
	void resetBit1() {
		value &= 0xfd;
	}

	/** Reset register bit 2
	  */
	void resetBit2() {
		value &= 0xfb;
	}

	/** Reset register bit 3
	  */	
	void resetBit3() {
		value &= 0xf7;
	}

	/** Reset register bit 4
	  */	
	void resetBit4() {
		value &= 0xef;
	}

	/** Reset register bit 5
	  */
	void resetBit5() {
		value &= 0xdf;
	}

	/** Reset register bit 6
	  */
	void resetBit6() {
		value &= 0xbf;
	}
	
	/** Reset register bit 7
	  */
	void resetBit7() {
		value &= 0x7f;
	}

	/** Extract register bits in the range lowBit to highBit (inclusive)
	  */
	unsigned char getBits(const unsigned char &lowBit, const unsigned char &highBit) const ;

	/** Get the current register value, ignoring the read bitmask
	  */
	unsigned char getValue() const { 
		return value; 
	}

	/** Get a pointer to the 8-bit register value
	  */
	unsigned char *getPtr(){ 
		return &value; 
	}

	/** Get a const pointer to the 8-bit register value
	  */
	const unsigned char *getConstPtr() const { 
		return (const unsigned char *)&value; 
	}

	/** Get the register read bitmask
	  */
	unsigned char getReadMask() const { 
		return readBits; 
	}
	
	/** Get the register write bitmask
	  */
	unsigned char getWriteMask() const { 
		return writeBits; 
	}
	
	/** Get the 16-bit register address
	  */
	unsigned short getAddress() const { 
		return address; 
	}
	
	/** Get the human-readable name of the system reegister
	  */
	std::string getName() const { 
		return sName; 
	}
	
	/** Get pointer to associated system component 
	  */
	SystemComponent *getSystemComponent(){ 
		return comp; 
	}

	/** Set the 16-bit register address
	  */
	void setAddress(const unsigned short &addr){ 
		address = addr; 
	}

	/** Set the human-readable name of the system register
	  */
	void setName(const std::string &name){ 
		sName = name; 
	}
	
	/** Set read and write bit masks
	  * Masks are used when reading from and writing to register value to prevent access to certain bits.
	  * Input string is expected to have at least 8 numerical characters whose accepted values are described below.
	  *  0: Not readable or writeable
	  *  1: Read-only
	  *  2: Write-only
	  *  3: Readable and writeable
	  * @param masks String containing read/write flags for each of the 8 register bits
	  */
	void setMasks(const std::string &masks);

	/** Set a single register bit
	  */
	void setBit(const unsigned char &bit);

	/** Set register bits in range lowBit to highBit (inclusive)
	  */
	void setBits(const unsigned char &lowBit, const unsigned char &highBit);

	/** Insert bits into register value in range lowBit to highBit (inclusive)
	  */
	void setBits(const unsigned char &lowBit, const unsigned char &highBit, const unsigned char &value);
	
	/** Reset a single register bit
	  */
	void resetBit(const unsigned char &bit);
	
	/** Reset register bits in range lowBit to highBit (inclusive)
	  */
	void resetBits(const unsigned char &lowBit, const unsigned char &highBit);
	
	/** Set the value of the register, ignoring the write bitmask
	  */
	void setValue(const unsigned char &val){
		value = val; 
	}

	/** Set pointer to associated system component
	  */
	void setSystemComponent(SystemComponent *component){
		comp = component; 
	}
	
	/** Set the value of the register to zero
	  */
	void clear(){
		value = 0x0; 
	}

	/** Return true if the register value is equal to zero
	  */
	bool zero() const {
		return (value == 0); 
	}

	/** Return true if system component address has been set
	  */
	bool set() const {
		return (comp != 0x0); 
	}
	
	/** Get string containing register name, address, and value
	  */
	std::string dump() const ;

private:
	unsigned char value; ///< 8-bit register value
	
	unsigned char readBits; ///< Bitmask used when reading from register
	
	unsigned char writeBits; ///< Bitmask used when writing to register
	
	std::string sName; ///< Human-readable name of register
	
	unsigned short address; ///< 16-bit address of system register
	
	SystemComponent *comp; ///< Pointer to associated system component object
};

#endif

