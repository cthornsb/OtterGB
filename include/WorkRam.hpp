#ifndef WORKRAM_HPP
#define WORKRAM_HPP

#include "SystemComponent.hpp"

class WorkRam : public SystemComponent {
public:
	WorkRam();

	virtual unsigned char *getPtr(const unsigned int &loc);

	virtual bool preWriteAction();
	
	virtual bool preReadAction();

private:
};

#endif
