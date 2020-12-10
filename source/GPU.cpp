
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdlib.h>

#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Support.hpp"
#include "Graphics.hpp"
#include "Console.hpp"
#include "GPU.hpp"
#include "SystemTimer.hpp"

constexpr unsigned short VRAM_LOW  = 0x8000;
constexpr unsigned short VRAM_HIGH = 0xA000;

constexpr unsigned short OAM_TABLE_LOW  = 0xFE00;
constexpr unsigned short OAM_TABLE_HIGH = 0xFEA0;

constexpr int MAX_SPRITES_PER_LINE = 10;

constexpr int SCREEN_WIDTH_PIXELS  = 160;
constexpr int SCREEN_HEIGHT_PIXELS = 144;

/////////////////////////////////////////////////////////////////////
// class SpriteAttributes
/////////////////////////////////////////////////////////////////////

bool SpriteAttributes::compareDMG(const SpriteAttributes &s1, const SpriteAttributes &s2){
	// When sprites with differing xPos overlap, the one with the smaller
	// xPos will have priority and will appear above the other. 
	if(s1.xPos != s2.xPos)
		return (s1.xPos < s2.xPos);
	// If it's the same, priority is assigned based on table ordering.
	return (s1.oamIndex < s2.oamIndex);
}

bool SpriteAttributes::compareCGB(const SpriteAttributes &s1, const SpriteAttributes &s2){
	// Sprite priority is assigned based on table ordering.
	return (s1.oamIndex < s2.oamIndex);
}

/////////////////////////////////////////////////////////////////////
// class SpriteHandler
/////////////////////////////////////////////////////////////////////

SpriteHandler::SpriteHandler() : SystemComponent("OAM", 160) { 
	reset();
}

bool SpriteHandler::updateNextSprite(std::vector<SpriteAttributes> &sprites){
	if(lModified.empty())
		return false;

	unsigned char spriteIndex = lModified.front();
	bModified[spriteIndex] = false;
	lModified.pop();

	// Search for the sprite in the list of active sprites
	std::vector<SpriteAttributes>::iterator iter = std::find(sprites.begin(), sprites.end(), spriteIndex);
	
	unsigned char *data = &mem[0][spriteIndex*4];
	if((data[0] == 0 || data[0] >= 160) || (data[1] == 0 || data[1] >= 168)){
		if(iter != sprites.end()) // Remove the sprite from the list
			sprites.erase(iter);
		return true; // Sprite is hidden.
	}

	SpriteAttributes *current;
	if(iter != sprites.end()) // Get a pointer to an existing sprite
		current = &(*iter);
	else{ // Add the sprite to the list
		sprites.push_back(SpriteAttributes());
		current = &sprites.back();
	}
	
	// Decode the sprite attributes.
	getSpriteData(data, current);
	
	// Set the sprite index. 
	current->oamIndex = spriteIndex;
	
	return true;
}

bool SpriteHandler::preWriteAction(){
	// The OAM table has no associated registers, so return false
	// because we aren't in the OAM region of memory.
	if(writeLoc < OAM_TABLE_LOW && writeLoc > OAM_TABLE_HIGH)
		return false;

	unsigned short spriteIndex = (writeLoc-OAM_TABLE_LOW)/4;
	if(!bModified[spriteIndex]){
		lModified.push((unsigned char)spriteIndex);
		bModified[spriteIndex] = true;
	}

	return true;
}

SpriteAttributes SpriteHandler::getSpriteAttributes(const unsigned char &index){
	unsigned char *data = &mem[0][index*4];
	SpriteAttributes attr;
	getSpriteData(data, &attr);
	attr.oamIndex = index;
	return attr;
}

void SpriteHandler::reset(){
	for(unsigned short i = 0; i < 40; i++)
		bModified[i] = false;
	while(!lModified.empty())
		lModified.pop();
}

