#ifndef LR35902_HPP
#define LR35902_HPP

#include <string>

#include "SystemComponent.hpp"

extern const unsigned char FLAG_Z_BIT;
extern const unsigned char FLAG_S_BIT;
extern const unsigned char FLAG_H_BIT;
extern const unsigned char FLAG_C_BIT;

extern const unsigned char FLAG_Z_MASK;
extern const unsigned char FLAG_S_MASK;
extern const unsigned char FLAG_H_MASK;
extern const unsigned char FLAG_C_MASK;

// GBC/GB cartridge
class Cartridge;

class LR35902 : public SystemComponent {
public:
	LR35902() : SystemComponent() { }

	bool initialize();

	unsigned short execute(Cartridge *cart);

	void setProgramCounter(const unsigned short &pc){ PC = pc; }

	unsigned short getProgramCounter() const { return PC; }

	// Should this be public??
	void callInterruptVector(const unsigned char &offset);

protected:
	unsigned char A; // Accumulator
	unsigned char B; // B register
	unsigned char C; // C register
	unsigned char D; // D register
	unsigned char E; // E register
	unsigned char H; // H register
	unsigned char L; // L register

	unsigned char F; // Flags register

	unsigned char d8; // 8-bit immediate data
	unsigned char d16h; // High 8 bits of 16-bit immediate data
	unsigned char d16l; // Low 8 bits of 16-bit immediate data

	unsigned short SP; // Stack Pointer (16-bit)
	unsigned short PC; // Program Counter (16-bit)
	unsigned short nCycles; ///< Instruction clock cycle counter

	std::string opcodes[256]; // Name of each opcode (for debugging)
	std::string opcodesCB[256]; // Name of each CB-prefix opcode (for debugging)

	void (LR35902::*funcPtr[256])(); // Opcode functions for LR35902 processor
	void (LR35902::*funcPtrCB[256])(); // CB-Prefix opcode functions for LR35902 processor

	bool halfCarry;
	bool fullCarry;

	void setFlag(const unsigned char &bit, bool state=true);

	void setFlags(bool Z, bool S, bool H, bool C);

	unsigned short get_d16();

	unsigned short getAF();

	unsigned short getBC();

	unsigned short getDE();

	unsigned short getHL();

	unsigned short setAF(const unsigned short &val);

	unsigned short setBC(const unsigned short &val);

	unsigned short setDE(const unsigned short &val);

	unsigned short setHL(const unsigned short &val);

	void rlc_d8(unsigned char *arg);

	void rrc_d8(unsigned char *arg);

	void rl_d8(unsigned char *arg);

	void rr_d8(unsigned char *arg);

	void res_d8(unsigned char *arg, const unsigned char &bit);

	void set_d8(unsigned char *arg, const unsigned char &bit);

	void inc_d16(unsigned char *addrH, unsigned char *addrL);

	void dec_d16(unsigned char *addrH, unsigned char *addrL);

	void inc_d8(unsigned char *arg);

	void dec_d8(unsigned char *arg);

	void jr_n(unsigned char *n);
	
	void jr_cc_n(unsigned char *n);

	void ld_d8(unsigned char *dest, unsigned char *src);

	void ld_SP_d16(unsigned char *addrH, unsigned char *addrL);

	void add_A_d8(unsigned char *arg);

	void add_A_aHL();

	void add_HL_d16(unsigned char *addrH, unsigned char *addrL);

	void adc_A_d8(unsigned char *arg);

	void sub_A_d8(unsigned char *arg);

	void sbc_A_d8(unsigned char *arg);

	void and_d8(unsigned char *arg);

	void xor_d8(unsigned char *arg);

	void or_d8(unsigned char *arg);

	void cp_d8(unsigned char *arg);

	void push_d16(unsigned char *addrH, unsigned char *addrL);

	void pop_d16(unsigned char *addrH, unsigned char *addrL);

	void jp_d16(unsigned char *addrH, unsigned char *addrL);

	void jp_cc_d16(unsigned char *addrH, unsigned char *addrL);

	void call_a16(unsigned char *addrH, unsigned char *addrL);

	void call_cc_a16(unsigned char *addrH, unsigned char *addrL);

	void rst_n(unsigned char n);

	void ret();

	void ret_cc();

	void getCarries(unsigned char *arg1, unsigned char *arg2, bool sub=false);

	void getCarries(unsigned char *arg1, bool sub=false);

	void sla_d8(unsigned char *arg);

	void sra_d8(unsigned char *arg);

	void swap_d8(unsigned char *arg);

	void srl_d8(unsigned char *arg);

