#include <iostream>

#include "colors.hpp"

ColorRGB::ColorRGB(const float &value) :
	r(toUChar(value)),
	g(toUChar(value)),
	b(toUChar(value))
{
}

ColorRGB::ColorRGB(const float &red, const float &green, const float &blue) :
	r(toUChar(red)),
	g(toUChar(green)),
	b(toUChar(blue))
{
}

ColorRGB ColorRGB::invert() const {
	return ColorRGB(255 - r, 255 - g, 255 - b);
}

void ColorRGB::toGrayscale(){
	// Based on the sRGB convention
	float value = toFloat(r) * 0.2126f + toFloat(g) * 0.7152f + toFloat(b) * 0.0722f;
	r = toUChar(value);
	g = toUChar(value);
	b = toUChar(value);
}

ColorRGB ColorRGB::operator + (const ColorRGB &rhs) const {
	float rprime = (r + rhs.r)/255.0f;
	float gprime = (g + rhs.g)/255.0f;
	float bprime = (b + rhs.b)/255.0f;
	return ColorRGB((rprime <= 1 ? rprime : 1), (gprime <= 1 ? gprime : 1), (bprime <= 1 ? bprime : 1));
}

ColorRGB ColorRGB::operator - (const ColorRGB &rhs) const {
	float rprime = (r - rhs.r)/255.0f;
	float gprime = (g - rhs.g)/255.0f;
	float bprime = (b - rhs.b)/255.0f;
	return ColorRGB((rprime > 0 ? rprime : 0), (gprime > 1 ? gprime : 0), (bprime > 1 ? bprime : 0));
}

ColorRGB ColorRGB::operator * (const float &rhs) const {
	return ColorRGB(r*rhs, g*rhs, b*rhs);
}

ColorRGB ColorRGB::operator / (const float &rhs) const {
	return ColorRGB(r/rhs, g/rhs, b/rhs);
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
	std::cout << "r=" << toFloat(r) << ", g=" << toFloat(g) << ", b=" << toFloat(b) << std::endl;
}
