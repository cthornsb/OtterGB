#include <algorithm>

#include "OTTWindow.hpp"

#ifdef WIN32
// "min" and "max" are macros defined in Windows.h
#undef min
#undef max
#endif // ifdef WIN32

#include "SystemRegisters.hpp"
#include "SystemGBC.hpp"
#include "Support.hpp"
#include "Console.hpp"
#include "GPU.hpp"
#include "SystemClock.hpp"
#include "ColorPalette.hpp"
#include "ConfigFile.hpp"

constexpr unsigned short VRAM_LOW  = 0x8000;
constexpr unsigned short VRAM_HIGH = 0xA000;

constexpr int MAX_SPRITES_PER_LINE = 10;

constexpr int SCREEN_WIDTH_PIXELS  = 160;
constexpr int SCREEN_HEIGHT_PIXELS = 144;

GPU::GPU() :
	SystemComponent("GPU", 0x20555050, 8192, 2, VRAM_LOW), // "PPU " (2 8kB banks of VRAM)
	bUserSelectedPalette(false),
	bWindowVisible(false),
	bWinDisplayEnable(false),
	bGrayscalePalette(false),
	bInvertColors(false),
	bGreenPaletteCGB(false),
	nScanline(0),
	nPosX(0),
	nPosY(0),
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
	dmgColorPalette{
		ColorRGB(155.0f / 255, 188.0f / 255, 15.0f / 255), // Green
		ColorRGB(139.0f / 255, 172.0f / 255, 15.0f / 255), // Light green
		ColorRGB(48.0f / 255, 98.0f / 255, 48.0f / 255),   // Dark green
		ColorRGB(15.0f / 255, 56.0f / 255, 15.0f / 255)    // Darkest green		
},
	fNextFrameOpacity(1.f),
	imageBuffer(0x0),
	sprites(),
	buffLock(),
	mode(PPUMODE::SCANLINE)
{
}

GPU::~GPU(){
}

