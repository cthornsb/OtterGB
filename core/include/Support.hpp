#ifndef SUPPORT_HPP
#define SUPPORT_HPP

#include <string>
#include <vector>

/** Return true if an input string is numerical
  */
bool isNumeric(const std::string& str);

/** Return true if an input string is numerical and is an integer
  */
bool isInteger(const std::string& str);

/** Return true if an input string is numerical and is a decimal value
  */
bool isDecimal(const std::string& str);

/** Return true if an input string is numerical and is a hexadecimal value
  */
bool isHexadecimal(const std::string& str);

/** Return true if an input string is numerical and is a binary value
  */
bool isBinary(const std::string& str);

/** Return true if an input string is NOT numerical
  */
bool isNotNumeric(const std::string& str);

/** Compute the two's compliment of an unsigned byte
  */
short twosComp(const unsigned char &n);

/** Split an input string into parts about a specified delimiter character
  * @param input Input string to split into parts
  * @param output Vector of strings to store split strings
  * @param delim Delimiter character about which the input string will be split
  * @return The number of output strings the input was split into
  */
unsigned int splitString(const std::string &input, std::vector<std::string> &output, const unsigned char &delim='\t');

/** Extract a sub-string from an input string
  */
std::string extractString(std::string& str, const char& c1, const char& c2, const std::string& repstr = "");

/** Counter the number of occurances of character 'c' in the input string and return the result
  */
unsigned int countOccurances(const std::string& str, const char& c);

/** Convert input integer to a hexadecimal string
  */
std::string getHex(const unsigned char &input);

/** Convert input integer to a hexadecimal string
  */
std::string getHex(const unsigned short &input);

/** Convert input integer to a binary string
  */
std::string getBinary(const unsigned char &input, const int &startBit=0);

/** Convert input integer to a binary string
  */
std::string getBinary(const unsigned short &input, const int &startBit=0);

/** Convert input integer to ascii string
  */
std::string getAscii(const unsigned short& input);

/** Convert input integer to ascii string
  */
std::string getAscii(const unsigned int& input);

/** Convert input integer to a decimal string
  */
std::string ucharToStr(const unsigned char &input);

/** Convert input integer to a decimal string
  */
std::string ushortToStr(const unsigned short &input);

/** Convert input integer to a decimal string
  */
std::string uintToStr(const unsigned int &input);

/** Convert input floating point to a string
  * If parameter 'fixed' is specified, fixed decimal point will be used
  */
std::string floatToStr(const float &input, const unsigned short &fixed=0);

/** Convert input double to a string
  * If parameter 'fixed' is specified, fixed decimal point will be used
  */
std::string doubleToStr(const double &input, const unsigned short &fixed=0);

/** Convert input alpha-numerical string to uppercase
  */
std::string toUppercase(const std::string &str);

/** Convert input alpha-numerical string to lowercase
  */
std::string toLowercase(const std::string &str);

/** Strip trailing whitespace characters from an input string
  */
std::string stripWhitespace(const std::string &str);

/** Strip ALL whitespace characters from an input string
  */
std::string stripAllWhitespace(const std::string& str);

/** Remove the first occurance of a specified character from an input string
  * @return True if a character was removed from the input string
  */
bool removeCharacter(std::string& str, const char& c);

/** Remove ALL occurances of a specified character from an input string
  * @return True if at least one character was removed from the input string
  */
bool removeAllCharacters(std::string& str, const char& c);

/** Get an unsigned 8-bit integer from a user input string
  * Acceptable input formats are decimal, hexadecimal, and binary (e.g. 1234, $abcd, and b0110)
  * @return The integer converted from the input string or 0 in the event that the input string was not numerical
  */
unsigned char getUserInputUChar(const std::string& str);

/** Get an unsigned 16-bit integer from a user input string
  * Acceptable input formats are decimal, hexadecimal, and binary (e.g. 1234, $abcd, and b0110)
  * @return The integer converted from the input string or 0 in the event that the input string was not numerical
  */
unsigned short getUserInputUShort(const std::string& str);

/** Get an unsigned 32-bit integer from a user input string
  * Acceptable input formats are decimal, hexadecimal, and binary (e.g. 1234, $abcd, and b0110)
  * @return The integer converted from the input string or 0 in the event that the input string was not numerical
  */
unsigned int getUserInputUInt(const std::string& str);

/** Concatenate two 8-bit integers into an 16-bit integer and return the result
  * @param h Most significant 8-bits
  * @param l Least significant 8-bits
  * @return The resulting 16-bit integer, u16 = (h << 8) + l
  */
unsigned short getUShort(const unsigned char &h, const unsigned char &l);

/** Get the state of a bit in an input 8-bit integer
  * Parameter 'bit' expected to be in range [0, 7], but no checks are performed
  */
bool bitTest(const unsigned char &input, const unsigned char &bit);

/** Get the state of a bit in an input 16-bit integer
  * Parameter 'bit' expected to be in range [0, 15], but no checks are performed
  */
bool bitTest(const unsigned short& input, const unsigned char& bit);

/** Get the state of a bit in an input 32-bit integer
  * Parameter 'bit' expected to be in range [0, 31], but no checks are performed
  */
bool bitTest(const unsigned int& input, const unsigned char& bit);

/** Set a bit of an input 8-bit integer
  * Parameter 'bit' expected to be in range [0, 7], but no checks are performed
  */
void bitSet(unsigned char &input, const unsigned char &bit);

/** Set a bit of an input 16-bit integer
  * Parameter 'bit' expected to be in range [0, 15], but no checks are performed
  */
void bitSet(unsigned short& input, const unsigned char& bit);

/** Set a bit of an input 32-bit integer
  * Parameter 'bit' expected to be in range [0, 31], but no checks are performed
  */
void bitSet(unsigned int& input, const unsigned char& bit);

/** Reset a bit of an input 8-bit integer
  * Parameter 'bit' expected to be in range [0, 7], but no checks are performed
  */
void bitReset(unsigned char &input, const unsigned char &bit);

/** Reset a bit of an input 16-bit integer
  * Parameter 'bit' expected to be in range [0, 15], but no checks are performed
  */
void bitReset(unsigned short& input, const unsigned char& bit);

/** Reset a bit of an input 32-bit integer
  * Parameter 'bit' expected to be in range [0, 31], but no checks are performed
  */
void bitReset(unsigned int& input, const unsigned char& bit);

/** Get an 8-bit mask for bits set between 'low' and 'high' (inclusive)
  */
unsigned char getBitmask(const unsigned char &low, const unsigned char &high);

#endif
