#include <algorithm>

#include "ImageBuffer.hpp"

void ImageBuffer::resize(const unsigned short& W, const unsigned short& H){
	nWidth = W;
	nHeight = H;
	nSize = W * H;
	nBytes = nSize * 3;
	bitmap.resize(nBytes); // Contract size or expand, filling new pixels with zero
}

void ImageBuffer::setPixel(const unsigned short& x, const unsigned short& y, const ColorRGB& color){
	unsigned char* base = &bitmap[(nWidth * (nHeight - y - 1) + x) * 3];
	base[0] = color.r;
	base[1] = color.g;
	base[2] = color.b;
}

void ImageBuffer::setPixelRow(const unsigned short& y, const ColorRGB& color){
	for(unsigned short i = 0; i < nWidth; i++)
		setPixel(i, y, color);
}

void ImageBuffer::fill(const unsigned char& value/*=0*/){
	std::fill(bitmap.begin(), bitmap.end(), value);
}

void ImageBuffer::clear(const ColorRGB& color){
	for(unsigned int i = 0; i < nSize; i++){
		bitmap[3 * i]     = color.r;
		bitmap[3 * i + 1] = color.g;
		bitmap[3 * i + 2] = color.b;
	}
}

