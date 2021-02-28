#ifndef IMAGE_BUFFER_HPP
#define IMAGE_BUFFER_HPP

#include <vector>

#include "colors.hpp"

class ImageBuffer{
public:
	/** Default constructor
	  */
	ImageBuffer() :
		nWidth(0),
		nHeight(0),
		nSize(0),
		nBytes(0),
		bitmap()
	{
	}
	
	/** Width and height constructor
	  */
	ImageBuffer(const unsigned int& W, const unsigned int& H) :
		nWidth(W),
		nHeight(H),
		nSize(W * H),
		nBytes(nSize * 3), // Three color channels
		bitmap(nBytes, 0)
	{
	}
	
	/** Copy constructor
	  */
	ImageBuffer(const ImageBuffer&) = delete;
	
	/** Assignment operator
	  */
	ImageBuffer& operator = (const ImageBuffer&) = delete;
	
	/** Destructor
	  */ 
	~ImageBuffer(){ }

	/** Return true if buffer data is empty
	  */
	bool empty() const {
		return (nSize > 0);
	}

	/** Get pointer to bitmap data
	  */
	const unsigned char* get() const {
		return static_cast<const unsigned char*>(bitmap.data());
	}
	
	/** Resize image buffer
	  */
	void resize(const unsigned short& W, const unsigned short& H);
	
	/** Set the color of a pixel in the buffer
	  */
	void setPixel(const unsigned short& x, const unsigned short& y, const ColorRGB& color);

	/** Set the color of a horizontal row of pixels
	  */
	void setPixelRow(const unsigned short& y, const ColorRGB& color);

	/** Fill image buffer with a value
	  * Uses std::fill to fill entire buffer with specified color component value.
	  */
	void fill(const unsigned char& value=0);
	
	/** Clear image buffer to a specified color
	  * For monochromatic colors, it should be faster to use fill().
	  */
	void clear(const ColorRGB& color);

private:
	unsigned short nWidth; ///< Width of image buffer in pixels
	
	unsigned short nHeight; ///< Height of image buffer in pixels
	
	unsigned int nSize; ///< Total size of image buffer in pixels
	
	unsigned int nBytes; ///< Total size of image in bytes
	
	std::vector<unsigned char> bitmap; ///< Image data
};

#endif
