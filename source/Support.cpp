
#include <sstream>

#include "Support.hpp"

const char LOWERCASE_LOW  = 97;
const char LOWERCASE_HIGH = 122;

const char UPPERCASE_LOW  = 65;
const char UPPERCASE_HIGH = 90;

bool isNumeric(const std::string& str) {
	for (auto i = 0; i < str.length(); i++)
		if ((str[i] < '0' || str[i] > '9') && str[i] != '.')
			return false;
	return true;
}

bool isInteger(const std::string& str) {
	for (auto i = 0; i < str.length(); i++)
		if (str[i] < '0' || str[i] > '9')
			return false;
	return true;
}

bool isDecimal(const std::string& str) {
	for (auto i = 0; i < str.length(); i++)
		if ((str[i] < '0' || str[i] > '9') && str[i] != '.')
			return false;
	return true;
}

bool isHexadecimal(const std::string& str) {
	if (str.length() < 2 || str.find('$') == std::string::npos)
		return false;
	for (auto i = 0; i < str.length(); i++) {
		if((str[i] < '0' || str[i] > '9') && (str[i] < 'a' || str[i] > 'f') && (str[i] < 'A' || str[i] > 'F') && str[i] != '$')
			return false;
	}
	return true;
}

bool isBinary(const std::string& str) {
	if (str.length() < 2 || str.find('b') == std::string::npos)
		return false;
	for (auto i = 0; i < str.length(); i++)
		if (str[i] != '0' && str[i] != '1' && str[i] != 'b')
			return false;
	return true;
}

bool isNotNumeric(const std::string& str) {
	return (!isDecimal(str) && !isHexadecimal(str) && !isBinary(str));
}

short twosComp(const unsigned char &n){
	// Compute the two's compliment of an unsigned byte
	if((n & 0x80) == 0) // Positive value
		return (short)n;
	// Negative value
	return(((n & 0x007F) + (n & 0x0080)) - 0x100);
}

unsigned int splitString(const std::string &input, std::vector<std::string> &output, const unsigned char &delim/*='\t'*/){
	output.clear();
	size_t start = 0;
	size_t stop = 0;
	std::string temp;
	while(true){
		stop = input.find(delim, start);
		if(stop == std::string::npos){
			temp = input.substr(start);
			if (!temp.empty())
				output.push_back(temp);
			break;
		}
		temp = input.substr(start, stop - start);
		if(!temp.empty())
			output.push_back(temp);
		start = stop+1;
	}
	return (unsigned int)output.size();
}

std::string extractString(std::string& str, const char& c1, const char& c2, const std::string& repstr/*=""*/) {
	size_t index1 = str.find_last_of(c1);
	if (index1 == std::string::npos) {
		return "";
	}
	index1;
	size_t index2 = str.find_first_of(c2, index1 + 1);
	if (index2 == std::string::npos) {
		return "";
	}
	std::string retval = str.substr(index1 + 1, index2 - (index1 + 1));
	str.replace(index1, retval.length() + 2, repstr);
	return retval;
}

unsigned int countOccurances(const std::string& str, const char& c) {
	unsigned int retval = 0;
	for (auto strc = str.begin(); strc != str.end(); strc++) {
		if (*strc == c)
			retval++;
	}
	return retval;
}

std::string getHex(const unsigned char &input){
	std::stringstream stream;
	stream << std::hex << (int)input;
	std::string output = stream.str();
	if(output.length() < 2) output = "0" + output;
	return ("$" + output);
}

std::string getHex(const unsigned short &input){
	std::stringstream stream;
	stream << std::hex << (int)input;
	std::string output = stream.str();
	output = (output.length() < 4 ? std::string(4-output.length(), '0') : "") + output;
	return ("$" + output);
}

std::string getBinary(const unsigned char &input, const int &startBit/*=0*/){
	std::stringstream stream;
	for(int i = 7; i >= startBit; i--)
		stream << ((input & (0x1 << i)) != 0);
	return stream.str();
}

std::string getBinary(const unsigned short &input, const int &startBit/*=0*/){
	std::stringstream stream;
	for(int i = 15; i >= startBit; i--)
		stream << ((input & (0x1 << i)) != 0);
	return stream.str();
}

std::string ucharToStr(const unsigned char &input){
	std::stringstream stream;
	stream << (int)input;
	return stream.str();
}

