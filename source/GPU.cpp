
#include <iostream>
#include <stdlib.h>

#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Support.hpp"
#include "Graphics.hpp"
#include "Bitmap.hpp"
#include "GPU.hpp"

#define VRAM_LOW  0x8000
#define VRAM_HIGH 0xA000

#define OAM_TABLE_LOW  0xFE00
#define OAM_TABLE_HIGH 0xFEA0

#define MAX_SPRITES_PER_LINE 10

#define SCREEN_WIDTH_PIXELS 160
#define SCREEN_HEIGHT_PIXELS 144

const ColorGBC GBC_WHITE;

/////////////////////////////////////////////////////////////////////
// class SpriteAttHandler
/////////////////////////////////////////////////////////////////////

bool SpriteAttHandler::getNextSprite(bool &visible){
	if(index >= 40){
		index = 0;
		return false;
	}

	unsigned char *data = &mem[0][(index++)*4];
	yPos = data[0]; // Specifies the Y coord of the bottom right of the sprite
	xPos = data[1]; // Specifies the X coord of the bottom right of the sprite
	
	if((yPos == 0 || yPos >= 160) || (xPos == 0 || xPos >= 168)){
		visible = false;
		return true;
	}
	visible = true;
	
	tileNum = data[2]; // Specifies sprite's tile number from VRAM tile data [8000,8FFF]
	// Note: In 8x16 pixel sprite mode, the lower bit of the tile number is ignored.

	if(bGBCMODE){
		gbcPalette  = (data[3] & 0x7); // OBP0-7 (GBC only)
		gbcVramBank = (data[3] & 0x8) != 0; // [0:Bank0, 1:Bank1] (GBC only)
	}
	else
		ngbcPalette  = (data[3] & 0x10) != 0; // Non GBC mode only [0:OBP0, 1:OP1]
	xFlip       = (data[3] & 0x20) != 0;
	yFlip       = (data[3] & 0x40) != 0;
	objPriority = (data[3] & 0x80) != 0; // 0: Use OAM priority, 1: Use BG priority

	return true;
}

bool SpriteAttHandler::preWriteAction(){
	if(writeLoc >= OAM_TABLE_LOW && writeLoc < OAM_TABLE_HIGH)
		return true;

	// The OAM table has no associated registers, so return false
	// because we aren't in the OAM region of memory.
	return false;
}

/////////////////////////////////////////////////////////////////////
// class GPU
/////////////////////////////////////////////////////////////////////

GPU::GPU() : SystemComponent(8192, VRAM_LOW, 2) { // 2 8kB banks of VRAM
	// Set default GB palettes
	for(unsigned short i = 0; i < 3; i++){
		ngbcPaletteColor[i][0] = 0x0;
		ngbcPaletteColor[i][1] = 0x1;
		ngbcPaletteColor[i][2] = 0x2;
		ngbcPaletteColor[i][3] = 0x3;
	}
	
	// Create a new window
	window = new Window(SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS);
#ifdef USE_OPENGL 
	// Create a link to the LCD driver
	window->setGPU(this);
#endif

	// 
	cmap = new CharacterMap();
	cmap->setWindow(window);
	cmap->setTransparency(false);
}

GPU::~GPU(){
	delete window;
	delete cmap;
}

void GPU::initialize(){
	// Setup the window
	window->initialize();
	window->clear();

	// Set default palettes
	if(bGBCMODE){ // Gameboy Color palettes (all white at startup)
		for(int i = 0; i < 16; i++)
			for(int j = 0; j < 4; j++)
				gbcPaletteColors[i][j] = Colors::WHITE;
	}
	else{ // Gameboy palettes
		gbcPaletteColors[0][0] = Colors::GB_GREEN;
		gbcPaletteColors[0][1] = (Colors::GB_LTGREEN);
		gbcPaletteColors[0][2] = (Colors::GB_DKGREEN);
		gbcPaletteColors[0][3] = (Colors::GB_DKSTGREEN);
	}
}

/** Retrieve the color of a pixel in a tile bitmap.
  * @param index The index of the tile in VRAM [0x8000,0x8FFF].
  * @param dx    The horizontal pixel in the bitmap [0,7] where the right-most pixel is denoted as x=0.
  * @param dy    The vertical pixel in the bitmap [0,7] where the top-most pixel is denoted as y=0.
  * @param bank  The VRAM bank number [0,1]
  * @return      The color of the pixel in the range [0,3]
  */
