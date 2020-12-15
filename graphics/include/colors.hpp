#ifndef COLORS_HPP
#define COLORS_HPP

#ifdef USE_OPENGL

	class ColorRGB{
	public:
		float r;
		float g;
		float b;
		
#else

	class ColorRGB{
	public:
		unsigned char r;
		unsigned char g;
		unsigned char b;

#endif

	/** Default constructor (black)
	  */
	ColorRGB() : r(0), g(0), b(0) { }

	/** Grayscale constructor (0, 1)
	  */
	ColorRGB(const float &value);

	/** RGB constructor (0, 1)
	  */
	ColorRGB(const float &red, const float &green, const float &blue);

	/** Equality operator
	  */
	bool operator == (const ColorRGB &rhs) const { return (r==rhs.r && g==rhs.g && b==rhs.b); }

	/** Inequality operator
	  */
	bool operator != (const ColorRGB &rhs) const { return (r!=rhs.r || g!=rhs.g || b!=rhs.b); }

	/** Add a color to this color and return the result
	  */
	ColorRGB operator + (const ColorRGB &rhs) const ;

	/** Subtract a color from this color and return the result
	  */
	ColorRGB operator - (const ColorRGB &rhs) const ;

	/** Multiply this color by a constant scaling factor and return the result
	  */
	ColorRGB operator * (const float &rhs) const ;

	/** Divide this color by a constant scaling factor and return the restul
	  */
	ColorRGB operator / (const float &rhs) const ;

	/** Add a color to this color
	  */
	ColorRGB& operator += (const ColorRGB &rhs);

	/** Subtract a color from this color
	  */
	ColorRGB& operator -= (const ColorRGB &rhs);

	/** Multiply this color by a constant scaling factor
	  */
	ColorRGB& operator *= (const float &rhs);
	
	/** Divide this color by a constant scaling factor
	  */
	ColorRGB& operator /= (const float &rhs);

	/** Get the RGB inverse of this color
	  */
	ColorRGB invert() const ;

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
	const ColorRGB BLACK(0, 0, 0);
	const ColorRGB DKGRAY(2/3.0f, 2/3.0f, 2/3.0f);
	const ColorRGB LTGRAY(1/3.0f, 1/3.0f, 1/3.0f);
	const ColorRGB WHITE(1, 1, 1);
	
	// Monochrome colors (GB)
	const ColorRGB GB_DKSTGREEN(15.0f/255,  56.0f/255,  15.0f/255);
	const ColorRGB GB_DKGREEN(  48.0f/255,  98.0f/255,  48.0f/255);
	const ColorRGB GB_LTGREEN(  139.0f/255, 172.0f/255, 15.0f/255);
	const ColorRGB GB_GREEN(    155.0f/255, 188.0f/255, 15.0f/255);
	
	// Primary colors
	const ColorRGB RED(1, 0, 0);
	const ColorRGB GREEN(0, 1, 0);
	const ColorRGB BLUE(0, 0, 1);
	
	// Secondary colors
	const ColorRGB YELLOW(1, 1, 0);
	const ColorRGB MAGENTA(1, 0, 1);
	const ColorRGB CYAN(0, 1, 1);
}

#endif
