
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include "ColorPalette.hpp"

/** Decode a color from a 32-bit interger with the form 0x00RRGGBB 
  * Here, R G and B are the red green and blue color components respectively.
  * The 8-bit color components of the input color will be converted to the 5-bit colors expected by CGB palettes.
  */
ColorRGB decodeColor(const unsigned int& input){
	float r = (unsigned char)((input & 0xff0000) >> 16) / 255.f; // 8-bit red
	float g = (unsigned char)((input & 0x00ff00) >> 8)  / 255.f; // 8-bit green
	float b = (unsigned char)((input & 0x0000ff))       / 255.f; // 8-bit blue
	return ColorRGB(r, g, b);
}

/** Convert an ascii file of CGB color palettes to the binary format expected by ottergb
  * Syntax: palettes <input> [output]
  * @param input Input ascii filename
  * @param output Output ascii filename (defaults to "palettes.bin" if left blank)
  * @return 0 upon success
  */
int main(int argc, char* argv[]){
	if(argc < 2){
		std::cout << " [palettes] Error! Invalid syntax" << std::endl;
		std::cout << " [palettes]  SYNTAX: palettes <input> [output]" << std::endl;
		return 1;
	}

	std::ifstream imap(argv[1]);
	if(!imap.good())
		return 2;
	
	std::ofstream omap;
	if(argc >= 3)
		omap.open(argv[2], std::ios::binary);
	else
		omap.open("palettes.bin", std::ios::binary);

	if(!omap.good()){
		imap.close();
		return 3;
	}

	std::vector<unsigned short> foundIDs;

	std::string raw[14];
	unsigned short count = 0;
	unsigned short id;
	unsigned char table;
	unsigned char entry;
	ColorPaletteDMG colors;
	while(true){
		// Read values from file
		for(int i = 0; i < 14; i++){
			imap >> raw[i];
			if(imap.eof())
				break;
		}
		if(imap.eof())
			break;
		
		// Convert ascii hex to integers
		table = (unsigned char)std::stoul(raw[0], 0, 16);
		entry = (unsigned char)std::stoul(raw[1], 0, 16);
		id = table + ((entry & 0xffff) << 8);
		auto iter = std::find(foundIDs.begin(), foundIDs.end(), id);
		if(iter != foundIDs.end()){ // Duplicate entry
			continue;
		}
		
		// Add entry to vector
		foundIDs.push_back(id);

		// Set the palette ID		
		colors.setPaletteID(table, entry);		
		
		// Decode palette colors
		for(int i = 0; i < 3; i++){ // Over palette number
			for(int j = 0; j < 4; j++){ // Over color index
				colors[i].set(j, decodeColor(std::stoul(raw[2 + i * 4 + j], 0, 16))); // Decode the color
			}
		}
		
		// Write the output DMG palette
		if(!colors.write(omap)){
			std::cout << " [palettes] Error! Failed to write DMG palette to output file!" << std::endl;
			break;
		}

		count++;
	}

	std::cout << " [palettes] Done! Found " << count << " unique color palettes (total " << count * 38 << " bytes)." << std::endl;

	imap.close();
	omap.close();

	return 0;
}
