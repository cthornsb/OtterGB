#include <OTTWindow.hpp>
#include <OTTKeyboard.hpp>

#include "SystemGBC.hpp"
#include "LR35902.hpp"
#include "Console.hpp"
#include "Support.hpp"
#include "Cartridge.hpp"

enum cmdType {
	NONE,
	CLOSE,     ///< Close console
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
	RESET,     ///< Reset emulator
	QSAVE,     ///< Quick-save
	QLOAD,     ///< Quick-load
	DIRECTORY, ///< ROM directory
	FILENAME,  ///< ROM filename
	VSYNC,     ///< Toggle VSync on/off
};


ConsoleGBC::ConsoleGBC() :
	OTTConsole(20, 18),
	sys(0x0)
{ 
	initialize();
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

void ConsoleGBC::onUserAddCommands() {
	addConsoleCommand("close", 0, cmdType::CLOSE, "", "Close console");
	addConsoleCommand("about", 0, cmdType::ABOUT, "", "Print program information");
	addConsoleCommand("version", 0, cmdType::ABOUT, "", "Print program information");
	addConsoleCommand("a", 0, cmdType::REG8, "[val]", "Print A register");
	addConsoleCommand("b", 0, cmdType::REG8, "[val]", "Print B register");
	addConsoleCommand("c", 0, cmdType::REG8, "[val]", "Print C register");
	addConsoleCommand("d", 0, cmdType::REG8, "[val]", "Print D register");
	addConsoleCommand("e", 0, cmdType::REG8, "[val]", "Print E register");
	addConsoleCommand("f", 0, cmdType::REG8, "[val]", "Print F register");
	addConsoleCommand("h", 0, cmdType::REG8, "[val]", "Print H register");
	addConsoleCommand("l", 0, cmdType::REG8, "[val]", "Print L register");
	addConsoleCommand("d8", 0, cmdType::REG8, "[val]", "Print d8 immediate");
	addConsoleCommand("af", 0, cmdType::REG16, "[val]", "Print AF register");
	addConsoleCommand("bc", 0, cmdType::REG16, "[val]", "Print BC register");
	addConsoleCommand("de", 0, cmdType::REG16, "[val]", "Print DE register");
	addConsoleCommand("hl", 0, cmdType::REG16, "[val]", "Print HL register");
	addConsoleCommand("pc", 0, cmdType::REG16, "[val]", "Print program counter");
	addConsoleCommand("sp", 0, cmdType::REG16, "[val]", "Print stack pointer");
	addConsoleCommand("d16", 0, cmdType::REG16, "[val]", "Print d16 immediate");
	addConsoleCommand("inst", 0, cmdType::INST, "", "Print instruction");
	addConsoleCommand("read", 1, cmdType::READ, "<addr>", "Read byte at address");
	addConsoleCommand("write", 2, cmdType::WRITE, "<addr> <val>", "Write byte to address");
	addConsoleCommand("rreg", 1, cmdType::READREG, "<reg>", "Read system register");
	addConsoleCommand("wreg", 2, cmdType::WRITEREG, "<reg> <val>", "Write system register");
	addConsoleCommand("hex", 1, cmdType::HEX, "<val>", "Convert value to hex");
	addConsoleCommand("bin", 1, cmdType::BIN, "<val>", "Convert value to binary");
	addConsoleCommand("dec", 1, cmdType::DEC, "<val>", "Convert value to decimal");
	addConsoleCommand("reset", 0, cmdType::RESET, "", "Reset emulator");
	addConsoleCommand("qsave", 0, cmdType::QSAVE, "[fname]", "Quicksave");
	addConsoleCommand("qload", 0, cmdType::QLOAD, "[fname]", "Quickload");
	addConsoleCommand("dir", 0, cmdType::DIRECTORY, "[path]", "Print ROM directory");
	addConsoleCommand("file", 0, cmdType::FILENAME, "[fname]", "Print ROM filename");
	addConsoleCommand("vsync", 0, cmdType::VSYNC, "", "Toggle VSync on or off");
}

void ConsoleGBC::onUserPrompt() {
	put('>');
}

bool ConsoleGBC::onUserUnknownCommand(const std::vector<std::string>& args) {
	// CPU opcodes
	OpcodeData data;
	if (sys->getCPU()->findOpcode(line, data)) { // Valid LR35902 opcode found
		sys->getCPU()->getOpcodeHandler()->execute(&data);
		(*this) << data.getShortInstruction() << "\n";
		return true;
	}
	// System registers
	std::string regname = toUppercase(args.front());
	Register* reg = sys->getRegisterByName(regname);
	if (reg) { // System register
		if (args.size() >= 2) { // Write register
			reg->write(getUserInputUChar(args.at(1)));
		}
		else { // Read register
			(*this) << getHex(reg->read()) << " (" << getBinary(reg->read()) << ")\n";
		}
		return true;
	}
	return false;
}

void ConsoleGBC::onUserHandleInput(ConsoleCommand* cmd, const std::vector<std::string>& args) {
	//handle user input commands here
	LR35902 *cpu = sys->getCPU();
	Register* reg = 0x0;
	std::string cmdstr = cmd->getName();
	unsigned char d8;
	unsigned short d16;
	switch(cmd->getID()){
		case cmdType::NONE:
			break;
		case cmdType::CLOSE:
			if(sys->getCartridge()->isLoaded())
				sys->closeDebugConsole();
			else
				(*this) << "No ROM loaded\n";
			break;
		case cmdType::ABOUT:
			(*this) << "OtterGB v" << sys->getVersionString() << "\n";
			(*this) << "by C Thornsberry\n";
			(*this) << "github.com/cthornsb\n";
			break;
		case cmdType::REG8: // 8 bit cpu registers
			if(args.size() >= 2){
				cpu->setRegister8bit(args.front(), getUserInputUChar(args.at(1)));
			}
			else{
				cpu->getRegister8bit(args.front(), d8);
				(*this) << getHex(d8) << "\n";
			}
			break;
		case cmdType::REG16: // 16 bit cpu registers
			if(args.size() >= 2){
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
			else // Absolute address
				d16 = getUserInputUShort(args.at(1));
			sys->read(d16, d8);
			(*this) << getHex(d8) << "\n";
			break;
		case cmdType::WRITE:
			if(args.at(1).find('(') != std::string::npos){ // Indirect address
				addrGetFunc func = cpu->getMemoryAddressFunction(args.at(1));
				d16 = (cpu->*func)();
			}
			else { // Absolute address
				d16 = getUserInputUShort(args.at(1));
				d8 = getUserInputUChar(args.at(2));
			}
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
		case cmdType::RESET: // Reset
			if(sys->getCartridge()->isLoaded()){
				if (sys->reset())
					sys->closeDebugConsole();
				else
					(*this) << "Reset failed\n";
			}
			else{
				(*this) << "No ROM loaded\n";
			}
			break;
		case cmdType::QSAVE: // Quicksave
			if(args.size() >= 2){ // User specified filename
				sys->quicksave(args.at(1));
			}
			else{
				sys->quicksave();
			}
			break;
		case cmdType::QLOAD: // Quickload
			if(args.size() >= 2){ // User specified filename
				sys->quickload(args.at(1));
			}
			else{
				sys->quickload();
			}
			break;
		case cmdType::DIRECTORY: // Display / set ROM directory
			if(args.size() >= 2){ // User specified filename
				sys->setRomDirectory(args.at(1));
			}
			else{
				(*this) << sys->getRomDirectory() << "/\n";
			}
			break;
		case cmdType::FILENAME: // Display / set ROM filename
			if(args.size() >= 2){ // User specified filename
				sys->setRomFilename(args.at(1));
				if(sys->reset())
					sys->closeDebugConsole();
				else
					(*this) << "Failed to load ROM\n";
			}
			else{
				(*this) << sys->getRomFilename() + "." + sys->getRomExtension() << "\n";
			}
			break;
		case cmdType::VSYNC: // Toggle VSync on/off
			if (!window->getVSync()) {
				sys->enableVSync();
				(*this) << "vsync enabled\n";
			}
			else {
				sys->disableVSync();
				(*this) << "vsync disabled\n";
			}
			break;
		default:
			break;
	}
}
