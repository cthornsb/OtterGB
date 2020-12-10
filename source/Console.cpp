#include <iostream>

#include "SystemGBC.hpp"
#include "LR35902.hpp"
#include "Console.hpp"
#include "Graphics.hpp"
#include "Support.hpp"

ConsoleGBC::ConsoleGBC() :
	CharacterMap(),
	nCols(20), 
	nRows(18), 
	nX(0), 
	nY(0),
	strbuff(),
	line(),
	buffer(nRows, std::string(nCols, ' '))
{ 
	addConsoleCommand("quit", 0, cmdType::QUIT, "", "Exit emulator");
	addConsoleCommand("exit", 0, cmdType::QUIT, "", "Exit emulator");
	addConsoleCommand("help", 0, cmdType::HELP, "", "Exit emulator");
	addConsoleCommand("a",    0, cmdType::REG8, "", "Exit emulator");
	addConsoleCommand("b",    0, cmdType::REG8, "", "Exit emulator");
	addConsoleCommand("c",    0, cmdType::REG8, "", "Exit emulator");
	addConsoleCommand("d",    0, cmdType::REG8, "", "Exit emulator");
	addConsoleCommand("e",    0, cmdType::REG8, "", "Exit emulator");
	addConsoleCommand("f",    0, cmdType::REG8, "", "Exit emulator");
	addConsoleCommand("h",    0, cmdType::REG8, "", "Exit emulator");
	addConsoleCommand("l",    0, cmdType::REG8, "", "Exit emulator");
	addConsoleCommand("d8",   0, cmdType::REG8, "", "Exit emulator");
	addConsoleCommand("af",   0, cmdType::REG16, "", "Exit emulator");
	addConsoleCommand("bc",   0, cmdType::REG16, "", "Exit emulator");
	addConsoleCommand("de",   0, cmdType::REG16, "", "Exit emulator");
	addConsoleCommand("hl",   0, cmdType::REG16, "", "Exit emulator");
	addConsoleCommand("pc",   0, cmdType::REG16, "", "Exit emulator");
	addConsoleCommand("sp",   0, cmdType::REG16, "", "Exit emulator");
	addConsoleCommand("d16",  0, cmdType::REG16, "", "Exit emulator");
	addConsoleCommand("inst", 0, cmdType::INST, "", "Exit emulator");
	addConsoleCommand("rd",   1, cmdType::READ, "<reg,val>", "Set reg");
	addConsoleCommand("wt",   2, cmdType::WRITE, "<reg,val>", "Set reg");
	addConsoleCommand("hex",  1, cmdType::HEX, "<reg,val>", "Set reg");
	addConsoleCommand("bin",  1, cmdType::BIN, "<reg,val>", "Set reg");
	addConsoleCommand("dec",  1, cmdType::DEC, "<reg,val>", "Set reg");
	addConsoleCommand("cls",  0, cmdType::CLS, "<reg,val>", "Set reg");
	addConsoleCommand("res",  0, cmdType::RES, "<reg,val>", "Set reg");
	put('>');
}

void ConsoleGBC::newline(){
	buffer.pop_front();
	if(nX > 1)
		line += buffer.back().substr(1, nX-1);
	buffer.push_back(std::string(nCols, ' '));
	nX = 0;
}

void ConsoleGBC::put(const char &c){
	if(nX >= nCols) // New line
		newline();
	buffer.back()[nX++] = c;
	if(nX >= nCols)
		newline();
	buffer.back()[nX] = '_';
}

void ConsoleGBC::unput(){
	if(nX <= 1){ // Do nothing (for now)
		return;
	}
	buffer.back()[nX--] = ' ';
	buffer.back()[nX] = '_';
}

void ConsoleGBC::update(){
	// Poll the screen controller to check for button presses.
	KeyStates *keys = window->getKeypress();
	char keypress;
	while(keys->get(keypress)){
		handle(keypress);
	}
}

void ConsoleGBC::draw(){
	for(unsigned short y = 0; y < nRows; y++){
		for(unsigned short x = 0; x < nCols; x++){
			putCharacter(buffer[y][x], x, y);
		}
	}
}

void ConsoleGBC::handle(const char& c, bool flag/*=true*/){
	if(c == '\t'){ // Tab
		for(unsigned short i = 0; i < (nX/4+1)*4-nX; i++)
			put(' ');
	}
	else if(c == '\n' || c == '\r'){ // New line
		buffer.back()[nX] = ' ';
		buffer.pop_front();
		if(flag && nX > 1)
			line += buffer.back().substr(1, nX-1);
		buffer.push_back(std::string(nCols, ' '));
		nX = 0;
		if(flag){
			handleInput();
			put('>');
		}
	}
	else if(c == 0x8){ // Backspace
		unput();
	}
	else{
		put(c);
	}
}