	void bit_d8(unsigned char *arg, const unsigned char &bit);
	
	/////////////////////////////////////////////////////////////////////
	// 
	/////////////////////////////////////////////////////////////////////

	// NOP

	void NOP(){ }

	// INC BC[DE|HL|(HL)]

	void INC_BC(){ inc_d16(&B, &C); }
	void INC_DE(){ inc_d16(&D, &E); }
	void INC_HL(){ inc_d16(&H, &L); }
	void INC_aHL();

	// DEC BC[DE|HL|(HL)]

	void DEC_BC(){ dec_d16(&B, &C); }
	void DEC_DE(){ dec_d16(&D, &E); }
	void DEC_HL(){ dec_d16(&H, &L); }
	void DEC_aHL();

	// INC A[B|C|D|E|H|L]

	void INC_A(){ inc_d8(&A); }
	void INC_B(){ inc_d8(&B); }
	void INC_C(){ inc_d8(&C); }
	void INC_D(){ inc_d8(&D); }
	void INC_E(){ inc_d8(&E); }
	void INC_H(){ inc_d8(&H); }
	void INC_L(){ inc_d8(&L); }

	// DEC A[B|C|D|E|H|L]

	void DEC_A(){ dec_d8(&A); }
	void DEC_B(){ dec_d8(&B); }
	void DEC_C(){ dec_d8(&C); }
	void DEC_D(){ dec_d8(&D); }
	void DEC_E(){ dec_d8(&E); }
	void DEC_H(){ dec_d8(&H); }
	void DEC_L(){ dec_d8(&L); }

	// RLCA | RLCA | RRA | RRCA

	void RLA(){ rl_d8(&A); }
	void RLCA(){ rlc_d8(&A); }
	void RRA(){ rr_d8(&A); }
	void RRCA(){ rrc_d8(&A); }

	// STOP 0

	void STOP_0(){ }

	// JR

	void JR_r8(){ jr_n(&d8); }
	void JR_NZ_r8(){ if((F & FLAG_Z_MASK) == 0) jr_cc_n(&d8); }
	void JR_Z_r8(){ if((F & FLAG_Z_MASK) != 0) jr_cc_n(&d8); }
	void JR_NC_r8(){ if((F & FLAG_C_MASK) == 0) jr_cc_n(&d8); }
	void JR_C_r8(){ if((F & FLAG_C_MASK) != 0) jr_cc_n(&d8); }
	
	// DAA

	void DAA();

	// CPL

	void CPL();

	// INC SP

	void INC_SP();

	// SCF

	void SCF();
	
	// ADD SP,d8
	
	void ADD_SP_r8();
	
	// LD d16,A 
	
	void LD_a16_A();
	
	// LD HL,SP+r8 or LDHL SP,r8

	void LD_HL_SP_r8();

	// LD HL,d16
	
	void LD_HL_d16();
	
	// LD d16 SP
	
	void LD_a16_SP();
	
	// LD SP,d16[HL]
	
	void LD_SP_d16(){ ld_SP_d16(&d16h, &d16l); }
	void LD_SP_HL(){ ld_SP_d16(&H, &L); }

	// ADD HL,BC[DE|HL|SP]
	
	void ADD_HL_BC(){ add_HL_d16(&B, &C); }
	void ADD_HL_HL(){ add_HL_d16(&H, &L); }
	void ADD_HL_DE(){ add_HL_d16(&D, &E); }
	void ADD_HL_SP();
	
	// LD HL-,A or LDD HL,A

	void LDD_aHL_A();

	// LD A,HL- or LDD A,HL

	void LDD_A_aHL();

	// LD HL+,A or LDI HL,A

	void LDI_aHL_A();
	
	// LD A,HL+ or LDI A,HL
	
	void LDI_A_aHL();

	// LDH d8,A

	void LDH_a8_A();

	// LDH A,d8

	void LDH_A_a8();

	// DEC SP

	void DEC_SP(){ SP--; }

	// CCF

	void CCF(){ setFlag(FLAG_C_BIT, (F & FLAG_C_MASK) == 0); }

	// LD B,A[B|C|D|E|H|L|d8]

	void LD_B_A(){ ld_d8(&B, &A); }
	void LD_B_B(){ ld_d8(&B, &B); }
	void LD_B_C(){ ld_d8(&B, &C); }
	void LD_B_D(){ ld_d8(&B, &D); }
	void LD_B_E(){ ld_d8(&B, &E); }
	void LD_B_H(){ ld_d8(&B, &H); }
	void LD_B_L(){ ld_d8(&B, &L); }
	void LD_B_d8(){ ld_d8(&B, &d8); }

