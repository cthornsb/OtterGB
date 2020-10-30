#include <iostream>

#include "colors.hpp"

#ifndef USE_OPENGL

	ColorRGB::ColorRGB(const float &value){
		r = toUChar(value);
		g = r;
		b = r;
	}

	ColorRGB::ColorRGB(const float &red, const float &green, const float &blue){
		r = toUChar(red);
		g = toUChar(green);
		b = toUChar(blue);
	}

	ColorRGB ColorRGB::invert() const {
		return ColorRGB(255-r, 255-g, 255-b);
	}

	void ColorRGB::toGrayscale(){
		// Based on the sRGB convention
		r = ((unsigned char)(0.2126*r));
		g = ((unsigned char)(0.7152*g));
		b = ((unsigned char)(0.0722*b));
	}

#else

	ColorRGB::ColorRGB(const float &value){
		r = value;
		g = r;
		b = r;
	}

	ColorRGB::ColorRGB(const float &red, const float &green, const float &blue){
		r = red;
		g = green;
		b = blue;
	}

	ColorRGB ColorRGB::invert() const {
		return ColorRGB(1-r, 1-g, 1-b);
	}

	void ColorRGB::toGrayscale(){
		// Based on the sRGB convention
		r *= 0.2126;
		g *= 0.7152;
		b *= 0.0722;
	}

#endif

ColorRGB ColorRGB::operator + (const ColorRGB &rhs) const {
	float rprime = (r + rhs.r)/255.0;
	float gprime = (g + rhs.g)/255.0;
	float bprime = (b + rhs.b)/255.0;
	return ColorRGB((rprime <= 1 ? rprime : 1), (gprime <= 1 ? gprime : 1), (bprime <= 1 ? bprime : 1));
}

ColorRGB ColorRGB::operator - (const ColorRGB &rhs) const {
	float rprime = (r - rhs.r)/255.0;
	float gprime = (g - rhs.g)/255.0;
	float bprime = (b - rhs.b)/255.0;
	return ColorRGB((rprime > 0 ? rprime : 0), (gprime > 1 ? gprime : 0), (bprime > 1 ? bprime : 0));
}

ColorRGB ColorRGB::operator * (const float &rhs) const {
	return ColorRGB(toFloat(r)*rhs, toFloat(g)*rhs, toFloat(b)*rhs);
}

ColorRGB ColorRGB::operator / (const float &rhs) const {
	return ColorRGB(toFloat(r)/rhs, toFloat(g)/rhs, toFloat(b)/rhs);
}

ColorRGB& ColorRGB::operator += (const ColorRGB &rhs){
	return ((*this) = (*this) + rhs);
}

ColorRGB& ColorRGB::operator -= (const ColorRGB &rhs){
	return ((*this) = (*this) - rhs);
}

ColorRGB& ColorRGB::operator *= (const float &rhs){
	return ((*this) = (*this) * rhs);
}

ColorRGB& ColorRGB::operator /= (const float &rhs){
	return ((*this) = (*this) / rhs);
}

void ColorRGB::dump() const {
	std::cout << "r=" << (int)r << ", g=" << (int)g << ", b=" << (int)b << std::endl;
}

void ColorGBC::setColorOBJ(const unsigned char &color, const unsigned char &palette, bool priority/*=true*/){
	nColor = color;
	nPalette = palette;
	bPriority = priority;
	bVisible = true;
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
