#ifndef GPU_HPP
#define GPU_HPP

#include <string>
#include <memory>
#include <mutex>

#include "ColorRGB.hpp"
#include "ColorGBC.hpp"
#include "SystemComponent.hpp"
#include "SpriteAttributes.hpp"
#include "ComponentThread.hpp"

class Register;
class OTTWindow;
class ConsoleGBC;

enum class PPUMODE{
	NONE, // Do nothing
	SCANLINE, // Render the next scanline
	DRAWBUFFER // Draw the image buffer to the screen
};

class GPU : public SystemComponent, public WorkerThread {
public:
	/** Default constructor
	  */
	GPU();

	/** Destructor
	  */
	~GPU();
	
	/** Initialize GPU and output window (LCD)
	  */
	void initialize();

	/** Update the interpreter console and draw it to the screen
	  */
	void drawConsole();

	/** Draw both VRAM tilemaps (0x8000 and 0x9800) in an external window
	  */
	void drawTileMaps(OTTWindow *win);

	/** Draw one of the drawing layers in an external window
	  * @param mapSelect If set to true, selects VRAM tile map at address 0x9C00, else selects map at 0x9800
	  */
	void drawLayer(OTTWindow *win, bool mapSelect=true);

	/** Disable one of the three drawing layers
	  * Layers are still rendered internally, but they are not drawn to the screen
	  *  0: Background layer
	  *  1: Window layer
	  *  2: Sprites layer
	  */
	void disableRenderLayer(const unsigned char &layer){ 
		if(layer <= 3)
			userLayerEnable[layer] = false; 
	}
	
	/** Enable one of the three drawing layers
	  * Layers are still rendered internally, but they are not drawn to the screen
	  *  0: Background layer
	  *  1: Window layer
	  *  2: Sprites layer
	  */
	void enableRenderLayer(const unsigned char &layer){ 
		if(layer <= 3)
			userLayerEnable[layer] = true; 
	}

	/** Draw the next LCD scanline
	  * @param oam Pointer to the system sprite manager
	  * @return The number of ticks to delay the 4 MHz pixel clock due to sprite rendering
	  */
	unsigned short drawNextScanline(SpriteHandler *oam);

	/** Draw the current screen buffer
	  */
	void render();

	/** Process OpenGL window events
	  */
	void processEvents();

	/** Get pointer to the graphical output window
	  */
	OTTWindow *getWindow(){ 
		return window.get(); 
	}

	/** Get the status of the OpenGL window
	  */
	bool getWindowStatus();
	
	/** Return true if the user has specified a DMG color palette to use for DMG games
	  */
	bool isUserPaletteSet() const {
		return bUserSelectedPalette;
	}

	/** Get the number of sprites drawn on the most recent LCD scanline
	  */
	unsigned char getSpritesDrawn() const {
		return nSpritesDrawn;
	}

	/** Get a DMG color code from DMG palette data array
	  */
	unsigned char getDmgPaletteColorHex(const unsigned short &index) const ;

	/** Get a 15-bit color (5-bit RGB components) from sprite palette data array
	  */
	unsigned short getBgPaletteColorHex(const unsigned short &index) const ;

	/** Get a 15-bit color (5-bit RGB components) from sprite palette data array
	  */
	unsigned short getObjPaletteColorHex(const unsigned short &index) const ;
	
	/** Set the OpenGL pixel scaling factor
	  */
	void setPixelScale(const unsigned int &n);

	/** Set default green DMG mode background and sprite color palettes
	  */
	void setColorPaletteDMG();

	/** Set DMG mode background and sprite color palettes using a pre-defined CGB palette
	  * @param paletteID Pre-defined CGB palette ID number (MSB table number, LSB entry number)
	  */
	void setColorPaletteDMG(const unsigned short& paletteID);

	/** Set DMG mode background and sprite color palettes
	  */
	void setColorPaletteDMG(const ColorRGB c0, const ColorRGB c1, const ColorRGB c2, const ColorRGB c3);

	/** User-set color palette will be cleared on next emulator reset
	  */
	void disableUserPalette(){
		bUserSelectedPalette = false;
	}

	/** Set current PPU operation mode
	  * The new operation will be performed the next time PPU is notified by thread handler.
	  */
	void setOperationMode(const PPUMODE& newMode);
	