	// LD C,A[B|C|D|E|H|L|d8]

	void LD_C_A(){ ld_d8(&C, &A); }
	void LD_C_B(){ ld_d8(&C, &B); }
	void LD_C_C(){ ld_d8(&C, &C); }
	void LD_C_D(){ ld_d8(&C, &D); }
	void LD_C_E(){ ld_d8(&C, &E); }
	void LD_C_H(){ ld_d8(&C, &H); }
	void LD_C_L(){ ld_d8(&C, &L); }
	void LD_C_d8(){ ld_d8(&C, &d8); }

	// LD (C),A

	void LD_aC_A();

	// LD D,A[B|C|D|E|H|L|d8]

	void LD_D_A(){ ld_d8(&D, &A); }
	void LD_D_B(){ ld_d8(&D, &B); }
	void LD_D_C(){ ld_d8(&D, &C); }
	void LD_D_D(){ ld_d8(&D, &D); }
	void LD_D_E(){ ld_d8(&D, &E); }
	void LD_D_H(){ ld_d8(&D, &H); }
	void LD_D_L(){ ld_d8(&D, &L); }
	void LD_D_d8(){ ld_d8(&D, &d8); }

	// LD E,A[B|C|D|E|H|L|d8]

	void LD_E_A(){ ld_d8(&E, &A); }
	void LD_E_B(){ ld_d8(&E, &B); }
	void LD_E_C(){ ld_d8(&E, &C); }
	void LD_E_D(){ ld_d8(&E, &D); }
	void LD_E_E(){ ld_d8(&E, &E); }
	void LD_E_H(){ ld_d8(&E, &H); }
	void LD_E_L(){ ld_d8(&E, &L); }
	void LD_E_d8(){ ld_d8(&E, &d8); }

	// LD H,A[B|C|D|E|H|L|d8]

	void LD_H_A(){ ld_d8(&H, &A); }
	void LD_H_B(){ ld_d8(&H, &B); }
	void LD_H_C(){ ld_d8(&H, &C); }
	void LD_H_D(){ ld_d8(&H, &D); }
	void LD_H_E(){ ld_d8(&H, &E); }
	void LD_H_H(){ ld_d8(&H, &H); }
	void LD_H_L(){ ld_d8(&H, &L); }
	void LD_H_d8(){ ld_d8(&H, &d8); }

	// LD L,A[B|C|D|E|H|L|d8]

	void LD_L_A(){ ld_d8(&L, &A); }
	void LD_L_B(){ ld_d8(&L, &B); }
	void LD_L_C(){ ld_d8(&L, &C); }
	void LD_L_D(){ ld_d8(&L, &D); }
	void LD_L_E(){ ld_d8(&L, &E); }
	void LD_L_H(){ ld_d8(&L, &H); }
	void LD_L_L(){ ld_d8(&L, &L); }
	void LD_L_d8(){ ld_d8(&L, &d8); }

	// LD HL,A[B|C|D|E|H|L|d8]

	void ld_aHL_d8(unsigned char *arg);

	void LD_aHL_A(){ ld_aHL_d8(&A); }
	void LD_aHL_B(){ ld_aHL_d8(&B); }
	void LD_aHL_C(){ ld_aHL_d8(&C); }
	void LD_aHL_D(){ ld_aHL_d8(&D); }
	void LD_aHL_E(){ ld_aHL_d8(&E); }
	void LD_aHL_H(){ ld_aHL_d8(&H); }
	void LD_aHL_L(){ ld_aHL_d8(&L); }
	void LD_aHL_d8(){ ld_aHL_d8(&d8); }
	void LD_aHL_d16();

	void ld_d8_aHL(unsigned char *arg);

	// LD B[C|D|E|H|L],(HL)

	void LD_B_aHL(){ ld_d8_aHL(&B); }
	void LD_C_aHL(){ ld_d8_aHL(&C); }
	void LD_D_aHL(){ ld_d8_aHL(&D); }
	void LD_E_aHL(){ ld_d8_aHL(&E); }
	void LD_H_aHL(){ ld_d8_aHL(&H); }
	void LD_L_aHL(){ ld_d8_aHL(&L); }

	// LD BC,A[d16]

	void LD_BC_d16();
	
	// LD DE,A[d16]

	void LD_DE_d16();

	void LD_aBC_A();

	void LD_aDE_A();

	// HALT

	void HALT(){ }

	// LD A,A[B|C|D|E|H|L|d8]

