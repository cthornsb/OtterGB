#ifndef SYSTEM_REGISTERS_HPP
#define SYSTEM_REGISTERS_HPP

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

	bool getBit(const unsigned char &bit){ return ((value & (0x1 << bit)) == (0x1 << bit)); }

	unsigned char getBits(const unsigned char &lowBit, const unsigned char &highBit) const ;

	unsigned char getValue() const { return value; }

	unsigned char *getPtr(){ return &value; }

	const unsigned char *getConstPtr() const { return (const unsigned char *)&value; }

	unsigned char getReadMask() const { return readBits; }
	
	unsigned char getWriteMask() const { return writeBits; }
	
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

// Pointer to joypad register
extern Register *rJOYP; ///< JOYP (Joypad register)

// Pointers to serial registers
extern Register *rSB; ///< SB (Serial transfer data byte)
extern Register *rSC; ///< SC (Serial transfer control)

// Pointers to timer registers
extern Register *rDIV;  ///< DIV (Divider register)
extern Register *rTIMA; ///< TIMA (Timer counter)
extern Register *rTMA;  ///< TMA (Timer modulo)
extern Register *rTAC;  ///< TAC (Timer control)

// Pointers to DMA registers
extern Register *rDMA;   ///< DMA transfer from ROM/RAM to OAM
extern Register *rHDMA1; ///< New DMA source, high byte (GBC only)
extern Register *rHDMA2; ///< New DMA source, low byte (GBC only)
extern Register *rHDMA3; ///< New DMA destination, high byte (GBC only)
extern Register *rHDMA4; ///< New DMA destination, low byte (GBC only)
extern Register *rHDMA5; ///< New DMA source, length/mode/start (GBC only)

// Pointers to GPU registers
extern Register *rLCDC; ///< LCDC (LCD Control Register)
extern Register *rSTAT; ///< STAT (LCDC Status Register)
extern Register *rSCY;  ///< SCY (Scroll Y)
extern Register *rSCX;  ///< SCX (Scroll X)
extern Register *rLY;   ///< LY (LCDC Y-coordinate) [read-only]
extern Register *rLYC;  ///< LYC (LY Compare)
extern Register *rBGP;  ///< BGP (BG palette data, non-gbc mode only)
extern Register *rOBP0; ///< OBP0 (Object palette 0 data, non-gbc mode only)
extern Register *rOBP1; ///< OBP1 (Object palette 1 data, non-gbc mode only)
extern Register *rWY;   ///< WY (Window Y Position)
extern Register *rWX;   ///< WX (Window X Position (minus 7))
extern Register *rVBK;  ///< VBK (VRAM bank select, gbc mode)
extern Register *rBGPI; ///< BCPS/BGPI (Background palette index, gbc mode)
extern Register *rBGPD; ///< BCPD/BGPD (Background palette data, gbc mode)
extern Register *rOBPI; ///< OCPS/OBPI (Sprite palette index, gbc mode)
extern Register *rOBPD; ///< OCPD/OBPD (Sprite palette index, gbc mode)

// Pointers to sound processor registers
extern Register *rNR10; ///< NR10 ([TONE] Channel 1 sweep register)
extern Register *rNR11; ///< NR11 ([TONE] Channel 1 sound length / wave pattern duty)
extern Register *rNR12; ///< NR12 ([TONE] Channel 1 volume envelope)
extern Register *rNR13; ///< NR13 ([TONE] Channel 1 frequency low)
extern Register *rNR14; ///< NR14 ([TONE] Channel 1 frequency high)
extern Register *rNR20; ///< Not used
extern Register *rNR21; ///< NR21 ([TONE] Channel 2 sound length / wave pattern duty)
extern Register *rNR22; ///< NR22 ([TONE] Channel 2 volume envelope
extern Register *rNR23; ///< NR23 ([TONE] Channel 2 frequency low)
extern Register *rNR24; ///< NR24 ([TONE] Channel 2 frequency high)
extern Register *rNR30; ///< NR30 ([TONE] Channel 3 sound on/off)
extern Register *rNR31; ///< NR31 ([WAVE] Channel 3 sound length)
extern Register *rNR32; ///< NR32 ([WAVE] Channel 3 select output level)
extern Register *rNR33; ///< NR33 ([WAVE] Channel 3 frequency low)
extern Register *rNR34; ///< NR34 ([WAVE] Channel 3 frequency high)
extern Register *rNR40; ///< Not used
extern Register *rNR41; ///< NR41 ([NOISE] Channel 4 sound length)
extern Register *rNR42; ///< NR42 ([NOISE] Channel 4 volume envelope)
extern Register *rNR43; ///< NR43 ([NOISE] Channel 4 polynomial counter)
extern Register *rNR44; ///< NR44 ([NOISE] Channel 4 counter / consecutive, initial)
extern Register *rNR50; ///< NR50 (Channel control / ON-OFF / volume)
extern Register *rNR51; ///< NR51 (Select sound output)
extern Register *rNR52; ///< NR52 (Sound ON-OFF)
extern Register *rWAVE[16]; ///< [Wave] pattern RAM (FF30-FF3F)

// Pointers to system registers	
extern Register *rIF;   ///< Interrupt Flag
extern Register *rKEY1; ///< Speed switch register
extern Register *rRP;   ///< Infrared comms port (not used)
extern Register *rIE;   ///< Interrupt enable
extern Register *rIME;  ///< Master interrupt enable
extern Register *rSVBK; ///< Work RAM bank number

// Pointers to undocumented registers
extern Register *rFF6C;	
extern Register *rFF72;	
extern Register *rFF73;
extern Register *rFF74;	
extern Register *rFF75;	
extern Register *rFF76;	
extern Register *rFF77;

// Gameboy Color flag
extern bool bGBCMODE;

#endif