	/** Print a string to the interpreter console
	  */
	void print(const std::string &str, const unsigned char &x, const unsigned char &y);

	/** Write to a GPU register
	  */
	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;
	
	/** Read a GPU register
	  */
	bool readRegister(const unsigned short &reg, unsigned char &val) override ;

	/** Define all GPU system registers
	  */
	void defineRegisters() override ;

	/** Read settings from an input user configuration file
	  */
	void readConfigFile(ConfigFile* config) override;

private:
	bool bUserSelectedPalette; ///< Set if user has specified a DMG color palette to use for DMG games

	bool bWindowVisible; ///< Set if window is present on current scanline

	bool winDisplayEnable; ///< Set if the window layer is enabled and is on screen

	unsigned char nScanline; ///< Current LCD scanline
	
	unsigned char nPosX; ///< Real horizontal position of current pixel on the background layer
	
	unsigned char nPosY; ///< Real vertical position of current pixel on the background layer

	unsigned char nSpritesDrawn; ///< Number of sprites drawn on the most recent scanline

	unsigned char bgPaletteIndex; ///< Current index in the background palette data array

	unsigned char objPaletteIndex; ///< Current index in the sprite palette data array

	unsigned char dmgPaletteColor[3][4]; ///< Original GB background palettes

	unsigned char bgPaletteData[64]; ///< GBC background palette 0-7

	unsigned char objPaletteData[64]; ///< GBC sprite palette 0-7

	ColorRGB cgbPaletteColor[16][4]; ///< RGB colors for GBC background and sprite palettes 0-7

	std::unique_ptr<OTTWindow> window; ///< Pointer to the main renderer window
	
	std::unique_ptr<ConsoleGBC> console; ///< Pointer to the console object used for printing text.
	
	ColorGBC currentLineSprite[256]; ///< Pixel color and palette information for the current sprite layer scanline
	
	ColorGBC currentLineWindow[256]; ///< Pixel color and palette information for the current window layer scanline
	
	ColorGBC currentLineBackground[256]; ///< Pixel color and palette information for the current background layer scanline

	bool userLayerEnable[3]; ///< Flags for the three render layers.

	std::vector<SpriteAttributes> sprites; ///< List of all currently active sprites

	std::mutex buffLock; ///< Image buffer mutex lock

	PPUMODE mode;

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

	/** Render the current scanline, pushing pixel data into the window image buffer
	  */
	void renderScanline();
	
	/** Get the real RGB values for a 15-bit GBC format color.
	  * @param low The low byte (RED and lower 3 bits of GREEN) of the GBC color.
	  * @param high The high byte (upper 2 bits of GREEN and BLUE) of the GBC color.
	  */
	ColorRGB getColorRGB(const unsigned char &low, const unsigned char &high);

	/** Get pointer to one of the four colors of one of the CGB palettes
	  * For DMG mode, palette 0 is used for BGP and palettes 8 and 9 are used for OBP0 and OBP1 respectively.
	  */
	ColorRGB& getPaletteColor(const unsigned char& palette, const unsigned char& color);
	
	/** Update true RGB background palette by converting GBC format colors.
	  * The color pointed to by the current bgPaletteIndex (register 0xFF68) will be udpated.
	  */
	void updateBackgroundPalette();
	
	/** Update true RGB sprite palette by converting GBC format colors.
	  * The color pointed to by the current objPaletteIndex (register 0xFF6B) will be udpated.
	  */
	void updateObjectPalette();
	
	/** Return true if the window layer is enabled and is on screen
	  */
	bool checkWindowVisible();
	
	/** Write pixel color data directly to output image buffer
	  * Mutex lock protected.
	  */
	void writeImageBuffer(const unsigned short& x, const unsigned short& y, const ColorRGB& color);
	
	/** Write a line of pixels directly to output image buffer
	  * Mutex lock protected.
	  */
	void writeImageBuffer(const unsigned short& y, const ColorRGB& color);
	
	/** Add elements to a list of values which will be written to / read from an emulator savestate
	  */
	void userAddSavestateValues() override;
	
	/** Reset all color palettes to startup values
	  */
	void onUserReset() override;
	
	/** Main graphical loop
	  */
	void mainLoop() override;
};

#endif
