#include <iostream> // TEMP
#include <algorithm>

#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Support.hpp"
#include "Graphics.hpp"
#include "ColorGBC.hpp"
#include "Console.hpp"
#include "GPU.hpp"
#include "SystemClock.hpp"
#include "ColorPalette.hpp"

constexpr unsigned short VRAM_LOW  = 0x8000;
constexpr unsigned short VRAM_HIGH = 0xA000;

constexpr int MAX_SPRITES_PER_LINE = 10;

constexpr int SCREEN_WIDTH_PIXELS  = 160;
constexpr int SCREEN_HEIGHT_PIXELS = 144;

GPU::GPU() : 
	SystemComponent("GPU", 0x20555050, 8192, 2, VRAM_LOW), // "PPU " (2 8kB banks of VRAM)
	bUserSelectedPalette(false),
	winDisplayEnable(false),
	nSpritesDrawn(0),
	bgPaletteIndex(0),
	objPaletteIndex(0),
	dmgPaletteColor{ {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3} }, // Default DMG palettes
	bgPaletteData(),
	objPaletteData(),
	cgbPaletteColor(),
	window(),
	console(),
	currentLineSprite(),
	currentLineWindow(),
	currentLineBackground(),
	userLayerEnable{ true, true, true },
	sprites()
{
}

GPU::~GPU(){
}

void GPU::initialize(){
	// Create a new window
	window = std::unique_ptr<Window>(new Window(SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS, 2));
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

	// Set default color palettes
	this->onUserReset();
}

unsigned char GPU::getBitmapPixel(const unsigned short &index, const unsigned char &dx, const unsigned char &dy, const unsigned char &bank/*=0*/){
	 // Retrieve a line of the bitmap at the requested pixel
	unsigned char pixelColor = (mem[bank][index+2*dy] & (0x1 << dx)) >> dx; // LS bit of the color
	pixelColor += ((mem[bank][index+2*dy+1] & (0x1 << dx)) >> dx) << 1; // MS bit of the color
	return pixelColor; // [0,3]
}

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
	if(rLCDC->bit4()) // 0x8000-0x8FFF
		bmpLow = 16*tileID;
	else // 0x8800-0x97FF
		bmpLow = (0x1000 + 16*twosComp(tileID));

	// Background & window tile attributes
	unsigned char bgPaletteNumber = 0;
	bool bgBankNumber = false;
	bool bgHorizontalFlip = false;
	bool bgVerticalFlip = false;
	bool bgPriority = false;
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
		if(bGBCMODE){ // CGB palettes
			pixelColor = getBitmapPixel(bmpLow, (!bgHorizontalFlip ? (7-dx) : dx), pixelY, (bgBankNumber ? 1 : 0));
			line[rx].setColorBG(pixelColor, bgPaletteNumber, bgPriority);
		}
		else{ // DMG palettes
			pixelColor = getBitmapPixel(bmpLow, (7-dx), pixelY);
			line[rx].setColorBG(pixelColor, 0);
		}
		rx++;
	}
	
	// Return the number of pixels drawn
	return (7-pixelX)+1;
}

