#ifndef COLOR_PALETTE_HPP
#define COLOR_PALETTE_HPP

#include <fstream>
#include <vector>

#include "ColorRGB.hpp"

class ColorPalette{
public:
	/** Default constructor
	  */
	ColorPalette() :
		trueColor{ 
			{ Colors::WHITE, Colors::LTGRAY, Colors::DKGRAY, Colors::BLACK },
			{ Colors::WHITE, Colors::LTGRAY, Colors::DKGRAY, Colors::BLACK },
			{ Colors::WHITE, Colors::LTGRAY, Colors::DKGRAY, Colors::BLACK }
		}
	{
	}

	/** Single palette constructor
	  */
	ColorPalette(const ColorRGB& c0, const ColorRGB& c1, const ColorRGB& c2, const ColorRGB& c3) :
		trueColor{
			{ c0, c1, c2, c3 },
			{ c0, c1, c2, c3 },
			{ c0, c1, c2, c3 }
	}
	{
	}

	/** Full CGB-mode palette constructor
	  */
	ColorPalette(
		const ColorRGB& c00, const ColorRGB& c01, const ColorRGB& c02, const ColorRGB& c03,
		const ColorRGB& c10, const ColorRGB& c11, const ColorRGB& c12, const ColorRGB& c13,
		const ColorRGB& c20, const ColorRGB& c21, const ColorRGB& c22, const ColorRGB& c23
	) :
		trueColor{
			{ c00, c01, c02, c03 },
			{ c10, c11, c12, c13 },
			{ c20, c21, c22, c23 }
		}
	{
	}

	/** Get a reference to one of the four palette colors
	  */
	ColorRGB* operator [] (const unsigned char& index){
		return trueColor[index];
	}

	/** Get a const reference to one of the four palette colors
	  */
	ColorRGB* operator [] (const unsigned char& index) const {
		return const_cast<ColorRGB*>(trueColor[index]);
	}

	/** Set all four palette colors in 5-bit RGB components (0 to 31) for one of the three palette colors
	  * Array must contain AT LEAST 12 elements {R0, G0, B0, ... , R3, G3, B3}.
	  */
	void setColors(const unsigned int& index, const unsigned char* components);
	
	/** Set all four palette colors in RGB components (0 to 1) for one of the three palette colors
	  * Array must contain AT LEAST 12 elements {R0, G0, B0, ... , R3, G3, B3}.
	  */
	void setColors(const unsigned int& index, const float* components);

	/** Set all four palette colors for one of the three palette colors
	  * Each integer contains red color component in bits 0-4, green in 5-9, and blue in 10-14.
	  */
	void setColors(const unsigned int& index, const unsigned int& c0, const unsigned int& c1, const unsigned int& c2, const unsigned int& c3);
	
	/** Set all four palette colors for one of the three palette colors
	  */
	void setColors(const unsigned int& index, const ColorRGB& c0, const ColorRGB& c1, const ColorRGB& c2, const ColorRGB& c3);

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
	ColorRGB trueColor[3][4]; ///< Real RGB palette colors
};

class ColorPaletteDMG{
public:
	ColorPaletteDMG() :
		bPalettesLoaded(false),
		allPalettes(),
		paletteSelect(allPalettes.end()),
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
	ColorRGB* operator [] (const unsigned char& index){
		return palettes[index];
	}

	/** Get a const reference to one of the three DMG color palettes
	  */
	ColorRGB* operator [] (const unsigned char& index) const {
		return const_cast<ColorRGB*>(palettes[index]);
	}

	/** Get a reference to one of the three DMG color palettes
	  */
	ColorPalette& operator () () {
		return (*paletteSelect);
	}

	/** Get a const reference to one of the three DMG color palettes
	  */
	ColorPalette operator () () const {
		return (*paletteSelect);
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

	/** Return true if external color palettes vector is empty
	  */
	bool empty() const {
		return allPalettes.empty();
	}

	/** Return true if external color palettes have been loaded from an input file
	  */
	bool getPalettesLoaded() const {
		return bPalettesLoaded;
	}

	/** Get the number of palettes which are currently loaded
	  */
	size_t getNumPalettesLoaded() const {
		return allPalettes.size();
	}

	/** Switch to the next palette
	  */
	std::vector<ColorPalette>::iterator next();

	/** Revert to the previous palette
	  */
	std::vector<ColorPalette>::iterator prev();

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
	bool find(const std::string& fname, const unsigned short& id, bool verbose = false);

	/** Extract all DMG color palettes from a list of pre-defined palettes
	  * @return The total number of palettes added to the output vector
	  */
	unsigned int findAll(const std::string& fname, bool verbose = false);

	/** Manually add a color palette to the list of all palettes
	  */
	void addColorPalette(const ColorPalette& pal);

private:
	bool bPalettesLoaded; ///< Set if external color palettes have been loaded from an input file

	std::vector<ColorPalette> allPalettes; ///< Holder for all externally defined DMG color palettes
	
	std::vector<ColorPalette>::iterator paletteSelect; ///< Currently selected palette

	ColorPalette palettes; ///< DMG color palettes (0: BG/WIN, 1: OBJ0, 2: OBJ1)

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