void GPU::initialize(){
	// Create a new window
	window.reset(new OTTWindow(SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS, 2));

	// Setup the ascii character map for text output
	console = std::unique_ptr<ConsoleGBC>(new ConsoleGBC());
	console->setWindow(window.get());
	console->setSystem(sys);
	console->setTransparency(false);

	// Setup the window
	window->initialize("ottergb");
	window->enableKeyboard();
	window->enableGamepad();
	window->lockWindowAspectRatio(true);
	window->disableVSync();
	window->clear();

	// Get a pointer to the output image buffer
	imageBuffer = window->getBuffer();
	imageBuffer->setBlendMode(BlendMode::AVERAGE);

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
		bgPriority       = bitTest(tileAttr, 7); // (0=Obj on top, 1=BG and WIN colors 1-3 over Obj)
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

bool GPU::drawSprite(const unsigned char& y, const SpriteAttributes& oam) {
	unsigned char xp = oam.xPos - 8; // X coordinate of left side of sprite
	unsigned char yp, ybot; // Y coordinate of top and bottom of sprite

	// Check for sprites at the top of the screen
	if (!rLCDC->bit2()) { // 8x8 sprites
		if (oam.yPos >= 16) { // Sprite fully on screen
			yp = oam.yPos - 16;
			ybot = yp + 8;
		}
		else if (oam.yPos >= 8) { // At least one row of sprite on screen
			yp = 0;
			ybot = oam.yPos - 8;
		}
		else // Fully off top of screen
			return false;
	}
	else { // 8x16 sprites
		if (oam.yPos >= 16) { // Sprite fully on screen
			yp = oam.yPos - 16;
			ybot = yp + 16;
		}
		else {
			yp = 0;
			ybot = oam.yPos;
		}
	}

	// Check that the current scanline goes through the sprite
	if (y < yp || y >= ybot)
		return false;

	unsigned char pixelY = (oam.yPos >= 16 ? y - yp : y + (16 - oam.yPos)); // Vertical pixel in the tile
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
}

void GPU::drawTileMaps(OTTWindow *win){
	int W = win->getWidth();
	win->setCurrent();
	// Tile maps are defined in VRAM [0x8000, 0x9800]
	const unsigned short tilesPerRow = W/8;
	unsigned short tileX, tileY;
	for(unsigned short i = 0; i < 384; i++){
		tileY = i / tilesPerRow;
		for(unsigned char dy = 0; dy < 8; dy++){
			tileX = i % tilesPerRow;
			for(unsigned char dx = 0; dx < 8; dx++){
				switch(getBitmapPixel(16*i, (7-dx), dy)){
				case 0:
					win->buffWrite(tileX * 8 + dx, tileY * 8 + dy, Colors::WHITE);
					break;
				case 1:
					win->buffWrite(tileX * 8 + dx, tileY * 8 + dy, Colors::LTGRAY);
					break;
				case 2:
					win->buffWrite(tileX * 8 + dx, tileY * 8 + dy, Colors::DKGRAY);
					break;
				case 3:
					win->buffWrite(tileX * 8 + dx, tileY * 8 + dy, Colors::BLACK);
					break;
				default:
					break;
				}
			}
		}
	}
}

void GPU::drawLayer(OTTWindow *win, bool mapSelect/*=true*/){
	win->setCurrent();
	unsigned char pixelX;
	ColorGBC line[256];
	for(unsigned short y = 0; y < 256; y++){
		pixelX = 0;
		for(unsigned short tx = 0; tx <= 32; tx++) // Draw the layer
			pixelX += drawTile(pixelX, (unsigned char)y, 0, (mapSelect ? 0x1C00 : 0x1800), line);
		for(unsigned short px = 0; px < 256; px++){ // Draw the tile
			switch(line[px].getColor()){
			case 0:
				win->buffWrite(px, y, Colors::WHITE);
				break;
			case 1:
				win->buffWrite(px, y, Colors::LTGRAY);
				break;
			case 2:
				win->buffWrite(px, y, Colors::DKGRAY);
				break;
			case 3:
				win->buffWrite(px, y, Colors::BLACK);
				break;
			default:
				break;
			}
		}
	}
}

unsigned short GPU::drawNextScanline(SpriteHandler *oam){
	// The pixel clock delay is dependent upon the number of sprites drawn on a given
	//  scanline, the background scroll register SCX, and the state of the window layer.
	unsigned short nPauseTicks = 0;

	// Here (nScanline) is the current scanline.
	nScanline = rLY->getValue();
	
	if(!rLCDC->bit7()){ // Screen disabled (draw a "white" line)
		if(bGBCMODE)
			writeImageBuffer(nScanline, Colors::WHITE);
		else
			writeImageBuffer(nScanline, cgbPaletteColor[0][0]);
		return 0;
	}

	// The variables (nPosX) and (nPosY) are the real coordinates on the background layer and will automatically handle screen wrapping.
	nPosX = rSCX->getValue();
	nPosY = nScanline + rSCY->getValue();

	for(unsigned short x = 0; x < 160; x++) // Reset the sprite line
		currentLineSprite[nPosX++].reset();

	// A non-zero scroll (SCX) will delay the clock SCX % 8 ticks.		
	nPauseTicks += nPosX % 8;

	// Handle the background layer
	nPosX = rSCX->getValue();
	if((bGBCMODE || rLCDC->bit0()) && userLayerEnable[0]){ // Background enabled
		for(unsigned short x = 0; x <= 20; x++) // Draw the background layer
			nPosX += drawTile(nPosX, nPosY, 0, (rLCDC->bit3() ? 0x1C00 : 0x1800), currentLineBackground);
	}
	else{ // Background disabled (white)
		for(unsigned short x = 0; x < 160; x++) // Draw a "white" line
			currentLineBackground[nPosX++].reset();
	}

	// Handle the window layer
	bWindowVisible = false; // Is the window visible on this line?
	if(bWinDisplayEnable && (nScanline >= rWY->getValue())){
		if(userLayerEnable[1]){
			nPosX = rWX->getValue()-7;
			unsigned short nTiles = (159-(rWX->getValue()-7))/8; // Number of visible window tiles
			for(unsigned short x = 0; x <= nTiles; x++){
				nPosX += drawTile(nPosX, rWLY->getValue(), rWX->getValue() - 7, (rLCDC->bit6() ? 0x1C00 : 0x1800), currentLineWindow);
			}
			bWindowVisible = true;
		}
		(*rWLY)++; // Increment the scanline in the window region
	}
	
	// The window layer being visible delays for 6 ticks.
	if(bWindowVisible)
		nPauseTicks += 6;

	// Handle the OBJ (sprite) layer
	nPosX = rSCX->getValue();
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
				if(drawSprite(nScanline, *sp)){ // Draw a row of the sprite
					nPauseTicks += 11 - std::min(5, (sp->xPos + nPosX) % 8);
					if(++nSpritesDrawn >= MAX_SPRITES_PER_LINE) // Max sprites per line
						break;
				}
			}
		}
	}
	
	// Reset horizontal pixel counter in preparation to render pixel data
	nPosX = rSCX->getValue();
	
	// Render the current scanline
	renderScanline();
	
	// Return the number of pixel clock ticks to delay HBlank interval
	return nPauseTicks;
}

