#ifndef GPU_HPP
#define GPU_HPP

#include <string>
#include <memory>

#include "colors.hpp"
#include "ColorGBC.hpp"
#include "SystemComponent.hpp"
#include "SpriteAttributes.hpp"

class Register;
class Window;
class ConsoleGBC;

class GPU : public SystemComponent {
public:
	GPU();

	~GPU();
	
	void initialize();

	void drawConsole();

	void drawTileMaps(Window *win);

	void drawLayer(Window *win, bool mapSelect=true);

	void disableRenderLayer(const unsigned char &layer){ userLayerEnable[layer] = false; }
	
	void enableRenderLayer(const unsigned char &layer){ userLayerEnable[layer] = true; }

	void drawNextScanline(SpriteHandler *oam);

	void render();

	void processEvents();

	Window *getWindow(){ return window.get(); }

	bool getWindowStatus();

	unsigned char getDmgPaletteColorHex(const unsigned short &index) const ;

	unsigned short getBgPaletteColorHex(const unsigned short &index) const ;

	unsigned short getObjPaletteColorHex(const unsigned short &index) const ;
	
	void setPixelScale(const unsigned int &n);

	void print(const std::string &str, const unsigned char &x, const unsigned char &y);

	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;
	
	bool readRegister(const unsigned short &reg, unsigned char &val) override ;

	void defineRegisters() override ;

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

	std::unique_ptr<Window> window; ///< Pointer to the main renderer window
	
	std::unique_ptr<ConsoleGBC> console; ///< Pointer to the console object used for printing text.
	
	ColorGBC currentLineSprite[256];
	ColorGBC currentLineWindow[256];
	ColorGBC currentLineBackground[256];

	bool userLayerEnable[3]; ///< Flags for the three render layers.

	std::vector<SpriteAttributes> sprites; ///< List of all currently active sprites

	/** Retrieve the color of a pixel in a tile bitmap.
	  * @param index The start address of the tile in VRAM [0x0000,0x1800].
	  * @param dx    The horizontal pixel in the bitmap [0,7] where the right-most pixel is denoted as x=0.
	  * @param dy    The vertical pixel in the bitmap [0,7] where the top-most pixel is denoted as y=0.
	  * @param bank  The VRAM bank number [0,1]
	  * @return      The color of the pixel in the range [0,3]
	  */
	unsigned char getBitmapPixel(const unsigned short &index, const unsigned char &dx, const unsigned char &dy, const unsigned char &bank=0);
	
	/** Draw a background tile.
	  * @param x The current LCD screen horizontal pixel [0,160).
	  * @param y The vertical pixel row of the tile to draw.
	  * @param x0 Horizontal pixel offset in the layer.
	  * @param offset The memory offset of the selected tilemap in VRAM.
	  * @param line Array of all pixels for the currnt scanline.
	  * @return The number of pixels drawn.
	  */
	unsigned char drawTile(const unsigned char &x, const unsigned char &y, const unsigned char &x0, const unsigned short &offset, ColorGBC *line);

	/** Draw the current sprite.
	  * @param y The current LCD screen scanline [0,144).
	  * @param oam The currently selected sprite to draw.
	  * @return Returns true if the current scanline passes through the sprite and return false otherwise.
	  */	
	bool drawSprite(const unsigned char &y, const SpriteAttributes &oam);
	
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
	
	bool checkWindowVisible();
	
	/** Add elements to a list of values which will be written to / read from an emulator savestate
	  */
	void userAddSavestateValues() override;
};

#endif