unsigned char GPU::getBitmapPixel(const unsigned short &index, const unsigned char &dx, const unsigned char &dy, const unsigned char &bank/*=0*/){
	 // Retrieve a line of the bitmap at the requested pixel
	unsigned char pixelColor = (mem[bank][index+2*dy] & (0x1 << dx)) >> dx; // LS bit of the color
	pixelColor += ((mem[bank][index+2*dy+1] & (0x1 << dx)) >> dx) << 1; // MS bit of the color
	return pixelColor; // [0,3]
}

/** Draw a background or window tile.
  * @param x The current LCD screen horizontal pixel [0,256).
  * @param y The current LCD screen scanline [0,256).
  * @param offset The memory offset of the selected tilemap in VRAM.
  * @return The number of pixels drawn.
  */
unsigned char GPU::drawTile(const unsigned char &x, const unsigned char &y, 
                             const unsigned char &x0, const unsigned char &y0,
                             const unsigned short &offset, ColorGBC *line){
	unsigned char tileY, tileX;
	unsigned char pixelY, pixelX;
	unsigned char tileID;
	unsigned char tileAttr;
	unsigned char pixelColor;
	unsigned short bmpLow;
	
	if(y0 <= y){
		tileY = (y-y0) / 8; // Current vertical BG tile [0,32)
		pixelY = (y-y0) % 8; // Vertical pixel in the tile [0,8)
	}
	else{ // Screen wrap
		tileY = (0xFF-(y0-y)+1) / 8;
		pixelY = (0xFF-(y0-y)+1) % 8;
	}
	if(x0 <= x){
		tileX = (x-x0) / 8; // Current horizontal BG tile [0,32)
		pixelX = (x-x0) % 8; // Horizontal pixel in the tile [0,8)
	}
	else{ // Screen wrap
		tileX = (0xFF-(x0-x)+1) / 8;
		pixelX = (0xFF-(x0-x)+1) % 8;
	}
	
	// Draw the background tile
	// Background tile map selection (tile IDs) [0: 9800-9BFF, 1: 9C00-9FFF]
	// Background & window tile data selection [0: 8800-97FF, 1: 8000-8FFF]
	//  -> Indexing for 0:-128,127 1:0,255
	tileID = mem[0][offset + 32*tileY + tileX]; // Retrieve the background tile ID from VRAM
	
	 // Retrieve a line of the bitmap at the requested pixel
	if(bgWinTileDataSelect) // 0x8000-0x8FFF
		bmpLow = 16*tileID;
	else // 0x8800-0x97FF
		bmpLow = (0x1000 + 16*twosComp(tileID));

	// Background & window tile attributes
	unsigned char bgPaletteNumber;
	bool bgBankNumber;
	bool bgHorizontalFlip;
	bool bgVerticalFlip;
	bool bgPriority;
	if(bGBCMODE){
		tileAttr = mem[1][offset + 32*tileY + tileX]; // Retrieve the BG tile attributes
		bgPaletteNumber  = tileAttr & 0x7;
		bgBankNumber     = bitTest(tileAttr, 3); // (0=Bank0, 1=Bank1)
		bgHorizontalFlip = bitTest(tileAttr, 5); // (0=Normal, 1=HFlip)
		bgVerticalFlip   = bitTest(tileAttr, 6); // (0=Normal, 1=VFlip)
		bgPriority       = bitTest(tileAttr, 7); // (0=Use OAM, 1=BG Priority)
		if(bgVerticalFlip) // Vertical flip
			pixelY = 7 - pixelY;
	}
	
	// Draw the specified line
	unsigned char rx = x;
	for(unsigned char dx = pixelX; dx <= 7; dx++){
		if(bGBCMODE){ // Gameboy Color palettes
			pixelColor = getBitmapPixel(bmpLow, (!bgHorizontalFlip ? (7-dx) : dx), pixelY, (bgBankNumber ? 1 : 0));
			line[rx].setColorBG(pixelColor, bgPaletteNumber, bgPriority);
		}
		else{ // Original gameboy palettes
			pixelColor = getBitmapPixel(bmpLow, (7-dx), pixelY);
			line[rx].setColorBG(pixelColor, 0);
		}
		rx++;
	}
	
	// Return the number of pixels drawn
	return (7-pixelX)+1;
}

