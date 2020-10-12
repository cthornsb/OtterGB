#ifndef COLORS_HPP
#define COLORS_HPP

class sdlColor{
public:
	unsigned char r;
	unsigned char g;
	unsigned char b;

	/** Default constructor (black)
	  */
	sdlColor() : r(0), g(0), b(0) { }

	/** Grayscale constructor (0, 1)
	  */
	sdlColor(const float &value);

	/** RGB constructor (0, 1)
	  */
	sdlColor(const float &red, const float &green, const float &blue);

	/** Equality operator
	  */
	bool operator == (const sdlColor &rhs) const { return (r==rhs.r && g==rhs.g && b==rhs.b); }

	/** Inequality operator
	  */
	bool operator != (const sdlColor &rhs) const { return (r!=rhs.r || g!=rhs.g || b!=rhs.b); }

	/** Add a color to this color and return the result
	  */
	sdlColor operator + (const sdlColor &rhs) const ;

	/** Subtract a color from this color and return the result
	  */
	sdlColor operator - (const sdlColor &rhs) const ;

	/** Multiply this color by a constant scaling factor and return the result
	  */
	sdlColor operator * (const float &rhs) const ;

	/** Divide this color by a constant scaling factor and return the restul
	  */
	sdlColor operator / (const float &rhs) const ;

	/** Add a color to this color
	  */
	sdlColor& operator += (const sdlColor &rhs);

	/** Subtract a color from this color
	  */
	sdlColor& operator -= (const sdlColor &rhs);

	/** Multiply this color by a constant scaling factor
	  */
	sdlColor& operator *= (const float &rhs);
	
	/** Divide this color by a constant scaling factor
	  */
	sdlColor& operator /= (const float &rhs);

	/** Get the RGB inverse of this color
	  */
	sdlColor invert() const ;

	/** Conver the color to grayscale using RGB coefficients based on the sRGB convention
	  */
	void toGrayscale();
	
	/** Dump the RGB color components to stdout
	  */
	void dump() const ;
	
	/** Convert a floating point value in the range [0, 1] to an unsigned char between 0 and 255
	  */
	static unsigned char toUChar(const float &val){ return ((unsigned char)(val*255)); }
	
	/** Convert an unsigned char to a floating point value in the range [0, 1]
	  */
	static float toFloat(const unsigned char &val){ return (float(val)/255); }
};

namespace Colors{
	const sdlColor BLACK(0, 0, 0);
	const sdlColor DKGRAY(2/3.0, 2/3.0, 2/3.0);
	const sdlColor LTGRAY(1/3.0, 1/3.0, 1/3.0);
	const sdlColor WHITE(1, 1, 1);
	
	// Monochrome colors (GB)
	const sdlColor GB_DKSTGREEN(15.0/255,  56.0/255,  15.0/255);
	const sdlColor GB_DKGREEN(  48.0/255,  98.0/255,  48.0/255);
	const sdlColor GB_LTGREEN(  139.0/255, 172.0/255, 15.0/255);
	const sdlColor GB_GREEN(    155.0/255, 188.0/255, 15.0/255);
	
	// Primary colors
	const sdlColor RED(1, 0, 0);
	const sdlColor GREEN(0, 1, 0);
	const sdlColor BLUE(0, 0, 1);
	
	// Secondary colors
	const sdlColor YELLOW(1, 1, 0);
	const sdlColor MAGENTA(1, 0, 1);
	const sdlColor CYAN(0, 1, 1);
}

#endif
