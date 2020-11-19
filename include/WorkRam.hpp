#ifndef WORKRAM_HPP
#define WORKRAM_HPP

#include "SystemComponent.hpp"

class Register;

class WorkRam : public SystemComponent {
public:
	WorkRam();

	virtual unsigned char *getPtr(const unsigned int &loc);

	virtual bool preWriteAction();
	
	virtual bool preReadAction();

	virtual void defineRegisters();

	virtual bool writeRegister(const unsigned short &reg, const unsigned char &val);

	virtual bool readRegister(const unsigned short &reg, unsigned char &dest);

private:
};

#endif