	void LD_A_d8(){ ld_d8(&A, &d8); }
	void LD_A_A(){ ld_d8(&A, &A); }
	void LD_A_B(){ ld_d8(&A, &B); }
	void LD_A_C(){ ld_d8(&A, &C); }
	void LD_A_D(){ ld_d8(&A, &D); }
	void LD_A_E(){ ld_d8(&A, &E); }
	void LD_A_H(){ ld_d8(&A, &H); }
	void LD_A_L(){ ld_d8(&A, &L); }

	void ld_A_a16(unsigned char *addrH, unsigned char *addrL);

	void LD_A_aC();
	void LD_A_aBC(){ ld_A_a16(&B, &C); }
	void LD_A_aDE(){ ld_A_a16(&D, &E); }
	void LD_A_aHL(){ ld_A_a16(&H, &L); }
	void LD_A_a16(){ ld_A_a16(&d16h, &d16l); }

	// ADD A,A[B|C|D|E|H|L]

	void ADD_A_d8(){ add_A_d8(&d8); }
	void ADD_A_A(){ add_A_d8(&A); }
	void ADD_A_B(){ add_A_d8(&B); }
	void ADD_A_C(){ add_A_d8(&C); }
	void ADD_A_D(){ add_A_d8(&D); }
	void ADD_A_E(){ add_A_d8(&E); }
	void ADD_A_H(){ add_A_d8(&H); }
	void ADD_A_L(){ add_A_d8(&L); }

	void ADD_A_aHL();

	// ADC A,A[B|C|D|E|H|L]

	void ADC_A_d8(){ adc_A_d8(&d8); }
	void ADC_A_A(){ adc_A_d8(&A); }
	void ADC_A_B(){ adc_A_d8(&B); }
	void ADC_A_C(){ adc_A_d8(&C); }
	void ADC_A_D(){ adc_A_d8(&D); }
	void ADC_A_E(){ adc_A_d8(&E); }
	void ADC_A_H(){ adc_A_d8(&H); }
	void ADC_A_L(){ adc_A_d8(&L); }
	
	void ADC_A_aHL();

	// SUB A[B|C|D|E|H|L]

	void SUB_d8(){ sub_A_d8(&d8); }
	void SUB_A(){ sub_A_d8(&A); }
	void SUB_B(){ sub_A_d8(&B); }
	void SUB_C(){ sub_A_d8(&C); }
	void SUB_D(){ sub_A_d8(&D); }
	void SUB_E(){ sub_A_d8(&E); }
	void SUB_H(){ sub_A_d8(&H); }
	void SUB_L(){ sub_A_d8(&L); }
	
	void SUB_aHL();

	// SBC A,A[B|C|D|E|H|L]

	void SBC_A_d8(){ sbc_A_d8(&d8); }
	void SBC_A_A(){ sbc_A_d8(&A); }
	void SBC_A_B(){ sbc_A_d8(&B); }
	void SBC_A_C(){ sbc_A_d8(&C); }
	void SBC_A_D(){ sbc_A_d8(&D); }
	void SBC_A_E(){ sbc_A_d8(&E); }
	void SBC_A_H(){ sbc_A_d8(&H); }
	void SBC_A_L(){ sbc_A_d8(&L); }
	
	void SBC_A_aHL();

	// AND A[B|C|D|E|H|L]

	void AND_d8(){ and_d8(&d8); }
	void AND_A(){ and_d8(&A); }
	void AND_B(){ and_d8(&B); }
	void AND_C(){ and_d8(&C); }
	void AND_D(){ and_d8(&D); }
	void AND_E(){ and_d8(&E); }
	void AND_H(){ and_d8(&H); }
	void AND_L(){ and_d8(&L); }
	
	void AND_aHL();

	// XOR A[B|C|D|E|H|L]

	void XOR_d8(){ xor_d8(&d8); }
	void XOR_A(){ xor_d8(&A); }
	void XOR_B(){ xor_d8(&B); }
	void XOR_C(){ xor_d8(&C); }
	void XOR_D(){ xor_d8(&D); }
	void XOR_E(){ xor_d8(&E); }
	void XOR_H(){ xor_d8(&H); }
	void XOR_L(){ xor_d8(&L); }
	
	void XOR_aHL();

	// OR A[B|C|D|E|H|L]

	void OR_d8(){ or_d8(&d8); }
	void OR_A(){ or_d8(&A); }
	void OR_B(){ or_d8(&B); }
	void OR_C(){ or_d8(&C); }
	void OR_D(){ or_d8(&D); }
	void OR_E(){ or_d8(&E); }
	void OR_H(){ or_d8(&H); }
	void OR_L(){ or_d8(&L); }
	
	void OR_aHL();

	// CP A[B|C|D|E|H|L]

