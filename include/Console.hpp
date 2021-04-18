#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <string>
#include <sstream>

#include <OTTConsole.hpp>

class SystemGBC;

class ConsoleGBC : public OTTConsole {
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
	
protected:
	SystemGBC* sys; ///< Pointer to emulator system

	void onUserAddCommands() override;

	void onUserPrompt() override;

	/** Handle unknown commands.
	  * If false is returned, the console will print an "unknown command" message
	  */
	bool onUserUnknownCommand(const std::vector<std::string>&) override;

	void onUserHandleInput(ConsoleCommand* cmd, const std::vector<std::string>& args) override;
};

#endif
