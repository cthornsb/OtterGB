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
	addConsoleCommand("quit", 0, cmdType::QUIT, "", "Exit console");
	addConsoleCommand("exit", 0, cmdType::QUIT, "", "Exit console");
	addConsoleCommand("help", 0, cmdType::HELP, "[cmd]", "Print list of commands or syntax for (cmd)");
	addConsoleCommand("a",    0, cmdType::REG8, "[val]", "Print A register");
	addConsoleCommand("b",    0, cmdType::REG8, "[val]", "Print B register");
	addConsoleCommand("c",    0, cmdType::REG8, "[val]", "Print C register");
	addConsoleCommand("d",    0, cmdType::REG8, "[val]", "Print D register");
	addConsoleCommand("e",    0, cmdType::REG8, "[val]", "Print E register");
	addConsoleCommand("f",    0, cmdType::REG8, "[val]", "Print F register");
	addConsoleCommand("h",    0, cmdType::REG8, "[val]", "Print H register");
	addConsoleCommand("l",    0, cmdType::REG8, "[val]", "Print L register");
	addConsoleCommand("d8",   0, cmdType::REG8, "[val]", "Print d8 immediate");
	addConsoleCommand("af",   0, cmdType::REG16, "[val]", "Print AF register");
	addConsoleCommand("bc",   0, cmdType::REG16, "[val]", "Print BC register");
	addConsoleCommand("de",   0, cmdType::REG16, "[val]", "Print DE register");
	addConsoleCommand("hl",   0, cmdType::REG16, "[val]", "Print HL register");
	addConsoleCommand("pc",   0, cmdType::REG16, "[val]", "Print program counter");
	addConsoleCommand("sp",   0, cmdType::REG16, "[val]", "Print stack pointer");
	addConsoleCommand("d16",  0, cmdType::REG16, "[val]", "Print d16 immediate");
	addConsoleCommand("inst", 0, cmdType::INST, "", "Print instruction");
	addConsoleCommand("read", 1, cmdType::READ, "<addr>", "Read byte at address");
	addConsoleCommand("write",2, cmdType::WRITE, "<addr> <val>", "Write byte to address");
	addConsoleCommand("rreg", 1, cmdType::READREG, "<reg>", "Read system register");
	addConsoleCommand("wreg", 2, cmdType::WRITEREG, "<reg> <val>", "Write system register");
	addConsoleCommand("hex",  1, cmdType::HEX, "<val>", "Convert value to hex");
	addConsoleCommand("bin",  1, cmdType::BIN, "<val>", "Convert value to binary");
	addConsoleCommand("dec",  1, cmdType::DEC, "<val>", "Convert value to decimal");
	addConsoleCommand("cls",  0, cmdType::CLS, "", "Clear screen");
	addConsoleCommand("reset",0, cmdType::RESET, "", "Reset emulator");
	addConsoleCommand("qsave",0, cmdType::QSAVE, "[fname]", "Quicksave");
	addConsoleCommand("qload",0, cmdType::QLOAD, "[fname]", "Quickload");
	addConsoleCommand("dir"  ,0, cmdType::DIRECTORY, "[path]", "Print ROM directory");
	addConsoleCommand("file" ,0, cmdType::FILENAME, "[fname]", "Print ROM filename");
	put('>');
}

void ConsoleGBC::setSystem(SystemGBC* ptr) { 
	sys = ptr;

	// Add parser definitions for all system registers
	std::vector<Register>* registers = sys->getRegisters();
	for (auto reg = registers->begin(); reg != registers->end(); reg++) {
		parser.addExternalDefinition(toLowercase(reg->getName()), CPPTYPE::UINT8, reg->getPtr());
	}

	// Add parser definitions for all cpu registers
	LR35902* cpu = sys->getCPU();
	parser.addExternalDefinition("a", CPPTYPE::UINT8, cpu->getPointerToRegister8bit("a"));
	parser.addExternalDefinition("b", CPPTYPE::UINT8, cpu->getPointerToRegister8bit("b"));
	parser.addExternalDefinition("c", CPPTYPE::UINT8, cpu->getPointerToRegister8bit("c"));
	parser.addExternalDefinition("d", CPPTYPE::UINT8, cpu->getPointerToRegister8bit("d"));
	parser.addExternalDefinition("e", CPPTYPE::UINT8, cpu->getPointerToRegister8bit("e"));
	parser.addExternalDefinition("f", CPPTYPE::UINT8, cpu->getPointerToRegister8bit("f"));
	parser.addExternalDefinition("h", CPPTYPE::UINT8, cpu->getPointerToRegister8bit("h"));
	parser.addExternalDefinition("l", CPPTYPE::UINT8, cpu->getPointerToRegister8bit("l"));
	parser.addExternalDefinition("pc", CPPTYPE::UINT16, cpu->getPointerToRegister16bit("pc"));
	parser.addExternalDefinition("sp", CPPTYPE::UINT16, cpu->getPointerToRegister16bit("sp"));
}

