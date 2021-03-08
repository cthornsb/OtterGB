#ifndef BITMAP_HPP
#define BITMAP_HPP

#include <string>

#include "ColorRGB.hpp"

class OTTWindow;

class Bitmap{
public:
	/** Default constructor
	  */
	Bitmap();
	
	/** Explicit pixel color constructor (loose-packing)
	  * Input array should have AT LEAST 64 elements
	  */
	Bitmap(unsigned char* bmp);
	
	/** Return true if the color of at least one pixel in the bitmap is non-zero
	  */
	bool isBlank() const { 
		return blank; 
	}
	
	/** Get the color of the pixel at location (x,y) indexed from (0,0) at the top left
	  */
	unsigned char get(const unsigned short &x, const unsigned short &y) const ;
	
	/** Set the color of the pixel at location (x,y) indexed from (0,0) at the top left
	  */
	void set(const unsigned short &x, const unsigned short &y, const unsigned char &color);
	
	/** Set the color of all bitmap pixels
	  * @param data Array of tightly-packed pixel colors, expects AT LEAST 16 elements (two bytes per row)
	  */
	void set(const unsigned char* data);
	
	/** Dump all pixel colors to stdout
	  */
	void dump();
	
private:
	bool blank; ///< Unset if any pixel in the bitmap is non-zero

	unsigned char pixels[8][8]; ///< Two-bit colors for all bitmap pixels
};

class CharacterMap{
public:
	/** Default constructor
	  */
	CharacterMap();

	/** Set the graphical window to draw to
	  */
	void setWindow(OTTWindow *win){ 
		window = win; 
	}

	/** Set one of the four text palette colors
	  * In DMG graphics, color 0 is typically background while color 3 is typically 
	  * foreground, but this depends on the character map currently being used.
	  */
	void setPaletteColor(unsigned short &index, const ColorRGB &color);

	/** Enable or disable text background transparency
	  */
	void setTransparency(bool state=true){ 
		transparency = state; 
	}

	/** Load ascii character bitmaps from a file
	  */
	bool loadCharacterMap(const std::string &fname);

	/** Print a character on the screen at a specified location
	  * Locations are indexed from (0,0) at top left of screen
	  * @param val The character to print
	  * @param x Column where character will be printed
	  * @param y Row where character will be printed 
	  */
	void putCharacter(const char &val, const unsigned short &x, const unsigned short &y);

	/** Print a string on the screen at a specified location
	  * Locations are indexed from (0,0) at top left of screen
	  * @param str The string to print
	  * @param x Column where first character will be printed
	  * @param y Row where first character will be printed 
	  * @param wrap If set to true, text extending off the screen will wrap to a new line
	  */
	void putString(const std::string &str, const unsigned short &x, const unsigned short &y, bool wrap=true);

protected:
	OTTWindow *window; ///< Pointer to graphical display window
	
	bool transparency; ///< Set if text background transparency is enabled
	
	ColorRGB palette[4]; ///< Text color palette

	Bitmap cmap[128]; ///< Ascii character map bitmaps
};

#endif
