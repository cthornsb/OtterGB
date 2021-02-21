#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <string>
#include <sstream>
#include <deque>
#include <map>

#include "Bitmap.hpp"
#include "TextParser.hpp"

class SystemGBC;

enum class cmdType{ 
	NONE, 
	QUIT,      ///< Exit emulator
	CLOSE,     ///< Close console
	HELP,      ///< Print help information
	REG8,      ///< Get or set 8-bit register
	REG16,     ///< Get or set 16-bit register
	INST,      ///< Print most recent instruction
	READ,      ///< Read from memory
	WRITE,     ///< Write to memory
	READREG,   ///< Read system register
	WRITEREG,  ///< Write system register
	HEX,       ///< Convert a value to hexadecimal
	BIN,       ///< Convert a value to binary
	DEC,       ///< Convert a value to decimal
	CLS,       ///< Clear terminal
	RESET,     ///< Reset emulator
	QSAVE,     ///< Quick-save
	QLOAD,     ///< Quick-load
	DIRECTORY, ///< ROM directory
	FILENAME   ///< ROM filename
};

class ConsoleCommand{
public:
	ConsoleCommand() :
		sName(),
		sArgs(),
		sHelp(),
		nReqArgs(0),
		eType(cmdType::NONE)
	{
	}

	ConsoleCommand(const std::string& name, const unsigned short& args, const cmdType& type, const std::string& argstr, const std::string& helpstr) :
		sName(name),
		sArgs(argstr),
		sHelp(helpstr),
		nReqArgs(args),
		eType(type)
	{
	}

	unsigned short getRequiredArgs() const { 
		return nReqArgs; 
	}
	
	bool operator == (const std::string& name) const { 
		return (sName == name); 
	}

	cmdType getType() const { 
		return eType; 
	}

	std::string getName() const {
		return sName;
	}
	
	std::string getArgStr() const {
		return sArgs;
	}

	std::string getHelpStr() const {
		return sHelp;
	}

private:
	std::string sName;
	std::string sArgs;
	std::string sHelp;
	
	unsigned short nReqArgs;
	
	cmdType eType;
};


class ConsoleGBC : public CharacterMap {
public:
	ConsoleGBC();
	
	void setSystem(SystemGBC* ptr);
	
	template <typename T>
	ConsoleGBC& operator << (const T& val){
		std::stringstream stream;
		stream << val;
		char c;
		while(stream.get(c)){
			strbuff += c;
		}
		flush();	
		return (*this);
	}

	void update();
	
	void draw();
	
private:
	unsigned short nCols;
	unsigned short nRows;
	
	unsigned short nX;
	unsigned short nY;
	
	std::string strbuff;

	std::string line;

	SystemGBC *sys;

	std::deque<std::string> buffer;

	std::map<std::string, ConsoleCommand> commands;

	TextParser parser;

	void put(const char &c);
	
	void unput();
	
	void newline();
	
	void handle(const char& c, bool flag=true);

	void handleInput();
	
	void addConsoleCommand(const std::string& name, const unsigned short& args, const cmdType& type, const std::string& argstr, const std::string& helpstr);
	
	void flush();

	void clear();
};

#endif