/** Draw the current sprite.
  * @param y The current LCD screen scanline [0,256).
  * @param oam Pointer to the sprite handler with the currently selected sprite.
  * @return Returns true if the current scanline passes through the sprite and return false otherwise.
  */
bool GPU::drawSprite(const unsigned char &y, SpriteAttHandler *oam){
	unsigned char xp = oam->xPos-8+rSCX->getValue(); // Top left
	unsigned char yp = oam->yPos-16+rSCY->getValue(); // Top left

	// Check that the current scanline goes through the sprite
	if(y < yp || y >= yp+(!objSizeSelect ? 8 : 16))
		return false;

	unsigned char pixelY = y - yp; // Vertical pixel in the tile
	unsigned char pixelColor;
	unsigned short bmpLow;
	
	// Retrieve the background tile ID from OAM
	// Tile map 0 is used (8000-8FFF)
	if(!objSizeSelect) // 8x8 pixel sprites
		bmpLow = 16*oam->tileNum;
	else if(pixelY <= 7) // Top half of 8x16 pixel sprites
		bmpLow = 16*(oam->tileNum & 0xFE);
	else{ // Bottom half of 8x16 pixel sprites
		bmpLow = 16*(oam->tileNum | 0x01); 
		pixelY -= 8;
	}

	if(oam->yFlip) // Vertical flip
		pixelY = 7 - pixelY;

	// Draw the specified line
	for(unsigned short dx = 0; dx < 8; dx++){
		if(bGBCMODE){ // Gameboy Color sprite palettes (OBP0-7)
			pixelColor = getBitmapPixel(bmpLow, (!oam->xFlip ? (7-dx) : dx), pixelY, (oam->gbcVramBank ? 1 : 0));
			currentLineSprite[xp].setColorOBJ(pixelColor, oam->gbcPalette+8, oam->objPriority);
		}
		else{ // Original gameboy sprite palettes (OBP0-1)
			pixelColor = getBitmapPixel(bmpLow, (!oam->xFlip ? (7-dx) : dx), pixelY);
			currentLineSprite[xp].setColorOBJ(pixelColor, (oam->ngbcPalette ? 2 : 1), oam->objPriority);
		}
		xp++;
	}
	
	return true;
}

void GPU::drawTileMaps(bool map1/*=false*/){
	unsigned char tileY, tileX;
	unsigned char pixelY, pixelX;
	unsigned char pixelColor;
	unsigned short bmpLow;
	for(unsigned short y = 0; y < 144; y++){ // Scanline (vertical pixel)
		for(unsigned short x = 0; x <= 20; x++){ // Horizontal tile
			tileY = y / 8; // Current vertical BG tile [0,32)
			pixelY = y % 8; // Vertical pixel in the tile [0,8)

			// Draw the background tile
			// Background tile map selection (tile IDs) [0: 9800-9BFF, 1: 9C00-9FFF]
			// Background & window tile data selection [0: 8800-97FF, 1: 8000-8FFF]
			//  -> Indexing for 0:-128,127 1:0,255
			if(!map1) // 0x8000-0x8FFF
				bmpLow = 16*(tileY*20+x);
			else // 0x8800-0x97FF
				bmpLow = 0x0800 + 16*(tileY*20+x);
			
			// Draw the specified line
			for(unsigned char dx = 0; dx <= 7; dx++){
				pixelColor = getBitmapPixel(bmpLow, (7-dx), pixelY);
				window->setDrawColor(gbcPaletteColors[0][pixelColor]);
				window->drawPixel(x*8+dx, y);
			}
		}
	}
}

