#ifndef BITMAP_HPP
#define BITMAP_HPP

#include "colors.hpp"

class Window;

class Bitmap{
public:
	Bitmap();
	
	bool isBlank() const { return blank; }
	
	unsigned char get(const unsigned short &x, const unsigned short &y) const ;
	
	void set(const unsigned short &x, const unsigned short &y, const unsigned char &color);
	
	void dump();
	
private:
	bool blank;

	unsigned char pixels[8][8];
};

class CharacterMap{
public:
	CharacterMap();

	void setWindow(Window *win){ window = win; }

	bool loadCharacterMap(const std::string &fname);

	void putCharacter(const char &val, const unsigned short &x, const unsigned short &y);

	void putString(const std::string &str, const unsigned short &x, const unsigned short &y, bool wrap=true);

private:
	Window *window;
	
	ColorRGB palette[4];

	Bitmap cmap[128];
};

#endif