	void CP_d8(){ cp_d8(&d8); }
	void CP_A(){ cp_d8(&A); }
	void CP_B(){ cp_d8(&B); }
	void CP_C(){ cp_d8(&C); }
	void CP_D(){ cp_d8(&D); }
	void CP_E(){ cp_d8(&E); }
	void CP_H(){ cp_d8(&H); }
	void CP_L(){ cp_d8(&L); }
	
	void CP_aHL();

	// PUSH BC[DE|HL|AF]

	void PUSH_BC(){ push_d16(&B, &C); }
	void PUSH_DE(){ push_d16(&D, &E); }
	void PUSH_HL(){ push_d16(&H, &L); }
	void PUSH_AF(){ push_d16(&A, &F); }

	// POP BC[DE|HL|AF]

	void POP_BC(){ pop_d16(&B, &C); }
	void POP_DE(){ pop_d16(&D, &E); }
	void POP_HL(){ pop_d16(&H, &L); }
	void POP_AF(){ pop_d16(&A, &F); }

	// JP 

	void JP_d16(){ jp_d16(&d16h, &d16l); }
	void JP_NZ_d16(){ if((F & FLAG_Z_MASK) == 0) jp_cc_d16(&d16h, &d16l); }
	void JP_Z_d16(){ if((F & FLAG_Z_MASK) != 0) jp_cc_d16(&d16h, &d16l); }
	void JP_NC_d16(){ if((F & FLAG_C_MASK) == 0) jp_cc_d16(&d16h, &d16l); }
	void JP_C_d16(){ if((F & FLAG_C_MASK) != 0) jp_cc_d16(&d16h, &d16l); }

	void JP_aHL();

	// CALL

	void CALL_a16(){ call_a16(&d16h, &d16l); }
	void CALL_NZ_a16(){ if((F & FLAG_Z_MASK) == 0) call_cc_a16(&d16h, &d16l); }
	void CALL_Z_a16(){ if((F & FLAG_Z_MASK) != 0) call_cc_a16(&d16h, &d16l); }
	void CALL_NC_a16(){ if((F & FLAG_C_MASK) == 0) call_cc_a16(&d16h, &d16l); }
	void CALL_C_a16(){ if((F & FLAG_C_MASK) != 0) call_cc_a16(&d16h, &d16l); }

	// RST 00[08|10|18|20|28|30|38]H

	void RST_00H(){ rst_n(0x00); }
	void RST_08H(){ rst_n(0x08); }
	void RST_10H(){ rst_n(0x10); }
	void RST_18H(){ rst_n(0x18); }
	void RST_20H(){ rst_n(0x20); }
	void RST_28H(){ rst_n(0x28); }
	void RST_30H(){ rst_n(0x30); }
	void RST_38H(){ rst_n(0x38); }

	// RET NZ[Z|NC|C]

	void RET(){ ret(); }
	void RET_NZ(){ if((F & FLAG_Z_MASK) == 0) ret_cc(); }
	void RET_Z(){ if((F & FLAG_Z_MASK) != 0) ret_cc(); }
	void RET_NC(){ if((F & FLAG_C_MASK) == 0) ret_cc(); }
	void RET_C(){ if((F & FLAG_C_MASK) != 0) ret_cc(); }
	void RETI();

	// DI

	void DI();

	// EI

	void EI();

	/////////////////////////////////////////////////////////////////////
	//
	/////////////////////////////////////////////////////////////////////

	// RLC A[B|C|D|E|H|L]

	void RLC_A(){ rlc_d8(&A); }
	void RLC_B(){ rlc_d8(&B); }
	void RLC_C(){ rlc_d8(&C); }
	void RLC_D(){ rlc_d8(&D); }
	void RLC_E(){ rlc_d8(&E); }
	void RLC_H(){ rlc_d8(&H); }
	void RLC_L(){ rlc_d8(&L); }
	void RLC_aHL();

	// RRC A[B|C|D|E|H|L]

	void RRC_A(){ rrc_d8(&A); }
	void RRC_B(){ rrc_d8(&B); }
	void RRC_C(){ rrc_d8(&C); }
	void RRC_D(){ rrc_d8(&D); }
	void RRC_E(){ rrc_d8(&E); }
	void RRC_H(){ rrc_d8(&H); }
	void RRC_L(){ rrc_d8(&L); }
	void RRC_aHL();

	// RL A[B|C|D|E|H|L]

	void RL_A(){ rl_d8(&A); }
	void RL_B(){ rl_d8(&B); }
	void RL_C(){ rl_d8(&C); }
	void RL_D(){ rl_d8(&D); }
	void RL_E(){ rl_d8(&E); }
	void RL_H(){ rl_d8(&H); }
	void RL_L(){ rl_d8(&L); }
	void RL_aHL();

