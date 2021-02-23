#ifndef COLOR_PALETTE_HPP
#define COLOR_PALETTE_HPP

#include <fstream>

#include "colors.hpp"

class ColorPalette{
public:
	/** Default constructor
	  */
	ColorPalette() :
		trueColor{ Colors::WHITE, Colors::LTGRAY, Colors::DKGRAY, Colors::BLACK }
	{
	}

	/** Get a reference to one of the four palette colors
	  */
	ColorRGB& operator [] (const unsigned char& index){
		return trueColor[index];
	}

	/** Get a const reference to one of the four palette colors
	  */
	ColorRGB operator [] (const unsigned char& index) const {
		return trueColor[index];
	}

	/** Set all four palette colors in 5-bit RGB components (0 to 31)
	  * Array must contain AT LEAST 12 elements {R0, G0, B0, ... , R3, G3, B3}.
	  */
	void setColors(const unsigned char* components);
	
	/** Set all four palette colors in RGB components (0 to 1)
	  * Array must contain AT LEAST 12 elements {R0, G0, B0, ... , R3, G3, B3}.
	  */
	void setColors(const float* components);

	/** Set all four palette colors
	  * Each integer contains red color component in bits 0-4, green in 5-9, and blue in 10-14.
	  */
	void setColors(const unsigned int& c0, const unsigned int& c1, const unsigned int& c2, const unsigned int& c3);
	
	/** Set all four palette colors
	  */
	void setColors(const ColorRGB& c0, const ColorRGB& c1, const ColorRGB& c2, const ColorRGB& c3);
	
	/** Set a single palette color
	  * @param index Color index (0-3)
	  * @param color Contains red color component in bits 0-4, green in 5-9, and blue in 10-14
	  */
	void set(const unsigned char& index, const unsigned int& color);
	
	/** Set a single palette color
	  * @param index Color index (0-3)
	  * @param color Real RGB color
	  */
	void set(const unsigned char& index, const ColorRGB& color);
	
	/** Write a color palette to a binary output file stream
	  * Will attempt to write 12 bytes to output file
	  */
	bool write(std::ofstream& f);
	
	/** Read a color palette from a binary input file stream
	  * Will attempt to read 12 bytes from input file
	  */
	bool read(std::ifstream& f);

	/** Convert input integer into RGB color components
	  * @param input Integer with color components stored in bits 0-4, green in 5-9, and blue in 10-14 respectively.
	  */
	static void convert(const unsigned int& input, unsigned char& r, unsigned char& g, unsigned char& b);

	/** Break input RGB color into 5-bit RGB color components
	  */	
	static void convert(const ColorRGB& input, unsigned char& r, unsigned char& g, unsigned char& b);

	/** Convert input integer into RGB color
	  * @param input Integer with color components stored in bits 0-4, green in 5-9, and blue in 10-14 respectively.
	  */
	static ColorRGB convert(const unsigned int& input);

	/** Convert input RGB components to RGB color
	  * All color components are restricted to 5-bits (0-31)
	  */
	static ColorRGB convert(const unsigned char& r, const unsigned char& g, const unsigned char& b);
	
	/** Convert input integer into RGB color component
	  * @param input 5-bit RGB color component (0-31)
	  */
	static float convert(const unsigned char& input);
	
	/** Convert input float into RGB color component
	  * @param input RGB color component clamped to the range 0 to 1
	  */
	static unsigned char convert(const float& input);
	
	/** Clamp input floating point value to between 0 and 1
	  */
	static float clamp(const float& input);
	
private:
	ColorRGB trueColor[4]; ///< Real RGB palette colors
};

class ColorPaletteDMG{
public:
	ColorPaletteDMG() :
		palettes(),
		nTableNumber(0),
		nEntryNumber(0)
	{
	}

	/** Equality operator
	  */
	bool operator == (const unsigned short& id) const {
		return (id == getPaletteID());
	}

	/** Get a reference to one of the three DMG color palettes
	  */
	ColorPalette& operator [] (const unsigned char& index){
		return palettes[index];
	}

	/** Get a const reference to one of the three DMG color palettes
	  */
	ColorPalette operator [] (const unsigned char& index) const {
		return palettes[index];
	}

	/** Get the CGB palette table number
	  */
	unsigned char getPaletteTable() const {
		return nTableNumber;
	}

	/** Get the CGB palette entry number
	  */
	unsigned char getPaletteEntry() const {
		return nEntryNumber;
	}
	
	/** Get the CGB palette ID number
	  */
	unsigned short getPaletteID() const {
		return (nTableNumber + ((nEntryNumber & 0xffff) << 8));
	}

	/** Set CGB palette table and entry number
	  */
	void setPaletteID(const unsigned char& table, const unsigned char& entry);

	/** Write a DMG color palette to a binary output file stream
	  * Will attempt to write 38 bytes to output file
	  */
	bool write(std::ofstream& f);
	
	/** Read a DMG color palette from a binary input file stream
	  * Will attempt to read 38 bytes from input file
	  */
	bool read(std::ifstream& f);
	
	/** Search for a DMG color palette in a list pre-defined palettes
	  * @param fname Filename of binary file containing DMG color palettes
	  * @return True if pre-defined palette (id) is found in the input file
	  */
	bool find(const std::string& fname, const unsigned short& id, bool verbose=false);

private:
	ColorPalette palettes[3]; ///< DMG color palettes (0: BG/WIN, 1: OBJ0, 2: OBJ1)

	unsigned char nTableNumber; ///< Color palette table number
	
	unsigned char nEntryNumber; ///< Color palette entry number
	
	/** Read table and entry number of DMG palette (two bytes)
	  */
	bool readHeader(std::ifstream& f);

	/** Read all three DMG color palettes (36 bytes)
	  */
	bool readPalettes(std::ifstream& f);
};

#endif

