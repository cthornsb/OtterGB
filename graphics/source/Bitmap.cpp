
#include <iostream>
#include <fstream>
#include <string>

#include "Bitmap.hpp"
#include "Graphics.hpp"

unsigned char getBitmapPixel(const unsigned char &dx, const unsigned char &high, const unsigned char &low){
	unsigned char pixelColor = (high & (0x1 << dx)) >> dx; // LS bit of the color
	pixelColor += ((low & (0x1 << dx)) >> dx) << 1; // MS bit of the color
	return pixelColor; // [0,3]
}

Bitmap::Bitmap() : blank(true) {
	for(unsigned short j = 0; j < 8; j++){ // Bitmap row
		for(unsigned short k = 0; k < 8; k++){ // Bitmap col
			pixels[k][j] = 0; // Transparent
		}
	}
}

unsigned char Bitmap::get(const unsigned short &x, const unsigned short &y) const {
	return pixels[x][y];
}

void Bitmap::set(const unsigned short &x, const unsigned short &y, const unsigned char &color){
	pixels[x][y] = color;
}

void Bitmap::dump(){
	for(unsigned short j = 0; j < 8; j++){ // Bitmap row
		for(unsigned short k = 0; k < 8; k++){ // Bitmap col
			std::cout << (int)pixels[k][j];
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

CharacterMap::CharacterMap() : window(0x0) {
	std::string path(TOP_DIRECTORY);
	path += "/assets/cmap.dat";
	loadCharacterMap(path);
	palette[0] = Colors::WHITE;
	palette[1] = Colors::LTGRAY;
	palette[2] = Colors::DKGRAY;
	palette[3] = Colors::BLACK;
}

void CharacterMap::setPaletteColor(unsigned short &index, const ColorRGB &color){
	if(index <= 3)
		palette[index] = color;
}

bool CharacterMap::loadCharacterMap(const std::string &fname){
	// The character map
	const unsigned short nVals = 95;
	const char letters[95] = {32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
		                      42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
		                      52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
		                      62, 63, 64, 65, 66, 86, 68, 69, 70, 71,
		                      72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
		                      82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
		                      92, 93, 94, 95, 96, 97, 98, 99, 100, 101,
		                      102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
		                      112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
		                      122, 123, 124, 125, 126};
                          
	unsigned char rgb[nVals][16];
	std::ifstream ifile(fname.c_str(), std::ios::binary);
	if(!ifile.good())
		return false;
	for(unsigned short x = 0; x < nVals; x++){
		ifile.read((char*)&rgb[x][0], 16); // 16 bytes per bitmap (2 per row)
	}
	ifile.close();
	for(unsigned short ch = 0; ch < nVals; ch++){ // Character index
		for(unsigned short j = 0; j < 8; j++){ // Bitmap row
			for(unsigned short k = 0; k < 8; k++){ // Bitmap col
				cmap[(unsigned int)letters[ch]].set(k, j, getBitmapPixel((7-k), rgb[ch][2*j], rgb[ch][2*j+1]));
			}
		}
	}
	return true;
}

void CharacterMap::putCharacter(const char &val, const unsigned short &x, const unsigned short &y){
	unsigned char pixelColor;
	for(unsigned short dy = 0; dy < 8; dy++){
		for(unsigned short dx = 0; dx < 8; dx++){
			pixelColor = cmap[(unsigned int)val].get(dx, dy);
			if(transparency && pixelColor == 0) // Transparent
				continue;
			window->setDrawColor(palette[pixelColor]);
			window->drawPixel(8*x+dx, 8*y+dy);
		}
	}
}

void CharacterMap::putString(const std::string &str, const unsigned short &x, const unsigned short &y, bool wrap/*=true*/){
	short sx = x;
	short sy = y;
	for(size_t i = 0; i < str.length(); i++){
		putCharacter(str[i], sx++, sy);
		if(sx >= 20){
			if(!wrap)
				return;
			sx = 0;
			sy++;
		}
	}
}
