#include "SystemRegisters.hpp"

// Set pointers to joypad registers
unsigned char *rJOYP = 0x0; // JOYP (Joypad register)

// Set pointers to serial data registers
unsigned char *rSB = 0x0;   // SB (Serial transfer data byte)
unsigned char *rSC = 0x0;   // SC (Serial transfer control)

// Pointers to timer registers
unsigned char *rDIV  = 0x0; // DIV (Divider register)
unsigned char *rTIMA = 0x0; // TIMA (Timer counter)
unsigned char *rTMA  = 0x0; // TMA (Timer modulo)
unsigned char *rTAC  = 0x0; // TAC (Timer control)

// Pointers to sound processor registers
unsigned char *rNR10 = 0x0; // NR10 ([TONE] Channel 1 sweep register)
unsigned char *rNR11 = 0x0; // NR11 ([TONE] Channel 1 sound length / wave pattern duty)
unsigned char *rNR12 = 0x0; // NR12 ([TONE] Channel 1 volume envelope)
unsigned char *rNR13 = 0x0; // NR13 ([TONE] Channel 1 frequency low)
unsigned char *rNR14 = 0x0; // NR14 ([TONE] Channel 1 frequency high)
unsigned char *rNR21 = 0x0; // NR21 ([TONE] Channel 2 sound length / wave pattern duty)
unsigned char *rNR22 = 0x0; // NR22 ([TONE] Channel 2 volume envelope
unsigned char *rNR23 = 0x0; // NR23 ([TONE] Channel 2 frequency low)
unsigned char *rNR24 = 0x0; // NR24 ([TONE] Channel 2 frequency high)
unsigned char *rNR30 = 0x0; // NR30 ([TONE] Channel 3 sound on/off)
unsigned char *rNR31 = 0x0; // NR31 ([WAVE] Channel 3 sound length)
unsigned char *rNR32 = 0x0; // NR32 ([WAVE] Channel 3 select output level)
unsigned char *rNR33 = 0x0; // NR33 ([WAVE] Channel 3 frequency low)
unsigned char *rNR34 = 0x0; // NR34 ([WAVE] Channel 3 frequency high)
unsigned char *rNR41 = 0x0; // NR41 ([NOISE] Channel 4 sound length)
unsigned char *rNR42 = 0x0; // NR42 ([NOISE] Channel 4 volume envelope)
unsigned char *rNR43 = 0x0; // NR43 ([NOISE] Channel 4 polynomial counter)
unsigned char *rNR44 = 0x0; // NR44 ([NOISE] Channel 4 counter / consecutive, initial)
unsigned char *rNR50 = 0x0; // NR50 (Channel control / ON-OFF / volume)
unsigned char *rNR51 = 0x0; // NR51 (Select sound output)
unsigned char *rNR52 = 0x0; // NR52 (Sound ON-OFF)
unsigned char *rWAVE = 0x0; // [Wave] pattern RAM (FF30-FF3F)

// Pointers to GPU registers
unsigned char *rLCDC = 0x0; // LCDC (LCD Control Register)
unsigned char *rSTAT = 0x0; // STAT (LCDC Status Register)
unsigned char *rSCY  = 0x0; // SCY (Scroll Y)
unsigned char *rSCX  = 0x0; // SCX (Scroll X)
unsigned char *rLY   = 0x0; // LY (LCDC Y-coordinate) [read-only]
unsigned char *rLYC  = 0x0; // LYC (LY Compare)
unsigned char *rBGP  = 0x0; // BGP (BG palette data, non-gbc mode only)
unsigned char *rOBP0 = 0x0; // OBP0 (Object palette 0 data, non-gbc mode only)
unsigned char *rOBP1 = 0x0; // OBP1 (Object palette 1 data, non-gbc mode only)
unsigned char *rWY   = 0x0; // WY (Window Y Position)
unsigned char *rWX   = 0x0; // WX (Window X Position (minus 7))
unsigned char *rVBK  = 0x0; // VBK (VRAM bank select, gbc mode)
unsigned char *rBGPI = 0x0; // BCPS/BGPI (Background palette index, gbc mode)
unsigned char *rBGPD = 0x0; // BCPD/BGPD (Background palette data, gbc mode)
unsigned char *rOBPI = 0x0; // OCPS/OBPI (Sprite palette index, gbc mode)
unsigned char *rOBPD = 0x0; // OCPD/OBPD (Sprite palette index, gbc mode)

// Pointers to system registers	
unsigned char *rIF    = 0x0; // Interrupt Flag
unsigned char *rDMA   = 0x0; // DMA transfer from ROM/RAM to OAM
unsigned char *rKEY1  = 0x0; // Speed switch register
unsigned char *rHDMA1 = 0x0; // New DMA source, high byte (GBC only)
unsigned char *rHDMA2 = 0x0; // New DMA source, low byte (GBC only)
unsigned char *rHDMA3 = 0x0; // New DMA destination, high byte (GBC only)
unsigned char *rHDMA4 = 0x0; // New DMA destination, low byte (GBC only)
unsigned char *rHDMA5 = 0x0; // New DMA source, length/mode/start (GBC only)
unsigned char *rRP    = 0x0; // Infrared comms port (not used)
unsigned char *rSVBK  = 0x0; // WRAM bank select
unsigned char *rIE    = 0x0; // Interrupt enable
unsigned char *rIME   = 0x0; // Master interrupt enable

// Pointers to undocumented registers
unsigned char *rFF6C = 0x0;	
unsigned char *rFF72 = 0x0;
unsigned char *rFF73 = 0x0;
unsigned char *rFF74 = 0x0;
unsigned char *rFF75 = 0x0;
unsigned char *rFF76 = 0x0;
unsigned char *rFF77 = 0x0;

// Gameboy Color flag
bool bGBCMODE = false;
