#include <iostream> // TEMP CRT
#include <algorithm>

#include "SpriteAttributes.hpp"
#include "SystemRegisters.hpp"
#include "Support.hpp"

#ifdef WIN32
// "min" and "max" are macros defined in Windows.h
#undef min
#undef max
#endif // ifdef WIN32

constexpr unsigned short OAM_TABLE_LOW  = 0xFE00;
constexpr unsigned short OAM_TABLE_HIGH = 0xFEA0;

constexpr unsigned short MAX_SPRITES_PER_LINE = 10;

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

SpriteHandler::SpriteHandler() : 
	SystemComponent("OAM", 0x204d414f, 160, 1), // "OAM " (1 160 B bank of RAM)
	bModified{ 
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0 
	},
	nSpritesDrawn(0),
	nMaxSpritesPerLine(MAX_SPRITES_PER_LINE),
	lModified(),
	spritesVisible(),
	spritesToDraw()
{ 
	reset();
}

bool SpriteHandler::updateNextSprite(std::vector<SpriteAttributes> &sprites){
	if(lModified.empty())
		return false;

	unsigned short spriteIndex = lModified.front();
	bModified[spriteIndex] = false;
	lModified.pop();

	// Search for the sprite in the list of active sprites
	std::vector<SpriteAttributes>::iterator iter = std::find(sprites.begin(), sprites.end(), spriteIndex);
	
	unsigned char *data = &mem[0][spriteIndex * 4];
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

	unsigned short spriteIndex = (writeLoc - OAM_TABLE_LOW)/4;
	if(!bModified[spriteIndex]){
		lModified.push(spriteIndex);
		bModified[spriteIndex] = true;
	}

	return true;
}

SpriteAttributes SpriteHandler::getSpriteAttributes(const unsigned short &index){
	unsigned char *data = &mem[0][index * 4];
	SpriteAttributes attr;
	getSpriteData(data, &attr);
	attr.oamIndex = index;
	return attr;
}

void SpriteHandler::clear(){
	for(unsigned short i = 0; i < 40; i++)
		bModified[i] = false;
	while(!lModified.empty())
		lModified.pop();
	spritesVisible.clear(); // Clear all sprite updates
	spritesToDraw.clear();
	nSpritesDrawn = 0;
}

int SpriteHandler::search() {
	// Decode sprites which have been updated
	if (!lModified.empty()) {
		// Gather sprite attributes from OAM
		while (updateNextSprite(spritesVisible)) {
		}
		// Sort sprites by priority.
		std::sort(spritesVisible.begin(), spritesVisible.end(), (bGBCMODE ? SpriteAttributes::compareCGB : SpriteAttributes::compareDMG));
	}

	spritesToDraw.clear();
	if (!rLCDC->bit1())
		return 0;

	unsigned short retval = 0;

	nSpritesDrawn = 0;
	if (!spritesVisible.empty()) {
		// Search for sprites which overlap this scanline
		// Get the number of sprites on this scanline
		// Each sprite pauses the clock for N = 11 - min(5, (x + SCX) mod 8)
		for (std::vector<SpriteAttributes>::iterator sp = spritesVisible.begin(); sp != spritesVisible.end(); sp++) {
			unsigned char y = rLY->getValue();
			unsigned char yp, ybot; // Y coordinate of top and bottom of sprite

			// Check for sprites at the top of the screen
			if (!rLCDC->bit2()) { // 8x8 sprites
				if (sp->yPos >= 16) { // Sprite fully on screen
					yp = sp->yPos - 16;
					ybot = yp + 8;
				}
				else if (sp->yPos >= 8) { // At least one row of sprite on screen
					yp = 0;
					ybot = sp->yPos - 8;
				}
				else // Fully off top of screen
					continue;
			}
			else { // 8x16 sprites
				if (sp->yPos >= 16) { // Sprite fully on screen
					yp = sp->yPos - 16;
					ybot = yp + 16;
				}
				else {
					yp = 0;
					ybot = sp->yPos;
				}
			}

			// Check that the current scanline goes through the sprite
			if (y < yp || y >= ybot)
				continue;

			// Compute the vertical pixel in the tile
			sp->pixelY = (sp->yPos >= 16 ? y - yp : y + (16 - sp->yPos));

			// Retrieve the background tile ID from OAM
			// Tile map 0 is used (8000-8FFF)
			if (!rLCDC->bit2()) { // 8x8 pixel sprites
				if (sp->yFlip) // Vertical flip
					sp->pixelY = 7 - sp->pixelY;
				sp->bmpLow = 16 * sp->tileNum;
			}
			else { // 8x16 pixel sprites	
				if (sp->yFlip) // Vertical flip
					sp->pixelY = 15 - sp->pixelY;
				if (sp->pixelY <= 7) // Top half of 8x16 pixel sprites
					sp->bmpLow = 16 * (sp->tileNum & 0xFE);
				else { // Bottom half of 8x16 pixel sprites
					sp->bmpLow = 16 * (sp->tileNum | 0x01);
					sp->pixelY -= 8;
				}
			}

			// Sprite overlaps the current scanline
			retval += 11 - std::min(5, (sp->xPos + rSCX->getValue()) % 8);
			spritesToDraw.push_back(&(*sp));
			if (++nSpritesDrawn >= nMaxSpritesPerLine) // Max sprites per line
				break;
		}
	}

	return retval;
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

void SpriteHandler::onUserReset(){
	clear();
}