	// RR A[B|C|D|E|H|L]

	void RR_A(){ rr_d8(&A); }
	void RR_B(){ rr_d8(&B); }
	void RR_C(){ rr_d8(&C); }
	void RR_D(){ rr_d8(&D); }
	void RR_E(){ rr_d8(&E); }
	void RR_H(){ rr_d8(&H); }
	void RR_L(){ rr_d8(&L); }
	void RR_aHL();

	// SLA A[B|C|D|E|H|L]

	void SLA_A(){ sla_d8(&A); }
	void SLA_B(){ sla_d8(&B); }
	void SLA_C(){ sla_d8(&C); }
	void SLA_D(){ sla_d8(&D); }
	void SLA_E(){ sla_d8(&E); }
	void SLA_H(){ sla_d8(&H); }
	void SLA_L(){ sla_d8(&L); }
	void SLA_aHL();

	// SRA A[B|C|D|E|H|L]

	void SRA_A(){ sra_d8(&A); }
	void SRA_B(){ sra_d8(&B); }
	void SRA_C(){ sra_d8(&C); }
	void SRA_D(){ sra_d8(&D); }
	void SRA_E(){ sra_d8(&E); }
	void SRA_H(){ sra_d8(&H); }
	void SRA_L(){ sra_d8(&L); }
	void SRA_aHL();

	// SWAP A[B|C|D|E|H|L]

	void SWAP_A(){ swap_d8(&A); }
	void SWAP_B(){ swap_d8(&B); }
	void SWAP_C(){ swap_d8(&C); }
	void SWAP_D(){ swap_d8(&D); }
	void SWAP_E(){ swap_d8(&E); }
	void SWAP_H(){ swap_d8(&H); }
	void SWAP_L(){ swap_d8(&L); }
	void SWAP_aHL();

	// SRL A[B|C|D|E|H|L]

	void SRL_A(){ srl_d8(&A); }
	void SRL_B(){ srl_d8(&B); }
	void SRL_C(){ srl_d8(&C); }
	void SRL_D(){ srl_d8(&D); }
	void SRL_E(){ srl_d8(&E); }
	void SRL_H(){ srl_d8(&H); }
	void SRL_L(){ srl_d8(&L); }
	void SRL_aHL();

	// BIT 0[1|2|3|4|5|6|7],A[B|C|D|E|H|L]

	void BIT_0_A(){ bit_d8(&A, 0); }
	void BIT_0_B(){ bit_d8(&B, 0); }
	void BIT_0_C(){ bit_d8(&C, 0); }
	void BIT_0_D(){ bit_d8(&D, 0); }
	void BIT_0_E(){ bit_d8(&E, 0); }
	void BIT_0_H(){ bit_d8(&H, 0); }
	void BIT_0_L(){ bit_d8(&L, 0); }
	void BIT_0_aHL();

	void BIT_1_A(){ bit_d8(&A, 1); }
	void BIT_1_B(){ bit_d8(&B, 1); }
	void BIT_1_C(){ bit_d8(&C, 1); }
	void BIT_1_D(){ bit_d8(&D, 1); }
	void BIT_1_E(){ bit_d8(&E, 1); }
	void BIT_1_H(){ bit_d8(&H, 1); }
	void BIT_1_L(){ bit_d8(&L, 1); }
	void BIT_1_aHL();

	void BIT_2_A(){ bit_d8(&A, 2); }
	void BIT_2_B(){ bit_d8(&B, 2); }
	void BIT_2_C(){ bit_d8(&C, 2); }
	void BIT_2_D(){ bit_d8(&D, 2); }
	void BIT_2_E(){ bit_d8(&E, 2); }
	void BIT_2_H(){ bit_d8(&H, 2); }
	void BIT_2_L(){ bit_d8(&L, 2); }
	void BIT_2_aHL();

	void BIT_3_A(){ bit_d8(&A, 3); }
	void BIT_3_B(){ bit_d8(&B, 3); }
	void BIT_3_C(){ bit_d8(&C, 3); }
	void BIT_3_D(){ bit_d8(&D, 3); }
	void BIT_3_E(){ bit_d8(&E, 3); }
	void BIT_3_H(){ bit_d8(&H, 3); }
	void BIT_3_L(){ bit_d8(&L, 3); }
	void BIT_3_aHL();

