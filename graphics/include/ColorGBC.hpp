#ifndef COLOR_GBC_HPP
#define COLOR_GBC_HPP

class ColorGBC{
public:
	/** Default constructor
	  */
	ColorGBC() : 
		nColor(0), 
		nPalette(0), 
		bPriority(0), 
		bVisible(0) 
	{ 
	}
	
	/** Set pixel color for sprite layer
	  * @param color Pixel color ID number (0-3)
	  * @param palette Pixel color palette number (0-16)
	  * @param priority Sprite layering priority flag (0: Sprite Above BG, 1: Sprite behind BG color 1-3)
	  */
	void setColorOBJ(const unsigned char &color, const unsigned char &palette, bool priority);

	/** Set pixel color for background / window layers
	  * @param color Pixel color ID number (0-3)
	  * @param palette Pixel color palette number (0-16)
	  * @param priority Sprite layering priority flag (0: Use sprite priority bit, 1: BG above sprite)
	  */
	void setColorBG(const unsigned char &color, const unsigned char &palette, bool priority=true);

	/** Set pixel layering priority flag
	  */
	void setPriority(bool priority){ 
		bPriority = priority; 
	}

	/** Get pixel color ID number (0-3)
	  */ 
	unsigned char getColor() const { 
		return nColor; 
	}
	
	/** Get pixel color palette number (0-16)
	  */
	unsigned char getPalette() const { 
		return nPalette; 
	}
	
	/** When running in DMG-compatibility mode, CGB palettes OBJ0 and OBJ1 are stored in CGB palette numbers 8 and 9 respectively
	  */
	unsigned char getPaletteDMG() const {
		return (nPalette > 0 ? nPalette + 7 : 0);
	}

	/** Get pixel layering priority flag
	  */
	bool getPriority() const { 
		return bPriority; 
	}

	/** Return true if the pixel's priority flag is set and its color is non-zero.
	  * Used to determine WIN and BG layer priority versus sprites.
	  */
	bool getColorPriority() const {
		return (bPriority && (nColor != 0));
	}

	/** Return true if pixel is not transparent
	  * Background and window pixels are always non-transparent, sprite palette color 0 is always transparent.
	  */
	bool visible() const { 
		return bVisible; 
	}

	/** Reset pixel color and layering flags
	  */
	void reset();

private:
	unsigned char nColor; ///< 2-bit color ID number
	
	unsigned char nPalette; ///< Color palette number

	bool bPriority; ///< Layering priority flag
	
	bool bVisible; ///< Visibility flag
};

#endif