void GPU::renderScanline(){
	// Render the current scanline
	ColorGBC *currentPixel;
	unsigned char layerSelect = 0;
	bool bDrawingWindow = false;
	for(unsigned short x = 0; x < 160; x++){ // Draw the scanline		
		// Determine what layer should be drawn
		// CGB:
		//  LCDC bit 0 - 0=Sprites always on top of BG/WIN, 1=BG/WIN have priority
		//  Tile attr priority bit - 0=Use OAM priority bit, 1=BG Priority
		//  OAM sprite priority bit - 0=Sprite Above BG, 1=Sprite Behind BG color 1-3
		// DMG:
		//  LCDC bit 0 - 0=Off (white), 1=On
		//  OAM sprite priority bit - 0=Sprite Above BG, 1=Sprite Behind BG color 1-3
		// 
		if (!bDrawingWindow) { // Check if the window layer is visible
			// Since the window layer is only ever visible at LY >= WY and X >= WX,
			// the window will always be visible on a scanline after reaching pixel WX.
			bDrawingWindow = (bWindowVisible && x >= (rWX->getValue() - 7));
		}
		if(bGBCMODE){
			if(rLCDC->bit0()){ // BG/WIN priority
				if (bDrawingWindow) { // Drawing visible window region
					if (currentLineWindow[x].getColorPriority() || !currentLineSprite[nPosX].visible())
						layerSelect = 1;
					else
						layerSelect = 2;
				}
				else if(currentLineBackground[nPosX].getColorPriority()){ // BG priority (0=Sprite on top, 1=BG and WIN colors 1-3 over Sprite)
					layerSelect = 0;
				}
				else if (currentLineSprite[nPosX].visible()) { // Sprite pixel is visible
					if (currentLineSprite[nPosX].getPriority()) { // Sprite behind WIN and BG color 1-3
						if (bDrawingWindow && currentLineWindow[x].getColor() > 0) // Draw window
							layerSelect = 1;
						else if (currentLineBackground[nPosX].getColor() > 0) // Draw background
							layerSelect = 0;
						else // Draw sprite
							layerSelect = 2;
					}
					else // Sprite above BG/WIN (except for color 0 which is always transparent)
						layerSelect = 2;
				}
				else { // Draw background or window
					layerSelect = (bDrawingWindow ? 1 : 0);
				}
			}
			else if (currentLineSprite[nPosX].visible()) { // Sprites always on top (if visible)
				layerSelect = 2;
			}
			else { // Draw background or window
				layerSelect = (bDrawingWindow ? 1 : 0);
			}
		}
		else{
			if(currentLineSprite[nPosX].visible()){ // Use OAM priority bit
				if(currentLineSprite[nPosX].getPriority() && currentLineBackground[nPosX].getColor()){ // Sprite behind BG color 1-3
					if(bDrawingWindow) // Draw window
							layerSelect = 1;
						else // Draw background
							layerSelect = 0;
				}
				else // Sprite above BG (except for color 0 which is always transparent)
					layerSelect = 2;
			}
			else if(bDrawingWindow) // Draw window
				layerSelect = 1;
			else // Draw background
				layerSelect = 0;
		}
		switch(layerSelect){
			case 0: // Draw background
				currentPixel = &currentLineBackground[nPosX];
				break;
			case 1: // Draw window
				currentPixel = &currentLineWindow[x];
				break;
			case 2: // Draw sprite
				currentPixel = &currentLineSprite[nPosX];
				break;
			default:
				break;
		}
		if (bGBCMODE) { // CGB
			if (fNextFrameOpacity == 1.f) {
				imageBuffer->setPixel(x, nScanline, getPaletteColor(currentPixel->getPalette(), currentPixel->getColor()));
			}
			else{
				imageBuffer->setDrawColor(getPaletteColor(currentPixel->getPalette(), currentPixel->getColor()));
				imageBuffer->drawPixel(x, nScanline);
			}
		}
		else { // DMG
			if (fNextFrameOpacity == 1.f) {
				imageBuffer->setPixel(x, nScanline, getPaletteColor(currentPixel->getPaletteDMG(), dmgPaletteColor[currentPixel->getPalette()][currentPixel->getColor()]));
			}
			else {
				imageBuffer->setDrawColor(getPaletteColor(currentPixel->getPaletteDMG(), dmgPaletteColor[currentPixel->getPalette()][currentPixel->getColor()]));
				imageBuffer->drawPixel(x, nScanline);
			}
		}
		nPosX++;
	}
}

