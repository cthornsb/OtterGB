
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
	// Set default GB palette
	ngbcPaletteColor[0] = 0x0;
	ngbcPaletteColor[1] = 0x1;
	ngbcPaletteColor[2] = 0x2;
	ngbcPaletteColor[3] = 0x3;
	
	// Create a new window
	window = new Window(SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS);
#ifdef USE_OPENGL 
	// Create a link to the LCD driver
	window->setGPU(this);
#endif

	// 
	cmap = new CharacterMap();
	cmap->setWindow(window);
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
		for(int i = 0; i < 8; i++)
			for(int j = 0; j < 4; j++)
				bgPaletteColors[i][j] = Colors::WHITE;
	}
	else{ // Gameboy palettes
		bgPaletteColors[0][0] = Colors::GB_GREEN;
		bgPaletteColors[0][1] = (Colors::GB_LTGREEN);
		bgPaletteColors[0][2] = (Colors::GB_DKGREEN);
		bgPaletteColors[0][3] = (Colors::GB_DKSTGREEN);
	}

	for(int i = 0; i < 256; i++) // Blank the line buffer
		currentLineColors[i] = &bgPaletteColors[0][0];
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
                             const unsigned short &offset){
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
		bgBankNumber     = ((tileAttr & 0x8) == 0x8); // (0=Bank0, 1=Bank1)
		bgHorizontalFlip = ((tileAttr & 0x20) == 0x20); // (0=Normal, 1=HFlip)
		bgVerticalFlip   = ((tileAttr & 0x40) == 0x40); // (0=Normal, 1=VFlip)
		bgPriority       = ((tileAttr & 0x80) == 0x80); // (0=Use OAM, 1=BG Priority)
		if(bgVerticalFlip) // Vertical flip
			pixelY = 7 - pixelY;
	}
	
	// Draw the specified line
	for(unsigned char dx = 0; dx <= (7-pixelX); dx++){
		if(bGBCMODE){ // Gameboy Color palettes
			pixelColor = getBitmapPixel(bmpLow, (!bgHorizontalFlip ? (7-dx) : dx), pixelY, (bgBankNumber ? 1 : 0));
			currentLineColors[x+dx] = &bgPaletteColors[bgPaletteNumber][pixelColor];
		}
		else{ // Original gameboy palettes
			pixelColor = getBitmapPixel(bmpLow, (7-dx), pixelY);
			currentLineColors[x+dx] = &bgPaletteColors[0][ngbcPaletteColor[pixelColor]];
		}
	}
	
	// Return the number of pixels drawn
	return (7-pixelX)+1;
}

/** Draw the current sprite.
  * @param y The current LCD screen scanline [0,256).
  * @param oam Pointer to the sprite handler with the currently selected sprite.
  */
void GPU::drawSprite(const unsigned char &y, SpriteAttHandler *oam){
	unsigned char xp = oam->xPos-8+*rSCX; // Top left
	unsigned char yp = oam->yPos-16+*rSCY; // Top left

	// Check that the current scanline goes through the sprite
	if(y < yp || y >= yp+(!objSizeSelect ? 8 : 16))
		return;

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

	/*bool objPriority; // Object to Background priority (0: OBJ above BG, 1: OBJ behind BG color 1-3, BG color 0 always behind))*/

	// Draw the specified line
	for(unsigned short dx = 0; dx < 8; dx++){
		if(bGBCMODE){ // Gameboy Color sprite palettes (OBP0-7)
			pixelColor = getBitmapPixel(bmpLow, (!oam->xFlip ? (7-dx) : dx), pixelY, (oam->gbcVramBank ? 1 : 0));
			if(pixelColor != 0) // Check for transparent pixel
				currentLineColors[xp+dx] = &objPaletteColors[oam->gbcPalette][pixelColor];
		}
		else{ // Original gameboy sprite palettes (OBP0-1)
			pixelColor = getBitmapPixel(bmpLow, (!oam->xFlip ? (7-dx) : dx), pixelY);
			if(pixelColor != 0) // Check for transparent pixel
				currentLineColors[xp+dx] = &bgPaletteColors[0][(oam->ngbcPalette ? ngbcObj1PaletteColor[pixelColor] : ngbcObj0PaletteColor[pixelColor])];
		}
	}
}