void ConsoleGBC::newline(){
	buffer.pop_front();
	//if(nX > 1)
	//	line += buffer.back().substr(1, nX-1);
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
		newline();
		if(flag){ // User input
			handleInput();
			put('>');
		}
		else{ // Printing output
			line = "";
		}
	}
	else if(c == 0x8){ // Backspace
		if(!line.empty()) // Pop off the last character
			line.pop_back();
		unput();
	}
	else{
		line += c;
		put(c);
	}
}

void ConsoleGBC::handleInput(){
	//handle user input commands here
	LR35902 *cpu = sys->getCPU();
	Register* reg = 0x0;
	std::vector<std::string> args;
	if (parser.isExpression(line)) {
		NumericalString value;
		if (parser.parse(line, value)) {
			if(value.type == NUMTYPE::INTEGER)
				(*this) << value.getUInt() << "\n";
			else if(value.type == NUMTYPE::BOOLEAN)
				(*this) << (value.getBool() ? "true" : "false") << "\n";
		}
		else {
			(*this) << "invalid syntax\n";
		}
		line = "";
		return;
	}
	unsigned int nArgs = splitString(line, args, ' ');
	line = "";
	if(!nArgs)
		return;
	std::string cmdstr = args.front();
	auto iter = commands.find(cmdstr);
	if(iter == commands.end()){
		// CPU opcodes
		OpcodeData data;
		if(cpu->findOpcode(line, data)){ // Valid LR35902 opcode found
			(*this) << data.getShortInstruction() << "\n";
			while(data.executing()){
				data.clock(cpu);
			}
			return;
		}
		// System registers
		std::string regname = toUppercase(args.front());
		Register* reg = sys->getRegisterByName(regname);
		if(reg){
			if(nArgs >= 2){ // Write register
				reg->write(getUserInputUChar(args.at(1)));
			}
			else{ // Read register
				(*this) << getHex(reg->read()) << " (" << getBinary(reg->read()) << ")\n";
			}
			return;
		}
		(*this) << "unknown command\n";
		return;
	}
	ConsoleCommand *cmd = &iter->second;
	if(nArgs-1 < cmd->getRequiredArgs()){
		(*this) << "syntax error\n";
		(*this) << cmd->getName() << " " << cmd->getArgStr() << "\n";
		return;
	}
	unsigned char d8;
	unsigned short d16;
	switch(cmd->getType()){
		case cmdType::NONE:
			break;
		case cmdType::QUIT:
			sys->closeDebugConsole();
			break;
		case cmdType::HELP:
			if(nArgs >= 2){ // User specified command to print syntax for
				for(auto allcmd = commands.cbegin(); allcmd != commands.cend(); allcmd++){
					if(allcmd->first == args.at(1)){
						(*this) << "syntax:\n";
						(*this) << " " << allcmd->second.getName() << " " << allcmd->second.getArgStr() << "\n";
						return;
					}
				}
				(*this) << "unknown command\n";
			}
			else{
				d16 = 0;
				clear();
				for(auto allcmd = commands.cbegin(); allcmd != commands.cend(); allcmd++){
					if(d16++ >= nRows-1)
						continue;
					(*this) << allcmd->first << "\n";
				}
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
		case cmdType::READREG:
			reg = sys->getRegisterByName(args.at(1));
			if(reg){
				(*this) << getHex(reg->getValue()) << "\n";
			}
			else{
				(*this) << "undefined register\n";
			}
			break;
		case cmdType::WRITEREG:
			reg = sys->getRegisterByName(args.at(1));
			if(reg){
				reg->setValue(getUserInputUChar(args.at(2)));
			}
			else{
				(*this) << "undefined register\n";
			}
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
		case cmdType::RESET: // Reset
			sys->reset();
			break;
		case cmdType::QSAVE: // Quicksave
			if(nArgs >= 2){ // User specified filename
				sys->quicksave(args.at(1));
			}
			else{
				sys->quicksave();
			}
			break;
		case cmdType::QLOAD: // Quickload
			if(nArgs >= 2){ // User specified filename
				sys->quickload(args.at(1));
			}
			else{
				sys->quickload();
			}
			break;
		case cmdType::DIRECTORY: // Display / set ROM directory
			if(nArgs >= 2){ // User specified filename
				sys->setRomDirectory(args.at(1));
			}
			else{
				(*this) << sys->getRomDirectory() << "/\n";
			}
			break;
		case cmdType::FILENAME: // Display / set ROM filename
			if(nArgs >= 2){ // User specified filename
				sys->setRomFilename(args.at(1));
			}
			else{
				(*this) << sys->getRomFilename() + "." + sys->getRomExtension() << "\n";
			}
			break;
		default:
			break;
	}
}

void ConsoleGBC::addConsoleCommand(const std::string& name, const unsigned short& args, const cmdType& type, const std::string& argstr, const std::string& helpstr){
	commands[name] = ConsoleCommand(name, args, type, argstr, helpstr);
}

void ConsoleGBC::flush(){
	for(size_t i = 0; i < strbuff.length(); i++){
		handle(strbuff[i], false);
	}
	strbuff = "";
}

void ConsoleGBC::clear(){
	buffer.clear();
	for(unsigned short i = 0; i < nRows; i++)
		buffer.push_back(std::string(nCols, ' '));
}
