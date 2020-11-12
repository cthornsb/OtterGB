#ifndef GPU_HPP
#define GPU_HPP

#include <string>

#include "colors.hpp"
#include "SystemComponent.hpp"

class Window;
class CharacterMap;

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
	
	void reset(){ index = 0; }
	
private:
	unsigned short index; // Current sprite index [0,40)
};

/////////////////////////////////////////////////////////////////////
// class GPU
/////////////////////////////////////////////////////////////////////

class GPU : public SystemComponent {
public:
	GPU();

	~GPU();
	
	void initialize();

	void drawTileMaps(bool map1=false);

	void drawNextScanline(SpriteAttHandler *oam);

	void render();

	Window *getWindow(){ return window; }

	bool getWindowStatus();
	
	void setPixelScale(const unsigned int &n);

	void print(const std::string &str, const unsigned char &x, const unsigned char &y);

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

	bool bgPaletteIndexAutoInc;
	bool objPaletteIndexAutoInc;

	// Gameboy colors
	unsigned char ngbcPaletteColor[3][4]; ///< Original GB background palettes

	unsigned char bgPaletteIndex;
	unsigned char objPaletteIndex;

	// GBC colors
	unsigned char bgPaletteData[64]; ///< GBC background palette 0-7
	unsigned char objPaletteData[64]; ///< GBC sprite palette 0-7

	ColorRGB gbcPaletteColors[16][4]; ///< RGB colors for GBC background and sprite palettes 0-7

	Window *window; ///< Pointer to the main renderer window
	
	CharacterMap *cmap; ///< Pointer to the character map used for printing text.
	
	ColorGBC currentLineSprite[256];
	ColorGBC currentLineWindow[256];
	ColorGBC currentLineBackground[256];

	/** Retrieve the color of a pixel in a tile bitmap.
	  * @param index The index of the tile in VRAM [0x8000,0x8FFF].
	  * @param dx    The horizontal pixel in the bitmap [0,7] where the right-most pixel is denoted as x=0.
	  * @param dy    The vertical pixel in the bitmap [0,7] where the top-most pixel is denoted as y=0.
	  * @param bank  The VRAM bank number [0,1]
	  * @return      The color of the pixel in the range [0,3]
	  */
	unsigned char getBitmapPixel(const unsigned short &index, const unsigned char &dx, const unsigned char &dy, const unsigned char &bank=0);
	
	/** Draw a background tile.
	  * @param x The current LCD screen horizontal pixel [0,160).
	  * @param y The vertical pixel row of the tile to draw.
	  * @param offset The memory offset of the selected tilemap in VRAM.
	  * @param line Array of all pixels for the currnt scanline.
	  * @return The number of pixels drawn.
	  */
	unsigned char drawTile(const unsigned char &x, const unsigned char &y, 
	                       const unsigned char &x0, const unsigned char &y0,
	                       const unsigned short &offset, ColorGBC *line);

	/** Draw the current sprite.
	  * @param y The current LCD screen scanline [0,144).
	  * @param oam Pointer to the sprite handler with the currently selected sprite.
	  * @return Returns true if the current scanline passes through the sprite and return false otherwise.
	  */	
	bool drawSprite(const unsigned char &y, SpriteAttHandler *oam);
	
	/** Get the real RGB values for a 15-bit GBC format color.
	  * @param low The low byte (RED and lower 3 bits of GREEN) of the GBC color.
	  * @param high The high byte (upper 2 bits of GREEN and BLUE) of the GBC color.
	  */
	ColorRGB getColorRGB(const unsigned char &low, const unsigned char &high);
	
	/** Update true RGB background palette by converting GBC format colors.
	  * The color pointed to by the current bgPaletteIndex (register 0xFF68) will be udpated.
	  */
	void updateBackgroundPalette();
	
	/** Update true RGB sprite palette by converting GBC format colors.
	  * The color pointed to by the current objPaletteIndex (register 0xFF6B) will be udpated.
	  */
	void updateObjectPalette();
};

#endif
