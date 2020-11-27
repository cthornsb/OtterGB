#ifndef SUPPORT_HPP
#define SUPPORT_HPP

#include <string>
#include <vector>

// Compute the two's compliment of an unsigned byte
short twosComp(const unsigned char &n);

unsigned int splitString(const std::string &input, std::vector<std::string> &output, const unsigned char &delim='\t');

std::string getHex(const unsigned char &input);

std::string getHex(const unsigned short &input);

std::string getBinary(const unsigned char &input, const int &startBit=0);

std::string getBinary(const unsigned short &input, const int &startBit=0);

std::string ucharToStr(const unsigned char &input);

std::string ushortToStr(const unsigned short &input);

std::string floatToStr(const float &input, const unsigned short &fixed=0);

std::string doubleToStr(const double &input, const unsigned short &fixed=0);

unsigned short getUShort(const unsigned char &h, const unsigned char &l);

bool bitTest(const unsigned char &input, const unsigned char &bit);

void bitSet(unsigned char &input, const unsigned char &bit);

void bitReset(unsigned char &input, const unsigned char &bit);

unsigned char getBitmask(const unsigned char &lowBit, const unsigned char &highBit);

#endif