void SpriteHandler::getSpriteData(unsigned char *ptr, SpriteAttributes *attr){
	attr->yPos = ptr[0]; // Specifies the Y coord of the bottom right of the sprite
	attr->xPos = ptr[1]; // Specifies the X coord of the bottom right of the sprite
	
	attr->tileNum = ptr[2]; // Specifies sprite's tile number from VRAM tile data [8000,8FFF]
	// Note: In 8x16 pixel sprite mode, the lower bit of the tile number is ignored.

	if(bGBCMODE){
		attr->gbcPalette  = (ptr[3] & 0x7); // OBP0-7 (GBC only)
		attr->gbcVramBank = bitTest(ptr[3], 3); // [0:Bank0, 1:Bank1] (GBC only)
	}
	else
		attr->ngbcPalette  = bitTest(ptr[3], 4); // Non GBC mode only [0:OBP0, 1:OP1]
	attr->xFlip       = bitTest(ptr[3], 5);
	attr->yFlip       = bitTest(ptr[3], 6);
	attr->objPriority = bitTest(ptr[3], 7); // 0: Use OAM priority, 1: Use BG priority
}

/////////////////////////////////////////////////////////////////////
// class GPU
/////////////////////////////////////////////////////////////////////

GPU::GPU() : SystemComponent("GPU", 8192, VRAM_LOW, 2) { // 2 8kB banks of VRAM
}

GPU::~GPU(){
}