std::string ushortToStr(const unsigned short &input){
	std::stringstream stream;
	stream << input;
	return stream.str();
}

std::string uintToStr(const unsigned int &input){
	std::stringstream stream;
	stream << input;
	return stream.str();
}

std::string floatToStr(const float &input, const unsigned short &fixed/*=0*/){
	std::stringstream stream;
	if(fixed != 0){
		stream.precision(fixed);
		stream << std::fixed;
	}
	stream << input;
	return stream.str();
}

std::string doubleToStr(const double &input, const unsigned short &fixed/*=0*/){
	std::stringstream stream;
	if(fixed != 0){
		stream.precision(fixed);
		stream << std::fixed;
	}
	stream << input;
	return stream.str();
}

std::string toUppercase(const std::string &str){
	std::string retval = str;
	for(size_t i = 0; i < str.length(); i++){
		if(retval[i] >= LOWERCASE_LOW && str[i] <= LOWERCASE_HIGH)
			retval[i] = (str[i]-LOWERCASE_LOW)+UPPERCASE_LOW;
	}
	return retval;
}

std::string toLowercase(const std::string &str){
	std::string retval = str;
	for(size_t i = 0; i < str.length(); i++){
		if(retval[i] >= UPPERCASE_LOW && str[i] <= UPPERCASE_HIGH)
			retval[i] = (str[i]-UPPERCASE_LOW)+LOWERCASE_LOW;
	}
	return retval;
}

std::string stripWhitespace(const std::string &str){
	return str.substr(0, str.find_last_not_of(' ')+1);
}

std::string stripAllWhitespace(const std::string& str) {
	std::string retval = "";
	for (auto c = str.begin(); c != str.end(); c++) {
		if (*c != ' ' && *c != '\n' && *c != '\t') {
			retval += *c;
		}
	}
	return retval;
}

bool removeCharacter(std::string& str, const char& c){
	size_t index = str.find(c);
	if(index != std::string::npos){
		str.replace(index, 1, "");
		return true;
	}
	return false;
}

bool removeAllCharacters(std::string& str, const char& c) {
	int counter = 0;
	while (removeCharacter(str, c)) {
		counter++;
	}
	return (counter != 0);
}

unsigned char getUserInputUChar(const std::string& str){
	return (unsigned char)getUserInputUShort(str);
}

unsigned short getUserInputUShort(const std::string& str){
	return (unsigned short)getUserInputUInt(str);
}

unsigned int getUserInputUInt(const std::string& str) {
	std::string input = str;
	if (isHexadecimal(str)) { // Hex
		removeCharacter(input, '$');
		return stoul(input, 0, 16);
	}
	else if (isBinary(str)) { // Binary
		removeCharacter(input, 'b');
		return stoul(input, 0, 2);
	}
	else if (isNumeric(str)) { // Decimal
		return stoul(input, 0, 10);
	}
	return 0;
}

unsigned short getUShort(const unsigned char &h, const unsigned char &l){ 
	return (((0xFFFF & h) << 8) + l); 
}

bool bitTest(const unsigned char &input, const unsigned char &bit){
	return ((input & (0x1 << bit)) == (0x1 << bit));
}

bool bitTest(const unsigned short& input, const unsigned char& bit) {
	return ((input & (0x1 << bit)) == (0x1 << bit));
}

bool bitTest(const unsigned int& input, const unsigned char& bit) {
	return ((input & (0x1 << bit)) == (0x1 << bit));
}

void bitSet(unsigned char &input, const unsigned char &bit){
	input |= (0x1 << bit);
}

void bitSet(unsigned short& input, const unsigned char& bit) {
	input |= (0x1 << bit);
}

void bitSet(unsigned int& input, const unsigned char& bit) {
	input |= (0x1 << bit);
}

void bitReset(unsigned char &input, const unsigned char &bit){
	input &= ~(0x1 << bit);
}

void bitReset(unsigned short& input, const unsigned char& bit) {
	input &= ~(0x1 << bit);
}

void bitReset(unsigned int& input, const unsigned char& bit) {
	input &= ~(0x1 << bit);
}

unsigned char getBitmask(const unsigned char &low, const unsigned char &high){
	unsigned char mask = 0;
	for(unsigned char i = low; i <= high; i++)
		mask |= (1 << i);
	return mask;
}
