#ifndef SPRITE_ATTRIBUTES_HPP
#define SPRITE_ATTRIBUTES_HPP

#include <queue>

#include "SystemComponent.hpp"

class SpriteAttributes{
public:
	unsigned char yPos; ///< Y-position of the current sprite
	
	unsigned char xPos; ///< X-position of the current sprite
	
	unsigned char tileNum; ///< Tile index for the current sprite
	
	bool objPriority; ///< Object to Background priority (0: OBJ above BG, 1: OBJ behind BG color 1-3, BG color 0 always behind))
	
	bool yFlip; ///< Vertical flip (0: normal, 1: mirrored vertically)
	
	bool xFlip; ///< Horizontal flip (0: normal, 1: mirrored horizontally)
	
	bool ngbcPalette; ///< DMG palette number (0: OBP0, 1: OBP1)
	
	bool gbcVramBank; ///< CGB VRAM bank number (0: Bank 0, 1: Bank 1)

	unsigned char gbcPalette; ///< CGB palette number (OBP0-7)
	
	unsigned char oamIndex; ///< Sprite index in the OAM table (0-39)
	
	/** Default constructor.
	  */
	SpriteAttributes() :
		yPos(0),
		xPos(0),
		tileNum(0),
		objPriority(false),
		yFlip(false),
		xFlip(false),
		ngbcPalette(false),
		gbcVramBank(false),
		gbcPalette(0),
		oamIndex(0)
	{ 
	}
	
	/** Equality operator.\
	  * @param other Another sprite to check for equality.
	  * @return True if the OAM sprite index is equal for both sprites and return false otherwise.
	  */
	bool operator == (const SpriteAttributes &other) const { 
		return (oamIndex == other.oamIndex); 
	}

	/** Equality operator.
	  * @param index An index in OAM memory to check for equality.
	  * @return True if the OAM sprite index is equal and return false otherwise.
	  */
	bool operator == (const unsigned char &index) const { 
		return (oamIndex == index); 
	}
	
	/** Get sprite layering priority for DMG sprites
	  * When sprites with differing X position overlap, the one whose positino is lower will have priority. 
	  * If their X position is the same, priority is assigned based on table ordering.
	  */
	static bool compareDMG(const SpriteAttributes &s1, const SpriteAttributes &s2);
	
	/** Get sprite layering priority for CGB sprites
	  * Sprite priority is assigned based solely on table ordering.
	  */
	static bool compareCGB(const SpriteAttributes &s1, const SpriteAttributes &s2);
};

class SpriteHandler : public SystemComponent {
public:
	/** Default constructor
	  */
	SpriteHandler();

	/** If the requested address is within OAM, write the new value to memory and mark that sprite as modified
	  * @return True if requeseted memory address was within OAM memory space
	  */
	bool preWriteAction() override ;
	
	/** Get the sprite attributes for a sprite in the OAM table
	  */
	SpriteAttributes getSpriteAttributes(const unsigned char &index);
	
	/** Decode the attributes of the next modified sprite and add it to a vector of sprite attributes
	  * @return True if there was at least one modified sprite in the list
	  */
	bool updateNextSprite(std::vector<SpriteAttributes> &sprites);
	
	/** Return true if the attributes of at least one sprite have been modified
	  */
	bool modified(){ 
		return !lModified.empty(); 
	}
	
	/** Ignore any pending sprite attribute updates
	  */
	void reset();
	
private:
	bool bModified[40]; ///< Set if the attributes of a sprite have been modified

	std::queue<unsigned char> lModified; ///< List of sprites whose attributes have been modified
	
	/** Decode sprite attributes from raw input data
	  * @param ptr Pointer to raw sprite data (expected to contain at least 4 bytes of data)
	  * @param attr Sprite attributes pointer where decoded sprite attributes will be stored
	  */
	void getSpriteData(unsigned char *ptr, SpriteAttributes *attr);
};

#endif

