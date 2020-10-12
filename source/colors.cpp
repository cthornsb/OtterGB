#include <iostream>

#include "colors.hpp"

sdlColor::sdlColor(const float &value){
	r = toUChar(value);
	g = r;
	b = r;
}

sdlColor::sdlColor(const float &red, const float &green, const float &blue){
	r = toUChar(red);
	g = toUChar(green);
	b = toUChar(blue);
}

sdlColor sdlColor::operator + (const sdlColor &rhs) const {
	float rprime = (r + rhs.r)/255.0;
	float gprime = (g + rhs.g)/255.0;
	float bprime = (b + rhs.b)/255.0;
	return sdlColor((rprime <= 1 ? rprime : 1), (gprime <= 1 ? gprime : 1), (bprime <= 1 ? bprime : 1));
}

sdlColor sdlColor::operator - (const sdlColor &rhs) const {
	float rprime = (r - rhs.r)/255.0;
	float gprime = (g - rhs.g)/255.0;
	float bprime = (b - rhs.b)/255.0;
	return sdlColor((rprime > 0 ? rprime : 0), (gprime > 1 ? gprime : 0), (bprime > 1 ? bprime : 0));
}

sdlColor sdlColor::operator * (const float &rhs) const {
	return sdlColor(toFloat(r)*rhs, toFloat(g)*rhs, toFloat(b)*rhs);
}

sdlColor sdlColor::operator / (const float &rhs) const {
	return sdlColor(toFloat(r)/rhs, toFloat(g)/rhs, toFloat(b)/rhs);
}

sdlColor& sdlColor::operator += (const sdlColor &rhs){
	return ((*this) = (*this) + rhs);
}

sdlColor& sdlColor::operator -= (const sdlColor &rhs){
	return ((*this) = (*this) - rhs);
}

sdlColor& sdlColor::operator *= (const float &rhs){
	return ((*this) = (*this) * rhs);
}

sdlColor& sdlColor::operator /= (const float &rhs){
	return ((*this) = (*this) / rhs);
}

sdlColor sdlColor::invert() const {
	return sdlColor(255-r, 255-g, 255-b);
}

void sdlColor::toGrayscale(){
	// Based on the sRGB convention
	r = ((unsigned char)(0.2126*r));
	g = ((unsigned char)(0.7152*g));
	b = ((unsigned char)(0.0722*b));
}

void sdlColor::dump() const {
	std::cout << "r=" << (int)r << ", g=" << (int)g << ", b=" << (int)b << std::endl;
}
