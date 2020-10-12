#ifndef WORKRAM_HPP
#define WORKRAM_HPP

#include "SystemComponent.hpp"

class WorkRam : public SystemComponent {
public:
	WorkRam();

	virtual bool preWriteAction();
	
	virtual bool preReadAction();

	virtual void postWriteAction(){ }

	virtual void postReadAction(){ }

private:
};

#endif