void GPU::drawNextScanline(SpriteAttHandler *oam){
	// Here (ry) is the real vertical coordinate on the background
	// and (rLY) is the current scanline.
	unsigned char ry = rLY->getValue() + rSCY->getValue();
	
	if(!rLCDC->getBit(7)){ // Screen disabled (draw a "white" line)
		if(bGBCMODE)
			window->setDrawColor(Colors::WHITE);
		else
			window->setDrawColor(gbcPaletteColors[0][0]);
		window->drawLine(0, rLY->getValue(), 159, rLY->getValue());
		return;
	}

	unsigned char rx = rSCX->getValue(); // This will automatically handle screen wrapping
	for(unsigned short x = 0; x < 160; x++) // Reset the sprite line
		currentLineSprite[rx++].reset();

	// Handle the background layer
	rx = rSCX->getValue();
	if(bGBCMODE || bgDisplayEnable){ // Background enabled
		for(unsigned short x = 0; x <= 20; x++) // Draw the background layer
			rx += drawTile(rx, ry, 0, 0, (bgTileMapSelect ? 0x1C00 : 0x1800), currentLineBackground);
	}
	else{ // Background disabled (white)
		for(unsigned short x = 0; x < 160; x++) // Draw a "white" line
			currentLineBackground[rx++].reset();
	}

	// Handle the window layer
	bool windowVisible = false; // Is the window visible on this line?
	if(winDisplayEnable && (rLY->getValue() >= rWY->getValue())){
		unsigned char rwy = rWY->getValue() + rSCY->getValue(); // Real Y coordinate of the top left corner of the window
		unsigned char rwx = (rWX->getValue()-7) + rSCX->getValue(); // Real X coordinate of the top left corner of the window
		rx = rwx;
		for(unsigned short x = 0; x <= 20; x++){
			// The window is visible if WX=[0,167) and WY=[0,144)
			// WX=7, WY=0 locates the window at the upper left of the screen
			rx += drawTile(rx, ry, rwx, rwy, (winTileMapSelect ? 0x1C00 : 0x1800), currentLineWindow);
		}
		windowVisible = true;
	}

	// Handle the OBJ (sprite) layer
	if(objDisplayEnable){
		int spritesDrawn = 0;
		bool visible = false;
		oam->reset();
		while(oam->getNextSprite(visible)){
			if(visible){ // The sprite is on screen
				if(drawSprite(ry, oam) && ++spritesDrawn >= MAX_SPRITES_PER_LINE) // Max sprites per line
					break;
			}
		}
	}
	
	// Render the current scanline
	rx = rSCX->getValue(); // This will automatically handle screen wrapping
	ColorGBC *currentPixel;
	ColorRGB *currentPixelRGB; // Real RGB color of the current pixel
	unsigned char layerSelect = 0;
	for(unsigned short x = 0; x < 160; x++){ // Draw the scanline
		// Determine what layer should be drawn
		// CGB:
		//  LCDC bit 0 - 0=Sprites always on top of BG/WIN, 1=BG/WIN have priority
		//  Tile attr priority bit - 0=Use OAM priority bit, 1=BG Priority
		//  OAM sprite priority bit - 0=OBJ Above BG, 1=OBJ Behind BG color 1-3
		// DMG:
		//  LCDC bit 0 - 0=Off (white), 1=On
		//  OAM sprite priority bit - 0=OBJ Above BG, 1=OBJ Behind BG color 1-3
		// 
		if(bGBCMODE){
			if(bgDisplayEnable){ // BG/WIN priority
				if(currentLineBackground[rx].getPriority()){ // BG priority (tile attributes)
					if(windowVisible && x >= (rWX->getValue()-7)) // Draw window
						layerSelect = 1;
					else // Draw background
						layerSelect = 0;
				}
				else if(currentLineSprite[rx].visible()){ // Use OAM priority bit
					if(currentLineSprite[rx].getPriority() && currentLineBackground[rx].getColor()){ // OBJ behind BG color 1-3
						if(windowVisible && x >= (rWX->getValue()-7)) // Draw window
								layerSelect = 1;
							else // Draw background
								layerSelect = 0;
					}
					else // OBJ above BG (except for color 0 which is always transparent)
						layerSelect = 2;
				}
				else{ // Draw background or window
					if(windowVisible && x >= (rWX->getValue()-7)) // Draw window
						layerSelect = 1;
					else // Draw background
						layerSelect = 0;
				}
			}
			else{ // Sprites always on top (if visible)
				if(currentLineSprite[rx].visible()) // Draw sprite
					layerSelect = 2;
				else if(windowVisible && x >= (rWX->getValue()-7)) // Draw window
					layerSelect = 1;
				else // Draw background
					layerSelect = 0;
			}
		}
		else{
			if(currentLineSprite[rx].visible()){ // Use OAM priority bit
				if(currentLineSprite[rx].getPriority() && currentLineBackground[rx].getColor()){ // OBJ behind BG color 1-3
					if(windowVisible && x >= (rWX->getValue()-7)) // Draw window
							layerSelect = 1;
						else // Draw background
							layerSelect = 0;
				}
				else // OBJ above BG (except for color 0 which is always transparent)
					layerSelect = 2;
			}
			else if(windowVisible && x >= (rWX->getValue()-7)) // Draw window
				layerSelect = 1;
			else // Draw background
				layerSelect = 0;
		}
		switch(layerSelect){
			case 0: // Draw background
				currentPixel = &currentLineBackground[rx];
				break;
			case 1: // Draw window
				currentPixel = &currentLineWindow[rx];
				break;
			case 2: // Draw sprite
				currentPixel = &currentLineSprite[rx];
				break;
			default:
				break;
		}
		if(bGBCMODE)
			currentPixelRGB = &gbcPaletteColors[currentPixel->getPalette()][currentPixel->getColor()];
		else
			currentPixelRGB = &gbcPaletteColors[0][ngbcPaletteColor[currentPixel->getPalette()][currentPixel->getColor()]];
		window->setDrawColor(currentPixelRGB);
		window->drawPixel(x, rLY->getValue());
		rx++;
	}
}