void GPU::render(){	
	// Update the screen
	if(window->status()){ // Check window status
		window->clear(); // Clear screen to avoid visual artifacts in un-drawn areas
		window->setCurrent();
		window->renderBuffer();
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
	window->updateWindowSize(n);
}

void GPU::setFrameBlur(const float& blur) {
	fNextFrameOpacity = (1.f - std::min(std::max(blur, 0.f), 1.f)); // Clamp to the range 0 to 1
	for (int i = 0; i < 4; i++)
		dmgColorPalette[i].a = fNextFrameOpacity * 255;
	for (int i = 0; i < 16; i++) { // Update the alpha (opacity) for all existing palette colors
		for (int j = 0; j < 4; j++)
			cgbPaletteColor[i][j].a = fNextFrameOpacity * 255;
	}
}

void GPU::setColorPaletteDMG(){
	if(bGBCMODE) // Do not set DMG palettes when in CGB mode
		return;
	// Monochrome green DMG color palette
	if (bGrayscalePalette) { // Convert to sRGB grayscale
		for (int i = 0; i < 4; i++)
			dmgColorPalette[i].toGrayscale();
	}
	if (bInvertColors) { // Invert colors
		for (int i = 0; i < 4; i++)
			dmgColorPalette[i] = dmgColorPalette[i].invert();
	}
	setColorPaletteDMG(
		dmgColorPalette[0],
		dmgColorPalette[1],
		dmgColorPalette[2],
		dmgColorPalette[3]
	);
	bUserSelectedPalette = false; // Unset user palette flag, since we're using the default palette
}

void GPU::setColorPaletteDMG(const unsigned short& paletteID){
	if(bGBCMODE) // Do not set DMG palettes when in CGB mode
		return;
	ColorPaletteDMG palette;
	if(palette.find("assets/palettes.dat", paletteID, verboseMode)){
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
	}
	bUserSelectedPalette = true;
}

void GPU::setOperationMode(const PPUMODE& newMode){
	mode = newMode;
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
	ColorRGB retval(r / 31.0f, g / 31.0f, b / 31.0f, fNextFrameOpacity);
	if (bGreenPaletteCGB) { // Convert CGB colors to DMG; dumb, but fun :)
		auto colorDist = [](const ColorRGB& input, const ColorRGB& output) {
			float dR = (float)input.r - (float)output.r;
			float dG = (float)input.g - (float)output.g;
			float dB = (float)input.b - (float)output.b;
			return std::sqrt(dR * dR + dG * dG + dB * dB);
		};
		float fMinimumDistance = 100000.f;
		ColorRGB* minColor = 0x0;
		for (int i = 0; i < 4; i++) {
			float dist = colorDist(retval, dmgColorPalette[i]);
			if (dist < fMinimumDistance) {
				fMinimumDistance = dist;
				minColor = &dmgColorPalette[i];
			}
		}
		if (minColor)
			retval = *minColor;
	}
	if (bGrayscalePalette) // Convert to sRGB grayscale
		retval.toGrayscale();
	if (bInvertColors)
		return retval.invert();
	return retval;
}

ColorRGB& GPU::getPaletteColor(const unsigned char& palette, const unsigned char& color){
	return cgbPaletteColor[palette][color];
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

void GPU::readConfigFile(ConfigFile* config) {
	if (config->search("COLOR_PALETTE", true)) // Set DMG color palette
		setColorPaletteDMG(getUserInputUShort(config->getCurrentParameterString()));
	if (config->search("PIXEL_SCALE", true)) // Set graphical window pixel scaling factor
		setPixelScale(config->getUInt());
	if (config->searchBoolFlag("START_FULLSCREEN")) // Start in fullscreen mode
		sys->toggleFullScreenMode();
	if (config->searchBoolFlag("UNLOCK_ASPECT_RATIO")) // Lock window aspect ratio
		window->lockWindowAspectRatio(false);
	if (config->searchBoolFlag("GRAYSCALE_PALETTE")) // Set DMG color palette to sRGB grayscale
		bGrayscalePalette = true;
	if (config->searchBoolFlag("INVERTED_COLORS")) // Invert output color palettes
		bInvertColors = true;
	if (config->searchBoolFlag("GREEN_PALETTE_CGB")) // Set CGB game colors to monochrome green DMG palette
		bGreenPaletteCGB = true;
	if (config->search("FRAME_BLUR", true))
		setFrameBlur(config->getFloat());
}

bool GPU::checkWindowVisible(){	
	// The window is visible if WX=[0,167) and WY=[0,144)
	// WX=7, WY=0 locates the window at the upper left of the screen
	return (bWinDisplayEnable = rLCDC->bit5() && (rWX->getValue() < 167) && (rWY->getValue() < 144));
}

void GPU::writeImageBuffer(const unsigned short& x, const unsigned short& y, const ColorRGB& color){
	buffLock.lock();
	window->buffWrite(x, y, color);
	buffLock.unlock();
}

void GPU::writeImageBuffer(const unsigned short& y, const ColorRGB& color){
	buffLock.lock();
	window->buffWriteLine(y, color);
	buffLock.unlock();
}

void GPU::userAddSavestateValues(){
	// Bools
	addSavestateValue(&bWinDisplayEnable, sizeof(bool));
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
	bWinDisplayEnable = false;
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

void GPU::mainLoop(){
	buffLock.lock();
	if(mode == PPUMODE::SCANLINE){
		renderScanline(); // Push pixel data into the image buffer
	}
	else if(mode == PPUMODE::DRAWBUFFER){
		window->setCurrent();
		window->renderBuffer();
		mode = PPUMODE::SCANLINE; // Automatically switch back to rendering scanlines
	}
	parent->notify(1); // Notify the main thread operation completed successfully
	buffLock.unlock();
}

