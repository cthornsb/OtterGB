#include <string>

#include "SystemGBC.hpp"

int main(int argc, char *argv[]){
	SystemGBC gbc;
	
	if(argc <= 1)
		gbc.execute("../../roms/cpu_instrs.gb");
	else
		gbc.execute(std::string(argv[1]));

	return 0;
}
