#ifndef SYSTEM_REGISTERS_HPP
#define SYSTEM_REGISTERS_HPP

#include "Register.hpp"

// Pointer to joypad register
extern Register *rJOYP; ///< JOYP (Joypad register)

// Pointers to serial registers
extern Register *rSB; ///< SB (Serial transfer data byte)
extern Register *rSC; ///< SC (Serial transfer control)

// Pointers to timer registers
extern Register *rDIV;  ///< DIV (Divider register)
extern Register *rTIMA; ///< TIMA (Timer counter)
extern Register *rTMA;  ///< TMA (Timer modulo)
extern Register *rTAC;  ///< TAC (Timer control)

// Pointers to DMA registers
extern Register *rDMA;   ///< DMA transfer from ROM/RAM to OAM
extern Register *rHDMA1; ///< New DMA source, high byte (GBC only)
extern Register *rHDMA2; ///< New DMA source, low byte (GBC only)
extern Register *rHDMA3; ///< New DMA destination, high byte (GBC only)
extern Register *rHDMA4; ///< New DMA destination, low byte (GBC only)
extern Register *rHDMA5; ///< New DMA source, length/mode/start (GBC only)

// Pointers to GPU registers
extern Register *rLCDC; ///< LCDC (LCD Control Register)
extern Register *rSTAT; ///< STAT (LCDC Status Register)
extern Register *rSCY;  ///< SCY (Scroll Y)
extern Register *rSCX;  ///< SCX (Scroll X)
extern Register *rLY;   ///< LY (LCDC Y-coordinate) [read-only]
extern Register *rLYC;  ///< LYC (LY Compare)
extern Register *rBGP;  ///< BGP (BG palette data, non-gbc mode only)
extern Register *rOBP0; ///< OBP0 (Object palette 0 data, non-gbc mode only)
extern Register *rOBP1; ///< OBP1 (Object palette 1 data, non-gbc mode only)
extern Register *rWY;   ///< WY (Window Y Position)
extern Register *rWX;   ///< WX (Window X Position (minus 7))
extern Register *rVBK;  ///< VBK (VRAM bank select, gbc mode)
extern Register *rBGPI; ///< BCPS/BGPI (Background palette index, gbc mode)
extern Register *rBGPD; ///< BCPD/BGPD (Background palette data, gbc mode)
extern Register *rOBPI; ///< OCPS/OBPI (Sprite palette index, gbc mode)
extern Register *rOBPD; ///< OCPD/OBPD (Sprite palette index, gbc mode)

// Pointers to sound processor registers
extern Register *rNR10; ///< NR10 ([TONE] Channel 1 sweep register)
extern Register *rNR11; ///< NR11 ([TONE] Channel 1 sound length / wave pattern duty)
extern Register *rNR12; ///< NR12 ([TONE] Channel 1 volume envelope)
extern Register *rNR13; ///< NR13 ([TONE] Channel 1 frequency low)
extern Register *rNR14; ///< NR14 ([TONE] Channel 1 frequency high)
extern Register *rNR20; ///< Not used
extern Register *rNR21; ///< NR21 ([TONE] Channel 2 sound length / wave pattern duty)
extern Register *rNR22; ///< NR22 ([TONE] Channel 2 volume envelope
extern Register *rNR23; ///< NR23 ([TONE] Channel 2 frequency low)
extern Register *rNR24; ///< NR24 ([TONE] Channel 2 frequency high)
extern Register *rNR30; ///< NR30 ([TONE] Channel 3 sound on/off)
extern Register *rNR31; ///< NR31 ([WAVE] Channel 3 sound length)
extern Register *rNR32; ///< NR32 ([WAVE] Channel 3 select output level)
extern Register *rNR33; ///< NR33 ([WAVE] Channel 3 frequency low)
extern Register *rNR34; ///< NR34 ([WAVE] Channel 3 frequency high)
extern Register *rNR40; ///< Not used
extern Register *rNR41; ///< NR41 ([NOISE] Channel 4 sound length)
extern Register *rNR42; ///< NR42 ([NOISE] Channel 4 volume envelope)
extern Register *rNR43; ///< NR43 ([NOISE] Channel 4 polynomial counter)
extern Register *rNR44; ///< NR44 ([NOISE] Channel 4 counter / consecutive, initial)
extern Register *rNR50; ///< NR50 (Channel control / ON-OFF / volume)
extern Register *rNR51; ///< NR51 (Select sound output)
extern Register *rNR52; ///< NR52 (Sound ON-OFF)
extern Register *rWAVE[16]; ///< [Wave] pattern RAM (FF30-FF3F)

// Pointers to system registers	
extern Register *rIF;   ///< Interrupt Flag
extern Register *rKEY1; ///< Speed switch register
extern Register *rRP;   ///< Infrared comms port (not used)
extern Register *rIE;   ///< Interrupt enable
extern Register *rIME;  ///< Master interrupt enable
extern Register *rSVBK; ///< Work RAM bank number

// Pointers to undocumented registers
extern Register *rFF6C;	
extern Register *rFF72;	
extern Register *rFF73;
extern Register *rFF74;	
extern Register *rFF75;	
extern Register *rFF76;	
extern Register *rFF77;

// Gameboy Color flag
extern bool bGBCMODE;

// Double-speed flag
extern bool bCPUSPEED;

// Window scanline
extern Register *rWLY;

#endif
