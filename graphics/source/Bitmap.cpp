
#include <iostream>
#include <fstream>
#include <string>

#include "OTTWindow.hpp"

#include "Bitmap.hpp"

unsigned char getBitmapPixel(const unsigned char &dx, const unsigned char &high, const unsigned char &low){
	unsigned char pixelColor = (high & (0x1 << dx)) >> dx; // LS bit of the color
	pixelColor += ((low & (0x1 << dx)) >> dx) << 1; // MS bit of the color
	return pixelColor; // [0,3]
}

Bitmap::Bitmap() : 
	blank(true) 
{
	for(unsigned short j = 0; j < 8; j++){ // Bitmap row
		for(unsigned short k = 0; k < 8; k++){ // Bitmap col
			pixels[k][j] = 0; // Transparent
		}
	}
}

Bitmap::Bitmap(unsigned char* bmp) :
	blank(true)
{
	for(unsigned short j = 0; j < 8; j++){ // Bitmap row
		for(unsigned short k = 0; k < 8; k++){ // Bitmap col
			set(k, j, bmp[j * 8 + k]);
		}
	}
}

unsigned char Bitmap::get(const unsigned short &x, const unsigned short &y) const {
	return pixels[x][y];
}

void Bitmap::set(const unsigned short &x, const unsigned short &y, const unsigned char &color){
	if(color != 0)
		blank = false;
	pixels[x][y] = color;
}

void Bitmap::set(const unsigned char* data){
	for(unsigned short i = 0; i < 8; i++){ // Bitmap row
		for(unsigned short j = 0; j < 8; j++){ // Bitmap col
			set(j, i, getBitmapPixel(7 - j, data[2 * i], data[2 * i + 1]));
		}
	}
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
	// Load the ascii character map
	unsigned char rgb[16];
	std::ifstream ifile(fname.c_str(), std::ios::binary);
	if(!ifile.good())
		return false;
	for(unsigned short x = 0; x < 128; x++){
		ifile.read((char*)rgb, 16); // 16 bytes per bitmap (2 per row)
		cmap[x].set(rgb);
	}
	ifile.close();
	return true;
}

void CharacterMap::putCharacter(const char &val, const unsigned short &x, const unsigned short &y){
	unsigned char pixelColor;
	for(unsigned short dy = 0; dy < 8; dy++){
		for(unsigned short dx = 0; dx < 8; dx++){
			pixelColor = cmap[(unsigned int)val].get(dx, dy);
			if(transparency && pixelColor == 0) // Transparent
				continue;
			window->buffWrite(
				8 * x + dx, 
				8 * y + dy, 
				palette[pixelColor]
			);
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
