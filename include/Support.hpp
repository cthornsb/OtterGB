#ifndef SUPPORT_HPP
#define SUPPORT_HPP

#include <string>
#include <vector>

// Return true if an input string is numerical
bool isNumeric(const std::string& str);

bool isInteger(const std::string& str);

bool isDecimal(const std::string& str);

bool isHexadecimal(const std::string& str);

bool isBinary(const std::string& str);

bool isNotNumeric(const std::string& str);

// Compute the two's compliment of an unsigned byte
short twosComp(const unsigned char &n);

unsigned int splitString(const std::string &input, std::vector<std::string> &output, const unsigned char &delim='\t');

std::string extractString(std::string& str, const char& c1, const char& c2, const std::string& repstr = "");

unsigned int countOccurances(const std::string& str, const char& c);

std::string getHex(const unsigned char &input);

std::string getHex(const unsigned short &input);

std::string getBinary(const unsigned char &input, const int &startBit=0);

std::string getBinary(const unsigned short &input, const int &startBit=0);

std::string ucharToStr(const unsigned char &input);

std::string ushortToStr(const unsigned short &input);

std::string uintToStr(const unsigned int &input);

std::string floatToStr(const float &input, const unsigned short &fixed=0);

std::string doubleToStr(const double &input, const unsigned short &fixed=0);

std::string toUppercase(const std::string &str);

std::string toLowercase(const std::string &str);

std::string stripWhitespace(const std::string &str);

std::string stripAllWhitespace(const std::string& str);

bool removeCharacter(std::string& str, const char& c);

unsigned char getUserInputUChar(const std::string& str);

unsigned short getUserInputUShort(const std::string& str);

unsigned int getUserInputUInt(const std::string& str);

unsigned short getUShort(const unsigned char &h, const unsigned char &l);

bool bitTest(const unsigned char &input, const unsigned char &bit);

void bitSet(unsigned char &input, const unsigned char &bit);

void bitReset(unsigned char &input, const unsigned char &bit);

unsigned char getBitmask(const unsigned char &lowBit, const unsigned char &highBit);

#endif
