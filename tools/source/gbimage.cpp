#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>

#include "OTTTexture.hpp"

#include "SOIL/SOIL.h"

const unsigned short nPixelsWidth = 160;
const unsigned short nPixelsHeight = 144;

namespace Colors{
	// Monochrome colors (GB)
	const ColorRGB GB_DKSTGREEN(15.0f/255,  56.0f/255,  15.0f/255);
	const ColorRGB GB_DKGREEN(  48.0f/255,  98.0f/255,  48.0f/255);
	const ColorRGB GB_LTGREEN(  139.0f/255, 172.0f/255, 15.0f/255);
	const ColorRGB GB_GREEN(    155.0f/255, 188.0f/255, 15.0f/255);
}

class Histogram{
public:
	Histogram() :
		nPaletteColors(0),
		nMinimum(255),
		nMaximum(0),
		fSum(0.f),
		bins(256, 0),
		colors()
	{
	}	

	void operator () (const unsigned char& val) {
		if(val < nMinimum)
			nMinimum = val;
		if(val > nMaximum)
			nMaximum = val;
		bins[val]++;
	}
	
	/** Get the minimum color component of this color channel
	  */
	unsigned char getMinimum() const {
		return nMinimum;
	}
	
	/** Get the maximum color component of this color channel
	  */
	unsigned char getMaximum() const {
		return nMaximum;
	}
	
	/** Get the dynamic range of this color channel
	  */
	unsigned char getDynamicRange() const {
		return (nMaximum - nMinimum);
	}
	
	/** Get the number of palette colors
	  */
	int getNumberPaletteColors() const {
		return nPaletteColors;
	}
	
	/** 
	  */
	void compute(const int& nColors){
		nPaletteColors = nColors;
		for(int i = 0; i < 256; i++){
			fSum += (float)bins[i];
		}
		int index = 0;
		float fRunningSum = 0.f;
		std::vector<float> values;
		const float step = fSum / nColors;
		for(int i = 0; i <= nColors; i++){
			values.push_back(i * step);
		}
		for(int i = 0; i < 256; i++){
			fRunningSum += (float)bins[i];
			if(fRunningSum >= values[index]){
				colors.push_back(i);
				if(++index > nColors)
					break;
			}
		}
	}
	
	/** Get monochrome color palette
	  */
	void getPalette(std::vector<ColorRGB>& output) const {
		for(auto val = colors.cbegin(); val != colors.cend(); val++){
			output.push_back(ColorRGB(*val / 255.f));
		}
	}
	
	std::vector<unsigned char>::const_iterator begin() const {
		return colors.cbegin();
	}

	std::vector<unsigned char>::const_iterator end() const {
		return colors.cend();
	}

private:
	int nPaletteColors;

	unsigned char nMinimum;
	
	unsigned char nMaximum;
	
	float fSum;

	std::vector<unsigned int> bins;
	
	std::vector<unsigned char> colors;
};

class Triplet{
public:
	unsigned int nCount;

	float fSum[3];
	
	Triplet() : 
		nCount(0),
		fSum{0, 0, 0}
	{
	}
	
	void add(const ColorRGB& color){
		fSum[0] += (float)color.r;
		fSum[1] += (float)color.g;
		fSum[2] += (float)color.b;
		nCount++;
	}
	
	void compute(){
		fSum[0] /= nCount;
		fSum[1] /= nCount;
		fSum[2] /= nCount;
	}
};

class ColorAverage{
public:
	ColorAverage() :
		nSums(),
		nBins(),
		histo(0x0)
	{
	}
	
	ColorAverage(const Histogram& h) :
		nSums(h.getNumberPaletteColors(), Triplet()),
		nBins(h.begin(), h.end()),
		histo(&h)
	{
		for(std::vector<unsigned char>::iterator iter = nBins.begin(); iter != nBins.end(); iter++){
			std::cout << " " << (int)*iter << std::endl;
		}
	}

	void operator () (const ColorRGB& color, const unsigned char& val) {
		std::vector<Triplet>::iterator iter = findBin(val);
		if(iter != nSums.end()){
			iter->add(color);
		}
	}
	
	void compute(std::vector<ColorRGB>& output){
		for(std::vector<Triplet>::iterator iter = nSums.begin(); iter != nSums.end(); iter++){
			iter->compute();
			output.push_back(ColorRGB(iter->fSum[0] / 255.f, iter->fSum[1] / 255.f, iter->fSum[2] / 255.f));
		}
	}

private:
	std::vector<Triplet> nSums;
	
	std::vector<unsigned char> nBins;
	
	const Histogram* histo;
	
	std::vector<Triplet>::iterator findBin(const unsigned char& val){
		int count = 0;
		for(std::vector<unsigned char>::iterator iter = nBins.begin() + 1; iter != nBins.end(); iter++){
			if(val <= *iter){
				return nSums.begin() + count;
			}
			count++;
		}
		return nSums.end();
	}
};

Histogram histo[3];

// Pixel colors
std::vector<ColorRGB> allColors;

// DMG palette
std::vector<ColorRGB> palette;

// Output bitmap
std::vector<std::vector<int> > bitmap(160, std::vector<int>(144, 0));

bool compColorsRed(const ColorRGB& left, const ColorRGB& right){
	return (left.r > right.r);
}

bool compColorsGreen(const ColorRGB& left, const ColorRGB& right){
	return (left.g > right.g);
}

bool compColorsBlue(const ColorRGB& left, const ColorRGB& right){
	return (left.b > right.b);
}

