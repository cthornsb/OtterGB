
#include <sstream>

#include "Support.hpp"

// Compute the two's compliment of an unsigned byte
short twosComp(const unsigned char &n){
	if((n & 0x80) == 0) // Positive value
		return (short)n;
	// Negative value
	return(((n & 0x007F) + (n & 0x0080)) - 0x100);
}

unsigned int splitString(const std::string &input, std::vector<std::string> &output, const unsigned char &delim/*='\t'*/){
	output.clear();
	size_t start = 0;
	size_t stop = 0;
	while(true){
		stop = input.find(delim, start);
		if(stop == std::string::npos){
			output.push_back(input.substr(start));
			break;
		}
		output.push_back(input.substr(start, stop-start));
		start = stop+1;
	}
	return output.size();
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

unsigned short getUShort(const unsigned char &h, const unsigned char &l){ return (((0xFFFF & h) << 8) + l); }
