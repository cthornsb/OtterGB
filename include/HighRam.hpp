#ifndef HIGH_RAM_HPP
#define HIGH_RAM_HPP

#include "SystemComponent.hpp"

class HighRam : public SystemComponent {
public:
	HighRam() : 
		SystemComponent("HRAM", 0x4d415248, 127, 1) // "HRAM" (127 byte RAM)
	{ 
	}

private:
};

#endif