void GPU::render(){	
	// Update the screen
	if(lcdDisplayEnable && window->status()){ // Check for events
		window->render();
	}
}

bool GPU::getWindowStatus(){
	return window->status();
}

void GPU::setPixelScale(const unsigned int &n){
	window->setScalingFactor(n);
}

void GPU::print(const std::string &str, const unsigned char &x, const unsigned char &y){
	cmap->putString(str, x, y);
}

bool GPU::preWriteAction(){
	if(writeLoc >= VRAM_LOW && writeLoc < VRAM_HIGH)
		return true;

	// Tile Data	
	// Tiles are 8x8
	// Tiles occupy 16 bytes, 2 bytes per line
	// VRAM contains two sets of 192 tiles at 8000:8FFF and 8800:97FF
	// The first tile set can be used for BG and Sprites and the 2nd for BG and Window
	// For each line, the first byte defines the LSB of the color number and the second byte the MSB

	return false;
} 

bool GPU::preReadAction(){
	if(readLoc >= VRAM_LOW && readLoc < VRAM_HIGH)
		return true;

	return false;
} 

bool GPU::writeRegister(const unsigned short &reg, const unsigned char &val){
	switch(reg){
		case 0xFF40: // LCDC (LCD Control Register)
			bgDisplayEnable     = rLCDC->getBit(0); // (0:off, 1:on)
			objDisplayEnable    = rLCDC->getBit(1); // (0:off, 1:on)
			objSizeSelect       = rLCDC->getBit(2); // (0:off, 1:on)
			bgTileMapSelect     = rLCDC->getBit(3); // (0:[9800,9BFF], 1-[9C00,9FFF])
			bgWinTileDataSelect = rLCDC->getBit(4); // (0:[9800,97FF], 1-[8000,8FFF])
			winDisplayEnable    = rLCDC->getBit(5); // (0:off, 1:on)
			winTileMapSelect    = rLCDC->getBit(6); // (0:[9800,9BFF], 1-[9C00,9FFF])
			lcdDisplayEnable    = rLCDC->getBit(7); // (0:off, 1:on);
			if(!lcdDisplayEnable) // LY is reset if LCD goes from on to off
				sys->getClock()->resetScanline();
			break;
		case 0xFF41: // STAT (LCDC Status Register)
			break;
		case 0xFF42: // SCY (Scroll Y)
			break;
		case 0xFF43: // SCX (Scroll X)
			break;
		case 0xFF44: // LY (LCDC Y-coordinate) [read-only]
			// Represents the current vertical scanline being drawn in range [0,153].
			// The values 144-153 indicate the v-blank period. Writing to this register
			// will reset it.
			sys->getClock()->resetScanline();
			break;
		case 0xFF45: // LYC (LY Compare)
			// Sets the LY comparison value. When the LY and LYC registers are equal,
			// the 2nd bit (coincidence bit) of the STAT register is set and a STAT
			// interrupt is requested (if 6th bit of STAT set).
			break;
		case 0xFF47: // BGP (BG palette data, non-gbc mode only)
			// Non-GBC palette "Colors"
			// 00 : White
			// 01 : Light gray
			// 10 : Dark Gray
			// 11 : Black
			ngbcPaletteColor[0][0] = rBGP->getBits(0,1);
			ngbcPaletteColor[0][1] = rBGP->getBits(2,3);
			ngbcPaletteColor[0][2] = rBGP->getBits(4,5);
			ngbcPaletteColor[0][3] = rBGP->getBits(6,7); 
			break;
		case 0xFF48: // OBP0 (Object palette 0 data, non-gbc mode only)
			// See BGP above
			ngbcPaletteColor[1][0] = 0x0; // Lower 2 bits not used, transparent for sprites
			ngbcPaletteColor[1][1] = rOBP0->getBits(2,3); 
			ngbcPaletteColor[1][2] = rOBP0->getBits(4,5); 
			ngbcPaletteColor[1][3] = rOBP0->getBits(6,7); 
			break;
		case 0xFF49: // OBP1 (Object palette 1 data, non-gbc mode only)
			// See BGP above
			ngbcPaletteColor[2][0] = 0x0; // Lower 2 bits not used, transparent for sprites
			ngbcPaletteColor[2][1] = rOBP1->getBits(2,3);
			ngbcPaletteColor[2][2] = rOBP1->getBits(4,5);
			ngbcPaletteColor[2][3] = rOBP1->getBits(6,7);
			break;
		case 0xFF4A: // WY (Window Y Position)
			break;
		case 0xFF4B: // WX (Window X Position (minus 7))
			break;
		case 0xFF4F: // VBK (VRAM bank select, gbc mode)
			bs = rVBK->getBit(0) ? 1 : 0;
			break;
		case 0xFF68: // BGPI (Background palette index, gbc mode)
			bgPaletteIndex        = rBGPI->getBits(0,5); // Index in the BG palette byte array
			bgPaletteIndexAutoInc = rBGPI->getBit(7); // Auto increment the BG palette byte index
			break;
		case 0xFF69: // BGPD (Background palette data, gbc mode)
			if(bgPaletteIndex > 0x3F)
				bgPaletteIndex = 0;
			bgPaletteData[bgPaletteIndex] = rBGPD->getValue();
			updateBackgroundPalette(); // Updated palette data, refresh the real RGB colors
			if(bgPaletteIndexAutoInc)
				bgPaletteIndex++;
			break;
		case 0xFF6A: // OBPI (Sprite palette index, gbc mode)
			objPaletteIndex        = rOBPI->getBits(0,5); // Index in the OBJ (sprite) palette byte array
			objPaletteIndexAutoInc = rOBPI->getBit(7); // Auto increment the OBJ (sprite) palette byte index
			break;
		case 0xFF6B: // OBPD (Sprite palette index, gbc mode)
			if(objPaletteIndex > 0x3F)
				objPaletteIndex = 0;
			objPaletteData[objPaletteIndex] = rOBPD->getValue();
			updateObjectPalette(); // Updated palette data, refresh the real RGB colors
			if(objPaletteIndexAutoInc)
				objPaletteIndex++;
			break;
		default:
			return false;
	}
	return true;
}

