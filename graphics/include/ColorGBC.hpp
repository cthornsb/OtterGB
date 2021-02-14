#ifndef COLOR_GBC_HPP
#define COLOR_GBC_HPP

class ColorGBC{
public:
	ColorGBC() : nColor(0), nPalette(0), bPriority(0), bVisible(0) { }
	
	void setColorOBJ(const unsigned char &color, const unsigned char &palette, bool priority=true);

	void setColorBG(const unsigned char &color, const unsigned char &palette, bool priority=true);

	void setPriority(bool priority){ bPriority = priority; }

	unsigned char getColor() const { return nColor; }
	
	unsigned char getPalette() const { return nPalette; }

	bool getPriority() const { return bPriority; }

	bool visible() const { return bVisible; }

	void reset();

private:
	unsigned char nColor;
	unsigned char nPalette;

	bool bPriority; ///< Priority flag
	bool bVisible; ///< Visibility flag
};

#endif

