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
	ABOUT,     ///< Print program details
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
	CLS,       ///< Clear console
	RESET,     ///< Reset emulator
	QSAVE,     ///< Quick-save
	QLOAD,     ///< Quick-load
	DIRECTORY, ///< ROM directory
	FILENAME,  ///< ROM filename
	VSYNC,     ///< Toggle VSync on/off
	ECHO       ///< Echo input commands back to the console
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
	bool bEcho; ///< Set if command input will be echoed back

	unsigned short nCols; ///< Number of console columns
	
	unsigned short nRows; ///< Number of console rows
	
	unsigned short nX; ///< Current console column position
	
	unsigned short nY; ///< Current console row position
	
	std::string strbuff; ///< Console character stream buffer

	std::string line; ///< Currently active console line

	SystemGBC *sys; ///< Pointer to emulator system

	std::deque<std::string> buffer; ///< Console row strings

	std::map<std::string, ConsoleCommand> commands; ///< Map of all console commands

	TextParser parser; ///< Text input parser

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