void GPU::drawTileMaps(bool map1/*=false*/){
	unsigned char tileY, tileX;
	unsigned char pixelY, pixelX;
	unsigned char pixelColor;
	unsigned short bmpLow;
	for(unsigned short y = 0; y < 144; y++){ // Scanline (vertical pixel)
		for(unsigned short x = 0; x < 20; x++){ // Horizontal pixel
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
				window->setDrawColor(bgPaletteColors[0][ngbcPaletteColor[pixelColor]]);
				window->drawPixel(x*8+dx, y);
			}
		}
	}
}

void GPU::drawNextScanline(SpriteAttHandler *oam){
	// Check for LYC coincidence interrupts
	if(lycCoincIntEnable){
		if((*rLY) != (*rLYC))
			(*rSTAT) &= 0xFB; // Reset bit 2 of STAT (coincidence flag)
		else{ // LY == LYC
			(*rSTAT) |= 0x4; // Set bit 2 of STAT (coincidence flag)
			sys->handleLcdInterrupt();
		}
	}

	// Here (ry) is the real vertical coordinate on the background
	// and (rLY) is the current scanline.
	unsigned char ry = (*rLY) + (*rSCY);
	unsigned char rwy = (*rWY) + (*rSCY); // Real Y coordinate of the top left corner of the window
	unsigned char rwx = (*rWX-7) + (*rSCX); // Real X coordinate of the top left corner of the window
	
	if(((*rLCDC) & 0x80) == 0){ // Screen disabled (draw a "white" line)
		window->setDrawColor(Colors::WHITE);
		for(unsigned short x = 0; x < 160; x++){ // Horizontal pixel
			window->drawPixel(x, (*rLY));
		}
		return;
	}

	// Handle the background and window layer
	unsigned char rx = (*rSCX); // This will automatically handle screen wrapping
	for(unsigned short x = 0; x < 20; x++){
		// The window is visible if WX=[0,160) and WY=[0,144)
		// WX=7, WY=0 locates the window at the upper left of the screen
		if(winDisplayEnable && ((*rLY) >= (*rWY) && (*rSCX+8*x) >= rwx)) // Draw the window layer (if enabled)
			rx += drawTile(rx, ry, rwx, rwy, (winTileMapSelect ? 0x1C00 : 0x1800));
		else
			rx += drawTile(rx, ry, 0, 0, (bgTileMapSelect ? 0x1C00 : 0x1800));
	}

	// Handle the OBJ (sprite) layer
	if(objDisplayEnable){
		//int spritesDrawn = 0;
		bool visible = false;
		while(oam->getNextSprite(visible)){
			if(visible){ // The sprite is on screen
				drawSprite(ry, oam);
				//if(++spritesDrawn >= MAX_SPRITES_PER_LINE) // Max sprites per line
					//break;
			}
		}
	}
	
	// Render the current scanline
	rx = (*rSCX); // This will automatically handle screen wrapping
	for(unsigned short x = 0; x < 160; x++){ // Draw the scanline
		window->setDrawColor(currentLineColors[rx++]);
		window->drawPixel(x, (*rLY));
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
			(*rLCDC) = val;
			bgDisplayEnable    = (val & 0x1) != 0; // (0:off, 1:on)
			objDisplayEnable   = (val & 0x2) != 0; // (0:off, 1:on)
			objSizeSelect      = (val & 0x4) != 0; // (0:off, 1:on)
			bgTileMapSelect    = (val & 0x8) != 0; // (0:[9800,9BFF], 1-[9C00,9FFF])
			bgWinTileDataSelect = (val & 0x10) != 0; // (0:[9800,97FF], 1-[8000,8FFF])
			winDisplayEnable   = (val & 0x20) != 0; // (0:off, 1:on)
			winTileMapSelect   = (val & 0x40) != 0; // (0:[9800,9BFF], 1-[9C00,9FFF])
			lcdDisplayEnable   = (val & 0x80) != 0; // (0:off, 1:on);
			break;
		case 0xFF41: // STAT (LCDC Status Register)
			(*rSTAT) = val;
			lcdcStatusFlag      = (val & 0x3);      // Mode flag (see below) [read-only]
			coincidenceFlag     = (val & 0x4) != 0; // (0:LYC!=LY, 1:LYC==LY) [read-only]
			mode0IntEnable      = (val & 0x8) != 0; // (0:Disabled, 1:Enabled)
			mode1IntEnable      = (val & 0x10) != 0; // (0:Disabled, 1:Enabled)
			mode2IntEnable      = (val & 0x20) != 0; // (0:Disabled, 1:Enabled)
			lycCoincIntEnable   = (val & 0x40) != 0; // (0:Disabled, 1:Enabled)
			// STAT register mode flags:
			// 00 : The LCD controller is in the H-Blanking interval (OAM/RAM may be accessed)
			// 01 : The LCD controller is in the V-Blanking interval or display disabled (OAM/RAM may be accessed)
			// 10 : The LCD controller is reading from OAM memory (OAM may not be accessed)
			// 11 : The LCD controller is reading from both OAM and VRAM (OAM/RAM may not be accessed);
			break;
		case 0xFF42: // SCY (Scroll Y)
			(*rSCY) = val;
			break;
		case 0xFF43: // SCX (Scroll X)
			(*rSCX) = val;
			break;
		case 0xFF44: // LY (LCDC Y-coordinate) [read-only]
			// Represents the current vertical scanline being drawn in range [0,153].
			// The values 144-153 indicate the v-blank period. Writing to this register
			// will reset it.
			(*rLY) = 0x0;
			break;
		case 0xFF45: // LYC (LY Compare)
			// Sets the LY comparison value. When the LY and LYC registers are equal,
			// the 2nd bit (coincidence bit) of the STAT register is set and a STAT
			// interrupt is requested (if 6th bit of STAT set).
			(*rLYC) = val;
			break;
		case 0xFF47: // BGP (BG palette data, non-gbc mode only)
			// Non-GBC palette "Colors"
			// 00 : White
			// 01 : Light gray
			// 10 : Dark Gray
			// 11 : Black
			(*rBGP) = val;
			ngbcPaletteColor[0] = (val & 0x3); 
			ngbcPaletteColor[1] = (val & 0xC) >> 2; 
			ngbcPaletteColor[2] = (val & 0x30) >> 4; 
			ngbcPaletteColor[3] = (val & 0xC0) >> 6; 
			break;
		case 0xFF48: // OBP0 (Object palette 0 data, non-gbc mode only)
			// See BGP above
			(*rOBP0) = val;
			ngbcObj0PaletteColor[0] = 0x0; // Lower 2 bits not used, transparent for sprites
			ngbcObj0PaletteColor[1] = (val & 0xC) >> 2; 
			ngbcObj0PaletteColor[2] = (val & 0x30) >> 4; 
			ngbcObj0PaletteColor[3] = (val & 0xC0) >> 6; 
			break;
		case 0xFF49: // OBP1 (Object palette 1 data, non-gbc mode only)
			// See BGP above
			(*rOBP1) = val;
			ngbcObj1PaletteColor[0] = 0x0; // Lower 2 bits not used, transparent for sprites
			ngbcObj1PaletteColor[1] = (val & 0xC) >> 2; 
			ngbcObj1PaletteColor[2] = (val & 0x30) >> 4; 
			ngbcObj1PaletteColor[3] = (val & 0xC0) >> 6;
			break;
		case 0xFF4A: // WY (Window Y Position)
			(*rWY) = val;
			break;
		case 0xFF4B: // WX (Window X Position (minus 7))
			(*rWX) = val;
			break;
		case 0xFF4F: // VBK (VRAM bank select, gbc mode)
			(*rVBK) = val;
			bs = ((val & 0x1) != 0) ? 0x1 : 0x0;
			break;
		case 0xFF68: // BCPS/BGPI (Background palette index, gbc mode)
			(*rBGPI) = val;
			bgPaletteIndex        = (val & 0x3F); // Index in the BG palette byte array
			bgPaletteIndexAutoInc = ((val & 0x80) == 0x80); // Auto increment the BG palette byte index
			break;
		case 0xFF69: // BCPD/BGPD (Background palette data, gbc mode)
			(*rBGPD) = val;
			if(bgPaletteIndex > 0x3F)
				bgPaletteIndex = 0;
			bgPaletteData[bgPaletteIndex] = val;
			updateBackgroundPalette(); // Updated palette data, refresh the real RGB colors
			if(bgPaletteIndexAutoInc)
				bgPaletteIndex++;
			break;
		case 0xFF6A: // OCPS/OBPI (Sprite palette index, gbc mode)
			(*rOBPI) = val;
			objPaletteIndex        = (val & 0x3F); // Index in the OBJ (sprite) palette byte array
			objPaletteIndexAutoInc = ((val & 0x80) == 0x80); // Auto increment the OBJ (sprite) palette byte index
			break;
		case 0xFF6B: // OCPD/OBPD (Sprite palette index, gbc mode)
			(*rOBPD) = val;
			if(objPaletteIndex > 0x3F)
				objPaletteIndex = 0;
			objPaletteData[objPaletteIndex] = val;
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
			dest = (*rLCDC);
			break;
		case 0xFF41: // STAT (LCDC Status Register)
			dest = (*rSTAT);
			break;
		case 0xFF42: // SCY (Scroll Y)
			dest = (*rSCY);
			break;
		case 0xFF43: // SCX (Scroll X)
			dest = (*rSCX);
			break;
		case 0xFF44: // LY (LCDC Y-coordinate) [read-only]
			dest = (*rLY);
			break;
		case 0xFF45: // LYC (LY Compare)
			dest = (*rLYC);
			break;
		case 0xFF46: // DMA transfer from ROM/RAM to OAM
			dest = (*rDMA);
			break;
		case 0xFF47: // BGP (BG palette data, non-gbc mode only)
			dest = (*rBGP);
			break;
		case 0xFF48: // OBP0 (Object palette 0 data, non-gbc mode only)
			dest = (*rOBP0);
			break;
		case 0xFF49: // OBP1 (Object palette 1 data, non-gbc mode only)
			dest = (*rOBP1);
			break;
		case 0xFF4A: // WY (Window Y Position)
			dest = (*rWY);
			break;
		case 0xFF4B: // WX (Window X Position (minus 7))
			dest = (*rWX);
			break;
		case 0xFF4F: // VBK (VRAM bank select, gbc mode)
			dest = (*rVBK);
			break;
		case 0xFF68: // BCPS/BGPI (Background palette index, gbc mode)
			dest = (*rBGPI);
			break;
		case 0xFF69: // BCPD/BGPD (Background palette data, gbc mode)
			dest = (*rBGPD);
			break;
		case 0xFF6A: // OCPS/OBPI (Sprite palette index, gbc mode)
			dest = (*rOBPI);
			break;
		case 0xFF6B: // OCPD/OBPD (Sprite palette index, gbc mode)
			dest = (*rOBPD);
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
	bgPaletteColors[bgPaletteIndex/8][(bgPaletteIndex%8)/2] = getColorRGB(lowByte, highByte);
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
	objPaletteColors[objPaletteIndex/8][(objPaletteIndex%8)/2] = getColorRGB(lowByte, highByte);
}