void GPU::initialize(){
	// Set default GB palettes
	for(unsigned short i = 0; i < 3; i++){
		ngbcPaletteColor[i][0] = 0x0;
		ngbcPaletteColor[i][1] = 0x1;
		ngbcPaletteColor[i][2] = 0x2;
		ngbcPaletteColor[i][3] = 0x3;
		userLayerEnable[i] = true;
	}
	
	// Create a new window
	window = std::unique_ptr<Window>(new Window(SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS));
#ifdef USE_OPENGL 
	// Create a link to the LCD driver
	window->setGPU(this);
#endif

	// Setup the ascii character map for text output
	console = std::unique_ptr<ConsoleGBC>(new ConsoleGBC());
	console->setWindow(window.get());
	console->setSystem(sys);
	console->setTransparency(false);

	// Setup the window
	window->initialize();
	window->setupKeyboardHandler();
	window->clear();

	// Set default palettes
	if(bGBCMODE){ // Gameboy Color palettes (all white at startup)
		for(int i = 0; i < 16; i++)
			for(int j = 0; j < 4; j++)
				gbcPaletteColors[i][j] = Colors::WHITE;
	}
	else{ // Gameboy palettes
		gbcPaletteColors[0][0] = Colors::GB_GREEN;
		gbcPaletteColors[0][1] = Colors::GB_LTGREEN;
		gbcPaletteColors[0][2] = Colors::GB_DKGREEN;
		gbcPaletteColors[0][3] = Colors::GB_DKSTGREEN;
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

/** Draw a background tile.
  * @param x The current horizontal pixel [0,256).
  * @param y The vertical pixel row of the tile to draw.
  * @param x0 Horizontal pixel offset in the layer.
  * @param offset The memory offset of the selected tilemap in VRAM.
  * @param line Array of all pixels for the current scanline.
  * @return The number of pixels drawn.
  */
unsigned char GPU::drawTile(const unsigned char &x, const unsigned char &y, const unsigned char &x0,
                            const unsigned short &offset, ColorGBC *line){
	unsigned char tileY, tileX;
	unsigned char pixelY, pixelX;
	unsigned char tileID;
	unsigned char tileAttr;
	unsigned char pixelColor;
	unsigned short bmpLow;
	
	tileY = y / 8; // Current vertical BG tile [0,32)
	pixelY = y % 8; // Vertical pixel in the tile [0,8)

	tileX = (x-x0) / 8; // Current horizontal BG tile [0,32)
	pixelX = (x-x0) % 8; // Horizontal pixel in the tile [0,8)
	
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
  * @param oam The currently selected sprite to draw.
  * @return Returns true if the current scanline passes through the sprite and return false otherwise.
  */
bool GPU::drawSprite(const unsigned char &y, const SpriteAttributes &oam){
	unsigned char xp = oam.xPos-8+rSCX->getValue(); // Top left
	unsigned char yp = oam.yPos-16+rSCY->getValue(); // Top left

	// Check that the current scanline goes through the sprite
	if(y < yp || y >= yp+(!objSizeSelect ? 8 : 16))
		return false;

	unsigned char pixelY = y - yp; // Vertical pixel in the tile
	unsigned char pixelColor;
	unsigned short bmpLow;
	
	// Retrieve the background tile ID from OAM
	// Tile map 0 is used (8000-8FFF)
	if(!objSizeSelect) // 8x8 pixel sprites
		bmpLow = 16*oam.tileNum;
	else if(pixelY <= 7) // Top half of 8x16 pixel sprites
		bmpLow = 16*(oam.tileNum & 0xFE);
	else{ // Bottom half of 8x16 pixel sprites
		bmpLow = 16*(oam.tileNum | 0x01); 
		pixelY -= 8;
	}

	if(oam.yFlip) // Vertical flip
		pixelY = 7 - pixelY;

	// Draw the specified line
	for(unsigned short dx = 0; dx < 8; dx++){
		if(!currentLineSprite[xp].getColor()){
			if(bGBCMODE){ // Gameboy Color sprite palettes (OBP0-7)
				pixelColor = getBitmapPixel(bmpLow, (!oam.xFlip ? (7-dx) : dx), pixelY, (oam.gbcVramBank ? 1 : 0));
				currentLineSprite[xp].setColorOBJ(pixelColor, oam.gbcPalette+8, oam.objPriority);
			}
			else{ // Original gameboy sprite palettes (OBP0-1)
				pixelColor = getBitmapPixel(bmpLow, (!oam.xFlip ? (7-dx) : dx), pixelY);
				currentLineSprite[xp].setColorOBJ(pixelColor, (oam.ngbcPalette ? 2 : 1), oam.objPriority);
			}
		}
		xp++;
	}
	
	return true;
}

void GPU::drawConsole(){
	console->update();
	console->draw();
	render();
}

void GPU::drawTileMaps(Window *win){
	int W = win->getWidth();
	int H = win->getHeight();
	win->setCurrent();
	// Tile maps are defined in VRAM [0x8000, 0x9800]
	const unsigned short tilesPerRow = W/8;
	unsigned short tileX, tileY;
	unsigned char pixelColor;
	for(unsigned short i = 0; i < 384; i++){
		tileY = i / tilesPerRow;
		for(unsigned char dy = 0; dy < 8; dy++){
			tileX = i % tilesPerRow;
			for(unsigned char dx = 0; dx < 8; dx++){
				pixelColor = getBitmapPixel(16*i, (7-dx), dy);
				switch(pixelColor){
					case 0:
						win->setDrawColor(Colors::WHITE);
						break;
					case 1:
						win->setDrawColor(Colors::LTGRAY);
						break;
					case 2:
						win->setDrawColor(Colors::DKGRAY);
						break;
					case 3:
						win->setDrawColor(Colors::BLACK);
						break;
					default:
						break;
				}
				win->drawPixel(tileX*8+dx, tileY*8+dy);
			}
		}
	}
}

void GPU::drawLayer(Window *win, bool mapSelect/*=true*/){
	win->setCurrent();
	unsigned char pixelX;
	ColorGBC line[256];
	for(unsigned short y = 0; y < 256; y++){
		pixelX = 0;
		for(unsigned short tx = 0; tx <= 32; tx++) // Draw the layer
			pixelX += drawTile(pixelX, (unsigned char)y, 0, (mapSelect ? 0x1C00 : 0x1800), line);
		for(unsigned short px = 0; px <= 256; px++){ // Draw the tile
			switch(line[px].getColor()){
				case 0:
					win->setDrawColor(Colors::WHITE);
					break;
				case 1:
					win->setDrawColor(Colors::LTGRAY);
					break;
				case 2:
					win->setDrawColor(Colors::DKGRAY);
					break;
				case 3:
					win->setDrawColor(Colors::BLACK);
					break;
				default:
					break;
			}
			win->drawPixel(px, y);
		}
	}
}

void GPU::drawNextScanline(SpriteHandler *oam){
	window->setCurrent();

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
	if((bGBCMODE || bgDisplayEnable) && userLayerEnable[0]){ // Background enabled
		for(unsigned short x = 0; x <= 20; x++) // Draw the background layer
			rx += drawTile(rx, ry, 0, (bgTileMapSelect ? 0x1C00 : 0x1800), currentLineBackground);
	}
	else{ // Background disabled (white)
		for(unsigned short x = 0; x < 160; x++) // Draw a "white" line
			currentLineBackground[rx++].reset();
	}

	// Handle the window layer
	bool windowVisible = false; // Is the window visible on this line?
	if(winDisplayEnable && (rLY->getValue() >= rWY->getValue())){
		if(userLayerEnable[1]){
			rx = rWX->getValue()-7;
			unsigned short nTiles = (159-(rWX->getValue()-7))/8; // Number of visible window tiles
			for(unsigned short x = 0; x <= nTiles; x++){
				rx += drawTile(rx, rWLY->getValue(), rWX->getValue() - 7, (winTileMapSelect ? 0x1C00 : 0x1800), currentLineWindow);
			}
			windowVisible = true;
		}
		(*rWLY)++; // Increment the scanline in the window region
	}

	// Handle the OBJ (sprite) layer
	if(objDisplayEnable && userLayerEnable[2]){
		int spritesDrawn = 0;
		if(oam->modified()){
			// Gather sprite attributes from OAM
			while(oam->updateNextSprite(sprites)){
			}
			// Sort sprites by priority.
			std::sort(sprites.begin(), sprites.end(), (bGBCMODE ? SpriteAttributes::compareCGB : SpriteAttributes::compareDMG));
		}
		if(!sprites.empty()){
			for(auto sp = sprites.begin(); sp != sprites.end(); sp++){
				if(drawSprite(ry, *sp) && ++spritesDrawn >= MAX_SPRITES_PER_LINE) // Max sprites per line
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
				currentPixel = &currentLineWindow[x];
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
	window->setCurrent();
	if(lcdDisplayEnable && window->status()){ // Check for events
		window->render();
	}
}

void GPU::processEvents(){
	window->setCurrent();
	window->processEvents();
}

bool GPU::getWindowStatus(){
	return window->status();
}

unsigned char GPU::getDmgPaletteColorHex(const unsigned short &index) const {
	return ngbcPaletteColor[index/4][index%4];
}

unsigned short GPU::getBgPaletteColorHex(const unsigned short &index) const { 
	return getUShort(bgPaletteData[index], bgPaletteData[index+1]); 
}

unsigned short GPU::getObjPaletteColorHex(const unsigned short &index) const { 
	return getUShort(objPaletteData[index], objPaletteData[index+1]); 
}

void GPU::setPixelScale(const unsigned int &n){
	window->setScalingFactor(n);
}

void GPU::print(const std::string &str, const unsigned char &x, const unsigned char &y){
	console->putString(str, x, y);
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
			if(winDisplayEnable) // Allow the window layer
				checkWindowVisible();
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
			checkWindowVisible();
			break;
		case 0xFF4B: // WX (Window X Position (minus 7))
			checkWindowVisible();
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
	return ColorRGB(r/31.0f, g/31.0f, b/31.0f);
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
	sys->addSystemRegister(this, 0x4C, rWLY,  "WLY",  "33333333"); // Fake window scanline register
	sys->addSystemRegister(this, 0x4F, rVBK,  "VBK",  "30000000");
	sys->addSystemRegister(this, 0x68, rBGPI, "BGPI", "33333303");
	sys->addSystemRegister(this, 0x69, rBGPD, "BGPD", "33333333");
	sys->addSystemRegister(this, 0x6A, rOBPI, "OBPI", "33333303");
	sys->addSystemRegister(this, 0x6B, rOBPD, "OBPD", "33333333");
}

bool GPU::checkWindowVisible(){	
	// The window is visible if WX=[0,167) and WY=[0,144)
	// WX=7, WY=0 locates the window at the upper left of the screen
	return (winDisplayEnable = rLCDC->getBit(5) && (rWX->getValue() < 167) && (rWY->getValue() < 144));
}
