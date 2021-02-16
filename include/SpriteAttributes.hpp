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
	bool yFlip; ///< (0: normal, 1: mirrored vertically)
	bool xFlip; ///< (0: normal, 1: mirrored horizontally)
	bool ngbcPalette; ///< (0: OBP0, 1: OBP1)
	bool gbcVramBank; ///< (0: Bank 0, 1: Bank 1)

	unsigned char gbcPalette; ///< (OBP0-7)
	unsigned char oamIndex; ///< Sprite index in the OAM table (0-39)
	
	/** Default constructor.
	  */
	SpriteAttributes(){ }
	
	/** Equality operator.
	  * @param other Another sprite to check for equality.
	  * @return True if the OAM sprite index is equal for both sprites and return false otherwise.
	  */
	bool operator == (const SpriteAttributes &other) const { return (oamIndex == other.oamIndex); }

	/** Equality operator.
	  * @param index An index in OAM memory to check for equality.
	  * @return True if the OAM sprite index is equal and return false otherwise.
	  */
	bool operator == (const unsigned char &index) const { return (oamIndex == index); }
	
	static bool compareDMG(const SpriteAttributes &s1, const SpriteAttributes &s2);
	
	static bool compareCGB(const SpriteAttributes &s1, const SpriteAttributes &s2);
};

class SpriteHandler : public SystemComponent {
public:
	SpriteHandler();

	bool preWriteAction() override ;
	
	SpriteAttributes getSpriteAttributes(const unsigned char &index);
	
	bool updateNextSprite(std::vector<SpriteAttributes> &sprites);
	
	bool modified(){ return !lModified.empty(); }
	
	void reset();
	
private:
	bool bModified[40];

	std::queue<unsigned char> lModified; ///< List of sprites which have been written to.
	
	void getSpriteData(unsigned char *ptr, SpriteAttributes *attr);
};

#endif