	void BIT_4_A(){ bit_d8(&A, 4); }
	void BIT_4_B(){ bit_d8(&B, 4); }
	void BIT_4_C(){ bit_d8(&C, 4); }
	void BIT_4_D(){ bit_d8(&D, 4); }
	void BIT_4_E(){ bit_d8(&E, 4); }
	void BIT_4_H(){ bit_d8(&H, 4); }
	void BIT_4_L(){ bit_d8(&L, 4); }
	void BIT_4_aHL();

	void BIT_5_A(){ bit_d8(&A, 5); }
	void BIT_5_B(){ bit_d8(&B, 5); }
	void BIT_5_C(){ bit_d8(&C, 5); }
	void BIT_5_D(){ bit_d8(&D, 5); }
	void BIT_5_E(){ bit_d8(&E, 5); }
	void BIT_5_H(){ bit_d8(&H, 5); }
	void BIT_5_L(){ bit_d8(&L, 5); }
	void BIT_5_aHL();

	void BIT_6_A(){ bit_d8(&A, 6); }
	void BIT_6_B(){ bit_d8(&B, 6); }
	void BIT_6_C(){ bit_d8(&C, 6); }
	void BIT_6_D(){ bit_d8(&D, 6); }
	void BIT_6_E(){ bit_d8(&E, 6); }
	void BIT_6_H(){ bit_d8(&H, 6); }
	void BIT_6_L(){ bit_d8(&L, 6); }
	void BIT_6_aHL();

	void BIT_7_A(){ bit_d8(&A, 7); }
	void BIT_7_B(){ bit_d8(&B, 7); }
	void BIT_7_C(){ bit_d8(&C, 7); }
	void BIT_7_D(){ bit_d8(&D, 7); }
	void BIT_7_E(){ bit_d8(&E, 7); }
	void BIT_7_H(){ bit_d8(&H, 7); }
	void BIT_7_L(){ bit_d8(&L, 7); }
	void BIT_7_aHL();

	// RES 0[1|2|3|4|5|6|7],A[B|C|D|E|H|L]

	void RES_0_A(){ res_d8(&A, 0); }
	void RES_0_B(){ res_d8(&B, 0); }
	void RES_0_C(){ res_d8(&C, 0); }
	void RES_0_D(){ res_d8(&D, 0); }
	void RES_0_E(){ res_d8(&E, 0); }
	void RES_0_H(){ res_d8(&H, 0); }
	void RES_0_L(){ res_d8(&L, 0); }
	void RES_0_aHL();

	void RES_1_A(){ res_d8(&A, 1); }
	void RES_1_B(){ res_d8(&B, 1); }
	void RES_1_C(){ res_d8(&C, 1); }
	void RES_1_D(){ res_d8(&D, 1); }
	void RES_1_E(){ res_d8(&E, 1); }
	void RES_1_H(){ res_d8(&H, 1); }
	void RES_1_L(){ res_d8(&L, 1); }
	void RES_1_aHL();

	void RES_2_A(){ res_d8(&A, 2); }
	void RES_2_B(){ res_d8(&B, 2); }
	void RES_2_C(){ res_d8(&C, 2); }
	void RES_2_D(){ res_d8(&D, 2); }
	void RES_2_E(){ res_d8(&E, 2); }
	void RES_2_H(){ res_d8(&H, 2); }
	void RES_2_L(){ res_d8(&L, 2); }
	void RES_2_aHL();

	void RES_3_A(){ res_d8(&A, 3); }
	void RES_3_B(){ res_d8(&B, 3); }
	void RES_3_C(){ res_d8(&C, 3); }
	void RES_3_D(){ res_d8(&D, 3); }
	void RES_3_E(){ res_d8(&E, 3); }
	void RES_3_H(){ res_d8(&H, 3); }
	void RES_3_L(){ res_d8(&L, 3); }
	void RES_3_aHL();

	void RES_4_A(){ res_d8(&A, 4); }
	void RES_4_B(){ res_d8(&B, 4); }
	void RES_4_C(){ res_d8(&C, 4); }
	void RES_4_D(){ res_d8(&D, 4); }
	void RES_4_E(){ res_d8(&E, 4); }
	void RES_4_H(){ res_d8(&H, 4); }
	void RES_4_L(){ res_d8(&L, 4); }
	void RES_4_aHL();

	void RES_5_A(){ res_d8(&A, 5); }
	void RES_5_B(){ res_d8(&B, 5); }
	void RES_5_C(){ res_d8(&C, 5); }
	void RES_5_D(){ res_d8(&D, 5); }
	void RES_5_E(){ res_d8(&E, 5); }
	void RES_5_H(){ res_d8(&H, 5); }
	void RES_5_L(){ res_d8(&L, 5); }
	void RES_5_aHL();

