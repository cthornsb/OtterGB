#ifndef GPU_HPP
#define GPU_HPP

#include "colors.hpp"
#include "SystemComponent.hpp"

class sdlWindow;

/////////////////////////////////////////////////////////////////////
// class Color
/////////////////////////////////////////////////////////////////////

class Color{
public:
	unsigned char r, g, b;

	Color() : r(0), g(0), b(0) { }
	
	Color(const float &fr, const float &fg, const float &fb){
		r = (const char)(fr * 255);
		g = (const char)(fg * 255);
		b = (const char)(fb * 255);
	}
};

/////////////////////////////////////////////////////////////////////
// class SpriteAttHandler
/////////////////////////////////////////////////////////////////////

class SpriteAttHandler : public SystemComponent {
public:
	unsigned char yPos; // Y-position of the current sprite
	unsigned char xPos; // X-position of the current sprite
	unsigned char tileNum; // Tile index for the current sprite
	
	bool objPriority; // Object to Background priority (0: OBJ above BG, 1: OBJ behind BG color 1-3, BG color 0 always behind))
	bool yFlip; // (0: normal, 1: mirrored vertically)
	bool xFlip; // (0: normal, 1: mirrored horizontally)
	bool ngbcPalette; // (0: OBP0, 1: OBP1)
	bool gbcVramBank; // (0: Bank 0, 1: Bank 1)

	unsigned char gbcPalette; // (OBP0-7)

	SpriteAttHandler() : SystemComponent(160), index(0) { }

	virtual bool preWriteAction();
	
	bool getNextSprite(bool &visible);
	
private:
	unsigned short index; // Current sprite index [0,40)
};

/////////////////////////////////////////////////////////////////////
// class TileMap
/////////////////////////////////////////////////////////////////////

class TileMap{
public:
	TileMap() : data0(0x0), data1(0x0) { }

	TileMap(unsigned char *ptr1, unsigned char *ptr2) : data0(ptr1), data1(ptr2) { }

	void setData(unsigned char *ptr1, unsigned char *ptr2){ data0 = ptr1; data1 = ptr2; }
	
	bool getPixelColors(const unsigned short &id, std::vector<unsigned short> &lines);
	
private:
	unsigned char *data0; // Pointer to the 0th tile in the lower tilemap [0x8000-0x9000]
	unsigned char *data1; // Pointer to the 0th tile in the upper tilemap [0x8800-0x9800]
};

// Background palette memory contains 64 bytes of color data
// Each color is represented by two bytes, with a total of 4
// colors (0-3) for BG palettes 0-7.
class Palette{
public:
	Palette() : data(0x0) { }
	
	Palette(unsigned char *ptr) : data(ptr) { }

	void setData(unsigned char *ptr){ data = ptr; }

	void updateColors();

	static sdlColor getRGB(const unsigned short &gbcColor);

private:
	unsigned char *data; // BGP0...BGP7 [color0...color3]
	
	unsigned char index; // Byte index in palette memory [0,63]
	
	sdlColor rgb[8]; ///< Real RGB colors
};

// 32x32 tile background maps in VRAM at [9800:9BFF] and [9C00:9FFF]
// Each map can be used to display background or window layers.
// Background Tile Map contains 32 rows of 32 Bytes where each byte
// contains a number of a tile to be displayed. 
class BackgroundMap{
public:
	BackgroundMap(){ }
	
	BackgroundMap(unsigned char *ptr) : data(ptr) { }

	void setData(unsigned char *ptr){ data = ptr; }

	class TileAttr{
	public:
		TileAttr(){ }
	
		unsigned char bgPaletteNum;
		bool tileBankNum;
		bool horizontalFlip;
		bool verticalFlip;
		bool bgPriority;
	};

	void getTileAttributes(TileAttr &t);

private:
	unsigned char *data; // Pointer to the 1st BG/Window tile map data
	unsigned char *attrData; // Pointer to BG tile map attribute data
	
	unsigned char index;
};

/////////////////////////////////////////////////////////////////////
// class GPU
/////////////////////////////////////////////////////////////////////

class GPU : public SystemComponent {
public:
	GPU();

	~GPU();

	void debug();

	void drawTileMaps();

	void drawNextScanline(SpriteAttHandler *oam);

	void render();

	bool getWindowStatus();

	virtual bool preWriteAction();
	
	virtual bool preReadAction();

	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val);
	
	virtual bool readRegister(const unsigned short &reg, unsigned char &val);

private:
	bool bgDisplayEnable;
	bool objDisplayEnable;
	bool objSizeSelect; ///< Sprite size [0: 8x8, 1: 8x16]
	bool bgTileMapSelect;
	bool bgWinTileDataSelect;
	bool winDisplayEnable;
	bool winTileMapSelect;
	bool lcdDisplayEnable;

	bool coincidenceFlag;
	bool mode0IntEnable;
	bool mode1IntEnable;
	bool mode2IntEnable;
	bool lycCoincIntEnable;

	bool bgPaletteIndexAutoInc;
	bool objPaletteIndexAutoInc;

	unsigned char lcdcStatusFlag;

	// Gameboy colors
	unsigned char ngbcPaletteColor[4]; ///< Original GB background palette
	unsigned char ngbcObj0PaletteColor[4]; ///< Original GB sprite palette 0
	unsigned char ngbcObj1PaletteColor[4]; ///< Original GB sprite palette 1

	unsigned char bgPaletteIndex;
	unsigned char objPaletteIndex;

	// GBC colors
	unsigned char bgPaletteData[64]; ///< GBC background palette 0-7
	unsigned char objPaletteData[64]; ///< GBC sprite palette 0-7

	BackgroundMap bgMap0;
	BackgroundMap bgMap1;
	Palette bgPalette;
	Palette objPalette;
	TileMap tiles0;
	TileMap tiles1;
	
	sdlWindow *window; ///< Pointer to the main renderer window
	
	// Each pixel has a 2-bit color depth (0-3)
	// Each of the 256 scanlines will contain 256*2=512 bits i.e. 64 bytes
	// The 0th byte contains colors for the first 4 pixels, where the lowest
	// significant 2 bits is the color for 
	unsigned char frameBuffer[256][256];
	
	void drawTile(const unsigned char &x, const unsigned char &y, const unsigned short &offset);
	
	void drawSprite(const unsigned char &y, SpriteAttHandler *oam);
};

#endif
