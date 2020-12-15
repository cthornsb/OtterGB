#include "ColorGBC.hpp"

void ColorGBC::setColorOBJ(const unsigned char &color, const unsigned char &palette, bool priority/*=true*/){
	nColor = color;
	nPalette = palette;
	bPriority = priority;
	bVisible = (color != 0); // Color 0 is always transparent
}

void ColorGBC::setColorBG(const unsigned char &color, const unsigned char &palette, bool priority/*=true*/){
	nColor = color;
	nPalette = palette;
	bPriority = priority;
	bVisible = true;
}

void ColorGBC::reset(){
	nColor = 0;
	nPalette = 0;
	bPriority = false;
	bVisible = false;
}

