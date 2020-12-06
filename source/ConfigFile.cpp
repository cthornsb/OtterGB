#include <iostream>
#include <fstream>

#include "ConfigFile.hpp"
#include "Support.hpp"

bool ConfigFile::read(const std::string& fname){
	filename = fname;
	std::ifstream ifile(filename.c_str());
	if (!ifile.good())
		return false;
	std::string line;
	unsigned short lineCount = 0;
	while (true) {
		std::getline(ifile, line);
		lineCount++;
		if (ifile.eof() || !ifile.good())
			break;
		if (line.empty() || line[0] == '#') // Comments or blank lines
			continue;
		std::vector<std::string> args;
		splitString(line, args, ' ');
		unsigned short argCount = 0;
		for(auto arg = args.begin(); arg != args.end(); arg++){
			if (arg->empty())
				continue;
			else if (argCount == 0)
				parameters[args.front()] = "";
			else if (argCount == 1)
				parameters[args.front()] = *arg;
			else {
				std::cout << " Warning! Extraneous arguments passed to parameter name \"" << args.front() << "\" on line " << lineCount << std::endl;
				break;
			}
			argCount++;
		}
	}
	ifile.close();
	return true;
}

bool ConfigFile::search(const std::string& name, bool bRequiredArg/*=false*/) {
	auto element = parameters.find(name);
	if (element != parameters.end()) {
		currentName = element->first;
		currentValue = element->second;
		if (bRequiredArg && currentValue.empty()) {
			std::cout << " Warning! Missing required argument to parameter \"" << currentName << "\"\n";
			return false;
		}
		return true;
	}
	return false;
}

std::string ConfigFile::getFirstValue(const std::string& name) const {
	auto element = parameters.find(name);
	if (element != parameters.end())
		return element->second;
	return "";
}

bool ConfigFile::getBoolFlag(const std::string& name) const {
	std::string input = toLowercase(getFirstValue(name));
	return (input == "true" || std::stoul(input) == 1);
}

unsigned char ConfigFile::getUChar(const std::string& name) const {
	return (unsigned char)std::stoul(getFirstValue(name));
}

unsigned short ConfigFile::getUShort(const std::string& name) const {
	return (unsigned short)std::stoul(getFirstValue(name));
}

unsigned int ConfigFile::getUInt(const std::string& name) const {
	return std::stoul(getFirstValue(name));
}

float ConfigFile::getFloat(const std::string& name) const {
	return std::stof(getFirstValue(name));
}

double ConfigFile::getDouble(const std::string& name) const {
	return std::stod(getFirstValue(name));
}

bool ConfigFile::getBoolFlag() const {
	std::string lower = toLowercase(currentValue);
	return (lower == "true" || std::stoul(lower) == 1);
}

unsigned char ConfigFile::getUChar() const {
	return (unsigned char)std::stoul(currentValue);
}

unsigned short ConfigFile::getUShort() const {
	return (unsigned short)std::stoul(currentValue);
}

unsigned int ConfigFile::getUInt() const {
	return std::stoul(currentValue);
}

float ConfigFile::getFloat() const {
	return std::stof(currentValue);
}

double ConfigFile::getDouble() const {
	return std::stod(currentValue);
}

void ConfigFile::print() const {
	for (auto arg = parameters.begin(); arg != parameters.end(); arg++) {
		std::cout << arg->first << "\t\"" << arg->second << "\"\n";
	}
}