bool GPU::readRegister(const unsigned short &reg, unsigned char &dest){
	switch(reg){
		case 0xFF40: // LCDC (LCD Control Register)
			break;
		case 0xFF41: // STAT (LCDC Status Register)
			break;
		case 0xFF42: // SCY (Scroll Y)
			break;
		case 0xFF43: // SCX (Scroll X)
			break;
		case 0xFF44: // LY (LCDC Y-coordinate) [read-only]
			break;
		case 0xFF45: // LYC (LY Compare)
			break;
		case 0xFF46: // DMA transfer from ROM/RAM to OAM
			break;
		case 0xFF47: // BGP (BG palette data, non-gbc mode only)
			break;
		case 0xFF48: // OBP0 (Object palette 0 data, non-gbc mode only)
			break;
		case 0xFF49: // OBP1 (Object palette 1 data, non-gbc mode only)
			break;
		case 0xFF4A: // WY (Window Y Position)
			break;
		case 0xFF4B: // WX (Window X Position (minus 7))
			break;
		case 0xFF4F: // VBK (VRAM bank select, gbc mode)
			break;
		case 0xFF68: // BCPS/BGPI (Background palette index, gbc mode)
			break;
		case 0xFF69: // BCPD/BGPD (Background palette data, gbc mode)
			break;
		case 0xFF6A: // OCPS/OBPI (Sprite palette index, gbc mode)
			break;
		case 0xFF6B: // OCPD/OBPD (Sprite palette index, gbc mode)
			break;
		default:
			return false;
	}
	return true;
}