	void RES_6_A(){ res_d8(&A, 6); }
	void RES_6_B(){ res_d8(&B, 6); }
	void RES_6_C(){ res_d8(&C, 6); }
	void RES_6_D(){ res_d8(&D, 6); }
	void RES_6_E(){ res_d8(&E, 6); }
	void RES_6_H(){ res_d8(&H, 6); }
	void RES_6_L(){ res_d8(&L, 6); }
	void RES_6_aHL();

	void RES_7_A(){ res_d8(&A, 7); }
	void RES_7_B(){ res_d8(&B, 7); }
	void RES_7_C(){ res_d8(&C, 7); }
	void RES_7_D(){ res_d8(&D, 7); }
	void RES_7_E(){ res_d8(&E, 7); }
	void RES_7_H(){ res_d8(&H, 7); }
	void RES_7_L(){ res_d8(&L, 7); }
	void RES_7_aHL();

	// SET 0[1|2|3|4|5|6|7],A[B|C|D|E|H|L]

	void SET_0_A(){ set_d8(&A, 0); }
	void SET_0_B(){ set_d8(&B, 0); }
	void SET_0_C(){ set_d8(&C, 0); }
	void SET_0_D(){ set_d8(&D, 0); }
	void SET_0_E(){ set_d8(&E, 0); }
	void SET_0_H(){ set_d8(&H, 0); }
	void SET_0_L(){ set_d8(&L, 0); }
	void SET_0_aHL();

	void SET_1_A(){ set_d8(&A, 1); }
	void SET_1_B(){ set_d8(&B, 1); }
	void SET_1_C(){ set_d8(&C, 1); }
	void SET_1_D(){ set_d8(&D, 1); }
	void SET_1_E(){ set_d8(&E, 1); }
	void SET_1_H(){ set_d8(&H, 1); }
	void SET_1_L(){ set_d8(&L, 1); }
	void SET_1_aHL();

	void SET_2_A(){ set_d8(&A, 2); }
	void SET_2_B(){ set_d8(&B, 2); }
	void SET_2_C(){ set_d8(&C, 2); }
	void SET_2_D(){ set_d8(&D, 2); }
	void SET_2_E(){ set_d8(&E, 2); }
	void SET_2_H(){ set_d8(&H, 2); }
	void SET_2_L(){ set_d8(&L, 2); }
	void SET_2_aHL();

	void SET_3_A(){ set_d8(&A, 3); }
	void SET_3_B(){ set_d8(&B, 3); }
	void SET_3_C(){ set_d8(&C, 3); }
	void SET_3_D(){ set_d8(&D, 3); }
	void SET_3_E(){ set_d8(&E, 3); }
	void SET_3_H(){ set_d8(&H, 3); }
	void SET_3_L(){ set_d8(&L, 3); }
	void SET_3_aHL();

	void SET_4_A(){ set_d8(&A, 4); }
	void SET_4_B(){ set_d8(&B, 4); }
	void SET_4_C(){ set_d8(&C, 4); }
	void SET_4_D(){ set_d8(&D, 4); }
	void SET_4_E(){ set_d8(&E, 4); }
	void SET_4_H(){ set_d8(&H, 4); }
	void SET_4_L(){ set_d8(&L, 4); }
	void SET_4_aHL();

	void SET_5_A(){ set_d8(&A, 5); }
	void SET_5_B(){ set_d8(&B, 5); }
	void SET_5_C(){ set_d8(&C, 5); }
	void SET_5_D(){ set_d8(&D, 5); }
	void SET_5_E(){ set_d8(&E, 5); }
	void SET_5_H(){ set_d8(&H, 5); }
	void SET_5_L(){ set_d8(&L, 5); }
	void SET_5_aHL();

	void SET_6_A(){ set_d8(&A, 6); }
	void SET_6_B(){ set_d8(&B, 6); }
	void SET_6_C(){ set_d8(&C, 6); }
	void SET_6_D(){ set_d8(&D, 6); }
	void SET_6_E(){ set_d8(&E, 6); }
	void SET_6_H(){ set_d8(&H, 6); }
	void SET_6_L(){ set_d8(&L, 6); }
	void SET_6_aHL();

	void SET_7_A(){ set_d8(&A, 7); }
	void SET_7_B(){ set_d8(&B, 7); }
	void SET_7_C(){ set_d8(&C, 7); }
	void SET_7_D(){ set_d8(&D, 7); }
	void SET_7_E(){ set_d8(&E, 7); }
	void SET_7_H(){ set_d8(&H, 7); }
	void SET_7_L(){ set_d8(&L, 7); }
	void SET_7_aHL();
};

#endif
