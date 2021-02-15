#ifndef WORKRAM_HPP
#define WORKRAM_HPP

#include "SystemComponent.hpp"

class Register;

class WorkRam : public SystemComponent {
public:
	WorkRam();

	unsigned char *getPtr(const unsigned short &loc) override ;

	bool preWriteAction() override ;
	
	bool preReadAction() override ;

	void defineRegisters() override ;

	bool writeRegister(const unsigned short &reg, const unsigned char &val) override ;

	bool readRegister(const unsigned short &reg, unsigned char &dest) override ;

private:
};

#endif