/** Get the real RGB values for a 15-bit GBC format color.
  * @param low The low byte (RED and lower 3 bits of GREEN) of the GBC color.
  * @param high The high byte (upper 2 bits of GREEN and BLUE) of the GBC color.
  */
ColorRGB GPU::getColorRGB(const unsigned char &low, const unsigned char &high){
	unsigned char r = low & 0x1F;
	unsigned char g = ((low & 0xE0) >> 5) + ((high & 0x3) << 3);
	unsigned char b = (high & 0x7C) >> 2;
	return ColorRGB(r/31.0, g/31.0, b/31.0);
}

/** Update true RGB background palette by converting GBC format colors.
  * The color pointed to by the current bgPaletteIndex (register 0xFF68) will be udpated.
  */
void GPU::updateBackgroundPalette(){
	unsigned char lowByte, highByte;
	if((bgPaletteIndex % 2) == 1){ // Odd, MSB of a color pair
		lowByte = bgPaletteData[bgPaletteIndex-1];
		highByte = bgPaletteData[bgPaletteIndex];
	}
	else{ // Even, LSB of a color pair
		lowByte = bgPaletteData[bgPaletteIndex];
		highByte = bgPaletteData[bgPaletteIndex+1];
	}
	gbcPaletteColors[bgPaletteIndex/8][(bgPaletteIndex%8)/2] = getColorRGB(lowByte, highByte);
}

/** Update true RGB sprite palette by converting GBC format colors.
  * The color pointed to by the current objPaletteIndex (register 0xFF6A) will be updated.
  */
void GPU::updateObjectPalette(){
	unsigned char lowByte, highByte;
	if((objPaletteIndex % 2) == 1){ // Odd, MSB of a color pair
		lowByte = objPaletteData[objPaletteIndex-1];
		highByte = objPaletteData[objPaletteIndex];	
	}
	else{ // Even, LSB of a color pair
		lowByte = objPaletteData[objPaletteIndex];
		highByte = objPaletteData[objPaletteIndex+1];	
	}
	gbcPaletteColors[objPaletteIndex/8+8][(objPaletteIndex%8)/2] = getColorRGB(lowByte, highByte);
}

void GPU::defineRegisters(){
	sys->addSystemRegister(this, 0x40, rLCDC, "LCDC", "33333333");
	sys->addSystemRegister(this, 0x41, rSTAT, "STAT", "11133330");
	sys->addSystemRegister(this, 0x42, rSCY,  "SCY",  "33333333");
	sys->addSystemRegister(this, 0x43, rSCX,  "SCX",  "33333333");
	sys->addSystemRegister(this, 0x44, rLY,   "LY",   "11111111");
	sys->addSystemRegister(this, 0x45, rLYC,  "LYC",  "33333333");
	sys->addSystemRegister(this, 0x47, rBGP,  "BGP",  "33333333");
	sys->addSystemRegister(this, 0x48, rOBP0, "OBP0", "33333333");
	sys->addSystemRegister(this, 0x49, rOBP1, "OBP1", "33333333");
	sys->addSystemRegister(this, 0x4A, rWY,   "WY",   "33333333");
	sys->addSystemRegister(this, 0x4B, rWX,   "WX",   "33333333");
	sys->addSystemRegister(this, 0x4F, rVBK,  "VBK",  "30000000");
	sys->addSystemRegister(this, 0x68, rBGPI, "BGPI", "33333303");
	sys->addSystemRegister(this, 0x69, rBGPD, "BGPD", "33333333");
	sys->addSystemRegister(this, 0x6A, rOBPI, "OBPI", "33333303");
	sys->addSystemRegister(this, 0x6B, rOBPD, "OBPD", "33333333");
}
