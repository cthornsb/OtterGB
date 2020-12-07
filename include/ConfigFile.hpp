#ifndef CONFIG_FILE_HPP
#define CONFIG_FILE_HPP

#include <sstream>
#include <string>
#include <map>

class ConfigFile {
public:
	ConfigFile(){}

	ConfigFile(const std::string& fname){ read(fname); }

	bool read(const std::string& fname);

	bool search(const std::string& name, bool bRequiredArg=false);

	bool searchBoolFlag(const std::string& name);

	std::string getCurrentFilename() const { return filename; }

	std::string getCurrentParameterName() const { return currentName; }

	std::string getCurrentParameterString() const { return currentValue; }

	std::string getValue(const std::string& name) const ;

	bool getBoolFlag(const std::string& name) const;

	unsigned char getUChar(const std::string& name) const ;

	unsigned short getUShort(const std::string& name) const;

	unsigned int getUInt(const std::string& name) const;

	float getFloat(const std::string& name) const;

	double getDouble(const std::string& name) const;

	bool getBoolFlag() const;

	unsigned char getUChar() const;

	unsigned short getUShort() const;

	unsigned int getUInt() const;

	float getFloat() const;

	double getDouble() const;

	void print() const;

	std::map<std::string, std::string>::iterator begin() { return parameters.begin(); }

	std::map<std::string, std::string>::iterator end() { return parameters.end(); }

private:
	std::string filename;
	std::string currentName;
	std::string currentValue;

	std::map<std::string, std::string> parameters;
};

#endif