bool GPU::drawSprite(const unsigned char &y, const SpriteAttributes &oam){
	unsigned char xp = oam.xPos - 8; // Top left
	unsigned char yp = oam.yPos - 16; // Top left

	// Check that the current scanline goes through the sprite
	if(y < yp || y >= yp + (!rLCDC->bit2() ? 8 : 16))
		return false;

	unsigned char pixelY = y - yp; // Vertical pixel in the tile
	unsigned char pixelColor;
	unsigned short bmpLow;
	
	// Retrieve the background tile ID from OAM
	// Tile map 0 is used (8000-8FFF)
	if(!rLCDC->bit2()){ // 8x8 pixel sprites
		if(oam.yFlip) // Vertical flip
			pixelY = 7 - pixelY;
		bmpLow = 16*oam.tileNum;
	}
	else{ // 8x16 pixel sprites	
		if(oam.yFlip) // Vertical flip
			pixelY = 15 - pixelY;
		if(pixelY <= 7) // Top half of 8x16 pixel sprites
			bmpLow = 16*(oam.tileNum & 0xFE);
		else{ // Bottom half of 8x16 pixel sprites
			bmpLow = 16*(oam.tileNum | 0x01); 
			pixelY -= 8;
		}
	}
	
	// Draw the specified line
	xp += rSCX->getValue();
	for(unsigned short dx = 0; dx < 8; dx++){
		if(!currentLineSprite[xp].getColor()){
			if(bGBCMODE){ // GBC sprite palettes (OBP0-7)
				pixelColor = getBitmapPixel(bmpLow, (!oam.xFlip ? (7-dx) : dx), pixelY, (oam.gbcVramBank ? 1 : 0));
				currentLineSprite[xp].setColorOBJ(pixelColor, oam.gbcPalette + 8, oam.objPriority);
			}
			else{ // DMG sprite palettes (OBP0-1)
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

unsigned short GPU::drawNextScanline(SpriteHandler *oam){
	window->setCurrent();

	// The pixel clock delay is dependent upon the number of sprites drawn on a given
	//  scanline, the background scroll register SCX, and the state of the window layer.
	unsigned short nPauseTicks = 0;

	// Here (ry) is the real vertical coordinate on the backgroun and (ly) is the current scanline.
	unsigned char ly = rLY->getValue();
	unsigned char ry = ly + rSCY->getValue();
	
	if(!rLCDC->bit7()){ // Screen disabled (draw a "white" line)
		if(bGBCMODE)
			window->setDrawColor(Colors::WHITE);
		else
			window->setDrawColor(cgbPaletteColor[0][0]);
		window->drawLine(0, ly, 159, ly);
		return 0;
	}

	unsigned char rx = rSCX->getValue(); // This will automatically handle screen wrapping
	for(unsigned short x = 0; x < 160; x++) // Reset the sprite line
		currentLineSprite[rx++].reset();

	// A non-zero scroll (SCX) will delay the clock SCX % 8 ticks.		
	nPauseTicks += rx % 8;

	// Handle the background layer
	rx = rSCX->getValue();
	if((bGBCMODE || rLCDC->bit0()) && userLayerEnable[0]){ // Background enabled
		for(unsigned short x = 0; x <= 20; x++) // Draw the background layer
			rx += drawTile(rx, ry, 0, (rLCDC->bit3() ? 0x1C00 : 0x1800), currentLineBackground);
	}
	else{ // Background disabled (white)
		for(unsigned short x = 0; x < 160; x++) // Draw a "white" line
			currentLineBackground[rx++].reset();
	}

	// Handle the window layer
	bool windowVisible = false; // Is the window visible on this line?
	if(winDisplayEnable && (ly >= rWY->getValue())){
		if(userLayerEnable[1]){
			rx = rWX->getValue()-7;
			unsigned short nTiles = (159-(rWX->getValue()-7))/8; // Number of visible window tiles
			for(unsigned short x = 0; x <= nTiles; x++){
				rx += drawTile(rx, rWLY->getValue(), rWX->getValue() - 7, (rLCDC->bit6() ? 0x1C00 : 0x1800), currentLineWindow);
			}
			windowVisible = true;
		}
		(*rWLY)++; // Increment the scanline in the window region
	}
	
	// The window layer being visible delays for 6 ticks.
	if(windowVisible)
		nPauseTicks += 6;

	// Handle the OBJ (sprite) layer
	rx = rSCX->getValue();
	if(rLCDC->bit1() && userLayerEnable[2]){
		nSpritesDrawn = 0;
		if(oam->modified()){
			// Gather sprite attributes from OAM
			while(oam->updateNextSprite(sprites)){
			}
			// Sort sprites by priority.
			std::sort(sprites.begin(), sprites.end(), (bGBCMODE ? SpriteAttributes::compareCGB : SpriteAttributes::compareDMG));
		}
		if(!sprites.empty()){
			// Search for sprites which overlap this scanline
			// Get the number of sprites on this scanline
			// Each sprite pauses the clock for N = 11 - min(5, (x + SCX) mod 8)
			for(auto sp = sprites.cbegin(); sp != sprites.cend(); sp++){
				if(drawSprite(ly, *sp)){ // Draw a row of the sprite
					nPauseTicks += 11 - std::min(5, (sp->xPos + rx) % 8);
					if(++nSpritesDrawn >= MAX_SPRITES_PER_LINE) // Max sprites per line
						break;
				}
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
			if(rLCDC->bit0()){ // BG/WIN priority
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
		if(bGBCMODE) // CGB
			currentPixelRGB = &cgbPaletteColor[currentPixel->getPalette()][currentPixel->getColor()];
		else // DMG
			currentPixelRGB = &cgbPaletteColor[currentPixel->getPaletteDMG()][dmgPaletteColor[currentPixel->getPalette()][currentPixel->getColor()]];
		window->setDrawColor(currentPixelRGB);
		window->drawPixel(x, ly);
		rx++;
	}
	
	// Return the number of pixel clock ticks to delay HBlank interval
	return nPauseTicks;
}

void GPU::render(){	
	// Update the screen
	window->setCurrent();
	if(rLCDC->bit7() && window->status()){ // Check for events
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
	return dmgPaletteColor[index/4][index%4];
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

void GPU::setColorPaletteDMG(){
	if(bGBCMODE) // Do not set DMG palettes when in CGB mode
		return;
	setColorPaletteDMG(Colors::GB_GREEN, Colors::GB_LTGREEN, Colors::GB_DKGREEN, Colors::GB_DKSTGREEN);
	bUserSelectedPalette = false; // Unset user palette flag, since we're using the default palette
}

void GPU::setColorPaletteDMG(const unsigned short& paletteID){
	if(bGBCMODE) // Do not set DMG palettes when in CGB mode
		return;
#ifdef TOP_DIRECTORY
	std::string path = std::string(TOP_DIRECTORY) + "/assets/palettes.dat";
#else
	std::string path = "./assets/palettes.dat";
#endif // ifdef TOP_DIRECTORY
	ColorPaletteDMG palette;
	if(palette.find(path, paletteID, verboseMode)){
		const int indices[3] = { 0, 8, 9 }; // The BG, OBJ0, and OBJ1 palettes
		for(int i = 0; i < 3; i++){
			cgbPaletteColor[indices[i]][0] = palette[i][0];
			cgbPaletteColor[indices[i]][1] = palette[i][1];
			cgbPaletteColor[indices[i]][2] = palette[i][2];
			cgbPaletteColor[indices[i]][3] = palette[i][3];
		}
		bUserSelectedPalette = true;
	}
}

void GPU::setColorPaletteDMG(const ColorRGB c0, const ColorRGB c1, const ColorRGB c2, const ColorRGB c3){
	if(bGBCMODE) // Do not set DMG palettes when in CGB mode
		return;
	const int indices[3] = { 0, 8, 9 }; // The BG, OBJ0, and OBJ1 palettes
	for(int i = 0; i < 3; i++){
		cgbPaletteColor[indices[i]][0] = c0;
		cgbPaletteColor[indices[i]][1] = c1;
		cgbPaletteColor[indices[i]][2] = c2;
		cgbPaletteColor[indices[i]][3] = c3;
		//for(int j = 0; j < 4; j++){
		//	cgbPaletteColor[i][j].toGrayscale();
		//}
	}
	bUserSelectedPalette = true;
}

void GPU::print(const std::string &str, const unsigned char &x, const unsigned char &y){
	console->putString(str, x, y);
}

bool GPU::writeRegister(const unsigned short &reg, const unsigned char &val){
	switch(reg){
		case 0xFF40: // LCDC (LCD Control Register)
			if(!rLCDC->bit7()) // LY is reset if LCD goes from on to off
				sys->getClock()->resetScanline();
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
			dmgPaletteColor[0][0] = rBGP->getBits(0,1);
			dmgPaletteColor[0][1] = rBGP->getBits(2,3);
			dmgPaletteColor[0][2] = rBGP->getBits(4,5);
			dmgPaletteColor[0][3] = rBGP->getBits(6,7); 
			break;
		case 0xFF48: // OBP0 (Object palette 0 data, non-gbc mode only)
			// See BGP above
			dmgPaletteColor[1][0] = 0x0; // Lower 2 bits not used, transparent for sprites
			dmgPaletteColor[1][1] = rOBP0->getBits(2,3); 
			dmgPaletteColor[1][2] = rOBP0->getBits(4,5); 
			dmgPaletteColor[1][3] = rOBP0->getBits(6,7); 
			break;
		case 0xFF49: // OBP1 (Object palette 1 data, non-gbc mode only)
			// See BGP above
			dmgPaletteColor[2][0] = 0x0; // Lower 2 bits not used, transparent for sprites
			dmgPaletteColor[2][1] = rOBP1->getBits(2,3);
			dmgPaletteColor[2][2] = rOBP1->getBits(4,5);
			dmgPaletteColor[2][3] = rOBP1->getBits(6,7);
			break;
		case 0xFF4A: // WY (Window Y Position)
			checkWindowVisible();
			break;
		case 0xFF4B: // WX (Window X Position (minus 7))
			checkWindowVisible();
			break;
		case 0xFF4F: // VBK (VRAM bank select, gbc mode)
			bs = rVBK->bit0() ? 1 : 0;
			break;
		case 0xFF68: // BGPI (Background palette index, gbc mode)
			bgPaletteIndex = rBGPI->getBits(0,5); // Index in the BG palette byte array
			break;
		case 0xFF69: // BGPD (Background palette data, gbc mode)
			if(bgPaletteIndex > 0x3F)
				bgPaletteIndex = 0;
			bgPaletteData[bgPaletteIndex] = rBGPD->getValue();
			updateBackgroundPalette(); // Updated palette data, refresh the real RGB colors
			if(rBGPI->bit7()) // Auto increment the BG palette byte index
				bgPaletteIndex++;
			break;
		case 0xFF6A: // OBPI (Sprite palette index, gbc mode)
			objPaletteIndex = rOBPI->getBits(0,5); // Index in the OBJ (sprite) palette byte array
			break;
		case 0xFF6B: // OBPD (Sprite palette index, gbc mode)
			if(objPaletteIndex > 0x3F)
				objPaletteIndex = 0;
			objPaletteData[objPaletteIndex] = rOBPD->getValue();
			updateObjectPalette(); // Updated palette data, refresh the real RGB colors
			if(rOBPI->bit7()) // Auto increment the OBJ (sprite) palette byte index
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

ColorRGB GPU::getColorRGB(const unsigned char &low, const unsigned char &high){
	unsigned char r = low & 0x1F;
	unsigned char g = ((low & 0xE0) >> 5) + ((high & 0x3) << 3);
	unsigned char b = (high & 0x7C) >> 2;
	return ColorRGB(r/31.0f, g/31.0f, b/31.0f);
}

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
	cgbPaletteColor[bgPaletteIndex/8][(bgPaletteIndex%8)/2] = getColorRGB(lowByte, highByte);
}

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
	cgbPaletteColor[objPaletteIndex/8+8][(objPaletteIndex%8)/2] = getColorRGB(lowByte, highByte);
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
	return (winDisplayEnable = rLCDC->bit5() && (rWX->getValue() < 167) && (rWY->getValue() < 144));
}

void GPU::userAddSavestateValues(){
	// Bools
	addSavestateValue(&winDisplayEnable, sizeof(bool));
	// Bytes
	unsigned int byte = sizeof(unsigned char);
	addSavestateValue(&bgPaletteIndex,  byte);
	addSavestateValue(&objPaletteIndex, byte);
	addSavestateValue(dmgPaletteColor,  byte * 12);
	addSavestateValue(bgPaletteData,    byte * 64);
	addSavestateValue(objPaletteData,   byte * 64);
	//ColorRGB cgbPaletteColor[16][4];
}

void GPU::onUserReset(){
	winDisplayEnable = false;
	nSpritesDrawn = 0;
	bgPaletteIndex = 0;
	objPaletteIndex = 0;

	// Reset DMG palettes
	for(unsigned char i = 0; i < 4; i++){
		dmgPaletteColor[0][i] = i;
		dmgPaletteColor[1][i] = i;
		dmgPaletteColor[2][i] = i;
	}

	// Clear palette data
	for(unsigned char i = 0; i < 64; i++){
		bgPaletteData[i] = 0;
		objPaletteData[i] = 0;
	}

	if(!bUserSelectedPalette){
		if(bGBCMODE){ // GBC palettes (all white at startup)
			for(int i = 0; i < 16; i++)
				for(int j = 0; j < 4; j++)
					cgbPaletteColor[i][j] = Colors::WHITE;
			bUserSelectedPalette = false; // Ignore user palettes for CGB games
		}
		else{ // Default DMG palettes
			setColorPaletteDMG();
		}
	}

	// Reset scanline buffers
	for(int i = 0; i < 256; i++){
		currentLineSprite[i].reset();
		currentLineWindow[i].reset();
		currentLineBackground[i].reset();
	}

	// Clear all sprite updates
	sprites.clear();
}

