#ifndef SYSTEM_REGISTERS_HPP
#define SYSTEM_REGISTERS_HPP

// Set pointers to joypad registers
extern unsigned char *rJOYP; // JOYP (Joypad register)

// Set pointers to serial data registers
extern unsigned char *rSB;   // SB (Serial transfer data byte)
extern unsigned char *rSC;   // SC (Serial transfer control)

// Pointers to timer registers
extern unsigned char *rDIV;  // DIV (Divider register)
extern unsigned char *rTIMA; // TIMA (Timer counter)
extern unsigned char *rTMA;  // TMA (Timer modulo)
extern unsigned char *rTAC;  // TAC (Timer control)

// Pointers to sound processor registers
extern unsigned char *rNR10; // NR10 ([TONE] Channel 1 sweep register)
extern unsigned char *rNR11; // NR11 ([TONE] Channel 1 sound length / wave pattern duty)
extern unsigned char *rNR12; // NR12 ([TONE] Channel 1 volume envelope)
extern unsigned char *rNR13; // NR13 ([TONE] Channel 1 frequency low)
extern unsigned char *rNR14; // NR14 ([TONE] Channel 1 frequency high)
extern unsigned char *rNR21; // NR21 ([TONE] Channel 2 sound length / wave pattern duty)
extern unsigned char *rNR22; // NR22 ([TONE] Channel 2 volume envelope
extern unsigned char *rNR23; // NR23 ([TONE] Channel 2 frequency low)
extern unsigned char *rNR24; // NR24 ([TONE] Channel 2 frequency high)
extern unsigned char *rNR30; // NR30 ([TONE] Channel 3 sound on/off)
extern unsigned char *rNR31; // NR31 ([WAVE] Channel 3 sound length)
extern unsigned char *rNR32; // NR32 ([WAVE] Channel 3 select output level)
extern unsigned char *rNR33; // NR33 ([WAVE] Channel 3 frequency low)
extern unsigned char *rNR34; // NR34 ([WAVE] Channel 3 frequency high)
extern unsigned char *rNR41; // NR41 ([NOISE] Channel 4 sound length)
extern unsigned char *rNR42; // NR42 ([NOISE] Channel 4 volume envelope)
extern unsigned char *rNR43; // NR43 ([NOISE] Channel 4 polynomial counter)
extern unsigned char *rNR44; // NR44 ([NOISE] Channel 4 counter / consecutive, initial)
extern unsigned char *rNR50; // NR50 (Channel control / ON-OFF / volume)
extern unsigned char *rNR51; // NR51 (Select sound output)
extern unsigned char *rNR52; // NR52 (Sound ON-OFF)
extern unsigned char *rWAVE; // [Wave] pattern RAM (FF30-FF3F)

// Pointers to GPU registers
extern unsigned char *rLCDC; // LCDC (LCD Control Register)
extern unsigned char *rSTAT; // STAT (LCDC Status Register)
extern unsigned char *rSCY;  // SCY (Scroll Y)
extern unsigned char *rSCX;  // SCX (Scroll X)
extern unsigned char *rLY;   // LY (LCDC Y-coordinate) [read-only]
extern unsigned char *rLYC;  // LYC (LY Compare)
extern unsigned char *rBGP;  // BGP (BG palette data, non-gbc mode only)
extern unsigned char *rOBP0; // OBP0 (Object palette 0 data, non-gbc mode only)
extern unsigned char *rOBP1; // OBP1 (Object palette 1 data, non-gbc mode only)
extern unsigned char *rWY;   // WY (Window Y Position)
extern unsigned char *rWX;   // WX (Window X Position (minus 7))
extern unsigned char *rVBK;  // VBK (VRAM bank select, gbc mode)
extern unsigned char *rBGPI; // BCPS/BGPI (Background palette index, gbc mode)
extern unsigned char *rBGPD; // BCPD/BGPD (Background palette data, gbc mode)
extern unsigned char *rOBPI; // OCPS/OBPI (Sprite palette index, gbc mode)
extern unsigned char *rOBPD; // OCPD/OBPD (Sprite palette index, gbc mode)

// Pointers to system registers	
extern unsigned char *rIF ;  // Interrupt Flag
extern unsigned char *rDMA;  // DMA transfer from ROM/RAM to OAM
extern unsigned char *rKEY1; // Speed switch register
extern unsigned char *rHDMA1; // New DMA source, high byte (GBC only)
extern unsigned char *rHDMA2; // New DMA source, low byte (GBC only)
extern unsigned char *rHDMA3; // New DMA destination, high byte (GBC only)
extern unsigned char *rHDMA4; // New DMA destination, low byte (GBC only)
extern unsigned char *rHDMA5; // New DMA source, length/mode/start (GBC only)
extern unsigned char *rRP;   // Infrared comms port (not used)
extern unsigned char *rSVBK; // WRAM bank select
extern unsigned char *rIE;   // Interrupt enable
extern unsigned char *rIME;  // Master interrupt enable

// Pointers to undocumented registers
extern unsigned char *rFF6C;	
extern unsigned char *rFF72;	
extern unsigned char *rFF73;	
extern unsigned char *rFF74;	
extern unsigned char *rFF75;	
extern unsigned char *rFF76;	
extern unsigned char *rFF77;

// Gameboy Color flag
extern bool bGBCMODE;

#endif