/** Compute the euclidian "distance" between two colors using the formula sqrt((r1-r2)^2 + (g1-g2)^2 + (b1-b2)^2)
  */
float colorDistance(OTTLogicalColor* input, const ColorRGB& output){
	float dR = (float)input->pArray[0] - (float)output.r;
	float dG = (float)input->pArray[1] - (float)output.g;
	float dB = (float)input->pArray[2] - (float)output.b;
	return std::sqrt(dR * dR + dG * dG + dB * dB);
}

/** Find the closest matching RGB color from a palette of pre-defined colors
  */
int matchColor(OTTLogicalColor* input, const std::vector<ColorRGB>& palette){
	int retval = -1;
	float minimumDistance = 9999.f;
	
	// Loop through all palette colors and find which color is closest to the input color
	int index = 0;
	for(auto color = palette.cbegin(); color != palette.cend(); color++){
		float dist = colorDistance(input, *color);
		if(dist < minimumDistance){
			retval = index;
			minimumDistance = dist;
		}
		index++;
	}
	
	return retval;
}

void analyzeImage(OTTLogicalColor* color, const int& x, const int& y){
	// Convert pixel to grayscale (from 24-bit to 8-bit color)
	//color->toGrayscale();
	allColors.push_back(color->getColor());
	for(int i = 0; i < 3; i++)
		histo[i](color->pArray[i]);
}

void pixelFunction(OTTLogicalColor* color, const int& x, const int& y){
	// Convert pixel color from 24-bit to pre-defined color palette
	int index = matchColor(color, palette);
	bitmap[x][y] = index;
}

int main(int argc, char* argv[]){
	if(argc < 2)
		return 1;

	// Load input image
	OTTTexture inputImage(argv[1]);

	// Check image loaded correctly
	if(!inputImage.isGood()){
		std::cout << " [Fatal Error] Failed to load input image!\n";
		return 1;
	}
	
	// Check image size
	if(inputImage.getWidth() != nPixelsWidth || inputImage.getHeight() != nPixelsHeight){
		std::cout << " [Fatal Error] Input image has incorrect dimension (" << inputImage.getWidth() << "x" << inputImage.getHeight() << ").\n";
		inputImage.free();
		return 2;
	}

	allColors.reserve(nPixelsWidth * nPixelsHeight);

	// Process input image
	inputImage.processImage(analyzeImage);

	// DMG
	//histo[0].compute(4);
	//histo[0].getPalette(palette);

	// CGB
	std::cout << (int)histo[0].getDynamicRange() << std::endl;
	std::cout << (int)histo[1].getDynamicRange() << std::endl;
	std::cout << (int)histo[2].getDynamicRange() << std::endl;

	unsigned char dR = histo[0].getDynamicRange();
	unsigned char dG = histo[1].getDynamicRange();
	unsigned char dB = histo[2].getDynamicRange();

	int maxRange = -1;
	if(dR >= dG){
		if(dR >= dB){ // max = dR
			maxRange = 0;
		}
		else{ // max = dB
			maxRange = 2;
		}
	}
	else{ // dG > dR
		if(dG >= dB){ // max = dG
			maxRange = 1;
		}
		else{ // max = dB
			maxRange = 2;
		}
	}
	
	std::cout << "max=" << maxRange << std::endl;
	std::cout << "size=" << allColors.size() << std::endl;
	
	histo[maxRange].compute(16);
	ColorAverage averager(histo[maxRange]);
	for(auto col = allColors.cbegin(); col != allColors.cend(); col++){
		averager(*col, (*col)[maxRange]);
	}	
	
	switch(maxRange){
	case 0: // Red
		//histo[0].compute(16);
		//histo[0].getPalette(palette);
		//std::sort(allColors.begin(), allColors.end(), compColorsRed);
		break;
	case 1: // Green
		//histo[1].compute(16);
	//	histo[1].getPalette(palette);
		//std::sort(allColors.begin(), allColors.end(), compColorsGreen);
		break;
	case 2: // Blue
		//histo[2].compute(16);
		//histo[2].getPalette(palette);
		//std::sort(allColors.begin(), allColors.end(), compColorsBlue);
		break;
	default:
		break;
	}
	
	averager.compute(palette);

	for(int i = 0; i < 16; i++){
		palette[i].dump();
	}

	// Convert the image to the new color palette
	inputImage.processImage(pixelFunction);

	int x0, x1;
	int y0, y1;
	OTTLogicalColor color;
	for(int i = 0; i < nPixelsHeight / 8; i++){
		y0 = i * 8;
		y1 = (i + 1) * 8;
		for(int j = 0; j < nPixelsWidth / 8; j++){
			x0 = j * 8;
			x1 = (j + 1) * 8;
			for(int x = x0; x < x1; x++){
				for(int y = y0; y < y1; y++){
					inputImage.getPixel(x, y, color);
					color.setColor(palette[bitmap[x][y]]);
					/*switch(bitmap[x][y]){
					case 0:
						color.setColor(Colors::GB_DKSTGREEN);
						break;
					case 1:
						color.setColor(Colors::GB_DKGREEN);
						break;
					case 2:
						color.setColor(Colors::GB_LTGREEN);
						break;
					case 3:
						color.setColor(Colors::GB_GREEN);
						break;
					default:
						color.setColor(Colors::RED); // Debugging
						break;
					}*/
				}
			}
		}
	}

	// Save the bitmap (for testing)
	inputImage.write("dummy.bmp");

	return 0;
}
