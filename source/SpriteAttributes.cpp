
#include <algorithm>

#include "SpriteAttributes.hpp"
#include "SystemRegisters.hpp"
#include "Support.hpp"

constexpr unsigned short OAM_TABLE_LOW  = 0xFE00;
constexpr unsigned short OAM_TABLE_HIGH = 0xFEA0;

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
	lModified()
{ 
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

void SpriteHandler::clear(){
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

void SpriteHandler::onUserReset(){
	clear();
}