void ConsoleGBC::handleInput(){
	//handle user input commands here
	LR35902 *cpu = sys->getCPU();
	std::vector<std::string> args;
	std::string userinput = toLowercase(line);
	unsigned int nArgs = splitString(userinput, args, ' ');
	line = "";
	if(!nArgs)
		return;
	std::string cmdstr = args.front();
	auto iter = commands.find(cmdstr);
	if(iter == commands.end()){
		// CPU opcodes
		OpcodeData data;
		if(cpu->findOpcode(userinput, data)){ // Valid LR35902 opcode found
			(*this) << data.getShortInstruction() << "\n";
			while(data.executing()){
				data.clock(cpu);
			}
			return;
		}
		// System registers
		std::vector<Register>* registers = sys->getRegisters();
		std::string regname = toUppercase(args.front());
		for(auto reg = registers->begin(); reg != registers->end(); reg++){
			if(regname == reg->getName()){
				if(nArgs >= 2){ // Write register
					reg->write(getUserInputUChar(args.at(1)));
				}
				else{ // Read register
					(*this) << getHex(reg->read()) << " (" << getBinary(reg->read()) << ")\n";
				}
				return;
			}
		}
		(*this) << "unknown command\n";
		return;
	}
	ConsoleCommand *cmd = &iter->second;
	if(nArgs-1 < cmd->getRequiredArgs()){
		(*this) << "syntax error\n";
		return;
	}
	unsigned char d8;
	unsigned short d16;
	switch(cmd->getType()){
		case cmdType::QUIT:
			sys->quit();
			break;
		case cmdType::HELP:
			d16 = 0;
			clear();
			for(auto allcmd = commands.begin(); allcmd != commands.end(); allcmd++){
				if(d16++ >= nRows-1)
					continue;
				(*this) << allcmd->first << "\n";
			}
			break;
		case cmdType::REG8: // 8 bit cpu registers
			if(nArgs >= 2){
				cpu->setRegister8bit(args.front(), getUserInputUChar(args.at(1)));
			}
			else{
				cpu->getRegister8bit(args.front(), d8);
				(*this) << getHex(d8) << "\n";
			}
			break;
		case cmdType::REG16: // 16 bit cpu registers
			if(nArgs >= 2){
				cpu->setRegister16bit(args.front(), getUserInputUChar(args.at(1)));
			}
			else{
				cpu->getRegister16bit(args.front(), d16);
				(*this) << getHex(d16) << "\n";
			}
			break;
		case cmdType::INST:
			(*this) << cpu->getInstruction() << "\n";
			break;
		case cmdType::READ:
			if(args.at(1).find('(') != std::string::npos){ // Indirect address
				addrGetFunc func = cpu->getMemoryAddressFunction(args.at(1));
				d16 = (cpu->*func)();
			}
			else
				d16 = getUserInputUShort(args.at(1));
			sys->read(d16, d8);
			(*this) << getHex(d8) << "\n";
			break;
		case cmdType::WRITE:
			if(args.at(1).find('(') != std::string::npos){ // Indirect address
				addrGetFunc func = cpu->getMemoryAddressFunction(args.at(1));
				d16 = (cpu->*func)();
			}
			else
				d16 = getUserInputUShort(args.at(1));
			sys->write(d16, d8);
			break;
		case cmdType::HEX:
			d16 = getUserInputUShort(args.at(1));
			if(d16 > 255) // 16-bit
				(*this) << getHex(d16) << "\n";
			else // 8-bit
				(*this) << getHex((unsigned char)d16) << "\n";
			break;
		case cmdType::BIN:
			d16 = getUserInputUShort(args.at(1));
			if(d16 > 255) // 16-bit
				(*this) << getBinary(d16) << "\n";
			else // 8-bit
				(*this) << getBinary((unsigned char)d16) << "\n";
			break;
		case cmdType::DEC:
			(*this) << getUserInputUShort(args.at(1)) << "\n";
			break;
		case cmdType::CLS: // clear screen
			clear();
			break;
		case cmdType::RES: // Resume
			sys->closeDebugConsole();
			break;
	}
}

void ConsoleGBC::addConsoleCommand(const std::string& name, const unsigned short& args, const cmdType& type, const std::string& argstr, const std::string& helpstr){
	commands[name] = ConsoleCommand(name, args, type, argstr, helpstr);
}

void ConsoleGBC::flush(){
	for(auto i = 0; i < strbuff.length(); i++){
		handle(strbuff[i], false);
	}
	strbuff = "";
}

void ConsoleGBC::clear(){
	buffer.clear();
	for(unsigned short i = 0; i < nRows; i++)
		buffer.push_back(std::string(nCols, ' '));
}
