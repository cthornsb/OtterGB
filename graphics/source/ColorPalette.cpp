#include <iostream>

#include "ColorPalette.hpp"

/////////////////////////////////////////////////////////////////////
// class ColorPalette
/////////////////////////////////////////////////////////////////////

void ColorPalette::setColors(const unsigned char* components){
	for(int i = 0; i < 4; i++){ // Over color index
		trueColor[i] = ColorRGB(
			convert(components[4 * i]),
			convert(components[4 * i + 1]),
			convert(components[4 * i + 2])
		);
	}
}

void ColorPalette::setColors(const float* components){
	for(int i = 0; i < 4; i++){ // Over color index
		trueColor[i] = ColorRGB(
			clamp(components[4 * i]),
			clamp(components[4 * i + 1]),
			clamp(components[4 * i + 2])
		);
	}
}

void ColorPalette::setColors(const unsigned int& c0, const unsigned int& c1, const unsigned int& c2, const unsigned int& c3){
	set(0, c0);
	set(1, c1);
	set(2, c2);
	set(3, c3);
}

void ColorPalette::setColors(const ColorRGB& c0, const ColorRGB& c1, const ColorRGB& c2, const ColorRGB& c3){
	set(0, c0);
	set(1, c1);
	set(2, c2);
	set(3, c3);
}

void ColorPalette::set(const unsigned char& index, const unsigned int& color){
	if(index > 3)
		return;
	trueColor[index] = convert(color);
}

void ColorPalette::set(const unsigned char& index, const ColorRGB& color){
	if(index > 3)
		return;
	trueColor[index] = color;
}

bool ColorPalette::write(std::ofstream& f){
	unsigned char r, g, b;
	for(int i = 0; i < 4; i++){ // Over color index
		// Write the color components
		convert(trueColor[i], r, g, b);
		f.write((char*)&r, 1);
		f.write((char*)&g, 1);
		f.write((char*)&b, 1);
	}
	return (f.good());
}

bool ColorPalette::read(std::ifstream& f){
	unsigned char r, g, b;
	for(int i = 0; i < 4; i++){ // Over color index
		// Write the color components
		f.read((char*)&r, 1);
		f.read((char*)&g, 1);
		f.read((char*)&b, 1);
		convert(trueColor[i], r, g, b);
	}
	return (f.good() && !f.eof());
}

void ColorPalette::convert(const unsigned int& input, unsigned char& r, unsigned char& g, unsigned char& b){
	r = (unsigned char)((input & 0xff0000) >> 16); // red
	g = (unsigned char)((input & 0x00ff00) >> 8);  // green
	b = (unsigned char)((input & 0x0000ff));       // blue
}

void ColorPalette::convert(const ColorRGB& input, unsigned char& r, unsigned char& g, unsigned char& b){
	r = (unsigned char)(clamp(input.r) * 31);
	g = (unsigned char)(clamp(input.g) * 31);
	b = (unsigned char)(clamp(input.b) * 31);
}

ColorRGB ColorPalette::convert(const unsigned int& input){
	unsigned char r, g, b;
	convert(input, r, g, b);
	return ColorRGB(convert(r), convert(g), convert(b));
}

ColorRGB ColorPalette::convert(const unsigned char& r, const unsigned char& g, const unsigned char& b){
	return ColorRGB(convert(r), convert(g), convert(b));
}

float ColorPalette::convert(const unsigned char& input){
	return ((input & 0x1f) / 31.f);
}

unsigned char ColorPalette::convert(const float& input){
	return ((unsigned char)(clamp(input) * 31));
}

float ColorPalette::clamp(const float& input){
	return std::min(std::max(input, 0.f), 1.f);
}

/////////////////////////////////////////////////////////////////////
// class ColorPaletteDMG
/////////////////////////////////////////////////////////////////////

void ColorPaletteDMG::setPaletteID(const unsigned char& table, const unsigned char& entry){
	nTableNumber = table;
	nEntryNumber = entry;
}

bool ColorPaletteDMG::write(std::ofstream& f){
	// Write palette numbers to file
	f.write((char*)&nTableNumber, 1);
	f.write((char*)&nEntryNumber, 1);
	
	// Decode palette colors
	unsigned char r, g, b;
	for(int i = 0; i < 3; i++){ // Over palette number
		for(int j = 0; j < 4; j++){ // Over color index
			// Convert color components to chars
			ColorPalette::convert(palettes[i][j], r, g, b);

			// Write the color components
			f.write((char*)&r, 1);
			f.write((char*)&g, 1);
			f.write((char*)&b, 1);
		}
	}
	
	return (f.good());
}

bool ColorPaletteDMG::read(std::ifstream& f){
	return (readHeader(f) && readPalettes(f));
}

bool ColorPaletteDMG::find(const std::string& fname, const unsigned short& id, bool verbose/*=false*/){
	std::ifstream f(fname.c_str(), std::ios::binary);
	if(!f.good()){
		if(verbose)
			std::cout << " [palettes] Error! Failed to read DMG palette file \"" << fname << "\"." << std::endl;
		return false;
	}
	
	bool retval = false;
	unsigned char findTable = (unsigned char)((id & 0xff00) >> 8);
	unsigned char findEntry = (unsigned char)((id & 0x00ff));
	while(readHeader(f)){
		if(findTable == nTableNumber && findEntry == nEntryNumber){ // Match
			if(verbose)
				std::cout << " [palettes] Found DMG palette matching table=" << (int)(findTable) << " and entry=" << (int)(findEntry) << std::endl;
			retval = readPalettes(f);
			break;			
		}
		else{ // Skip the rest of the DMG palette
			f.seekg(36, f.cur);
		}
	}
	
	if(!retval && verbose)
		std::cout << " [palettes] Error! Failed to load DMG color palette." << std::endl;
	
	f.close();
	
	return retval;
}

bool ColorPaletteDMG::readHeader(std::ifstream& f){
	f.read((char*)&nTableNumber, 1);
	f.read((char*)&nEntryNumber, 1);
	return (f.good() && !f.eof());
}

bool ColorPaletteDMG::readPalettes(std::ifstream& f){	
	// Decode palette colors
	unsigned char r, g, b;
	for(int i = 0; i < 3; i++){ // Over palette number
		for(int j = 0; j < 4; j++){ // Over color index
			// Read the color components
			f.read((char*)&r, 1);
			f.read((char*)&g, 1);
			f.read((char*)&b, 1);
			if(f.eof())
				return false;
				
			// Convert color components
			palettes[i][j] = ColorPalette::convert(r, g, b);
		}
	}
	return (f.good() && !f.eof());
}


