/*
** Boi, 2020
** DMG / CPU.hpp
*/

#pragma once

////////////////////////////////////////////////////////////////////////////////

#include "MMU.hpp"
#include "Utils/Assertions.hpp"
#include "Utils/Types.hpp"

#include <functional>
#include <map>
#include <memory>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

namespace DMG
{

class CPU
{
	union Register
	{
		struct {
			u16 word;
		};
		struct {
			u8 lower_byte;
			u8 higher_byte;
		};
	};

	enum RegisterIndex8
	{
		RegisterA = 0,
		RegisterF,
		RegisterB,
		RegisterC,
		RegisterD,
		RegisterE,
		RegisterH,
		RegisterL,
	};

	enum RegisterIndex16
	{
		RegisterAF = 0,
		RegisterBC,
		RegisterDE,
		RegisterHL,
		RegisterSP,
	};

	enum Flags : u8
	{
		Zero      = 0x80,
		Substract = 0x40,
		HalfCarry = 0x20,
		Carry     = 0x10,
	};

	struct Instruction
	{
		u8 op;
		u8 length;
		u8 cycles;
		const char* mnemonic;
		std::function<void()> handler;
	};

public:
	explicit CPU(MMU&);
	void dump() const;
	void execNextInstruction();

	u8 imm8();
	u16 imm16();
	void cpImpl(u8);
	void decImpl(u8&);
	void incImpl(u8&);
	void xorImpl(u8);

private:
	/*
	 * a  = address
	 * C  = condition (is flag (un)set?)
	 * d  = immediate value
	 * dp = dereference immediate value (pointer)
	 * r  = register
	 * rp = dereference register value (pointer)
	 * s  = signed immediate value
	 */

	void ADD_r16_r16(RegisterIndex16, RegisterIndex16);
	void ADD_r8_rp16(RegisterIndex8, RegisterIndex16);
	void CCF();
	void CP_d8();
	void CP_r8(RegisterIndex8);
	void CP_rp16(RegisterIndex16);
	void CPL();
	void DAA();
	void DEC_r16(RegisterIndex16);
	void DEC_r8(RegisterIndex8);
	void DEC_rp16(RegisterIndex16);
	void DI();
	void EI();
	void HALT();
	void INC_r16(RegisterIndex16);
	void INC_r8(RegisterIndex8);
	void INC_rp16(RegisterIndex16);
	void JP_a16();
	void JR_C_s8(Flags);
	void JR_NC_s8(Flags);
	void JR_s8();
	void LD_dp16_r16(RegisterIndex16);
	void LD_r16_d16(RegisterIndex16);
	void LD_r8_d8(RegisterIndex8);
	void LD_r8_r8(RegisterIndex8, RegisterIndex8);
	void LD_r8_rp16(RegisterIndex8, RegisterIndex16);
	void LD_rp16_d8(RegisterIndex16);
	void LD_rp16_r8(RegisterIndex16, RegisterIndex8);
	void LDD_r8_rp16(RegisterIndex8, RegisterIndex16);
	void LDD_rp16_r8(RegisterIndex16, RegisterIndex8);
	void LDH_dp8_r8(RegisterIndex8);
	void LDH_r8_dp8(RegisterIndex8);
	void LDI_r8_rp16(RegisterIndex8, RegisterIndex16);
	void LDI_rp16_r8(RegisterIndex16, RegisterIndex8);
	void NOP() {}
	void RL_r8(RegisterIndex8);
	void RLC_r8(RegisterIndex8);
	void RR_r8(RegisterIndex8);
	void RRC_r8(RegisterIndex8);
	void SCF();
	void STOP();
	void XOR_d8();
	void XOR_r8(RegisterIndex8);
	void XOR_rp16(RegisterIndex16);

	void fillInstructionsMap();

public:
	u8 reg8(RegisterIndex8 r) const
	{
		switch (r) {
			case RegisterA: return m_registers[RegisterAF].lower_byte;
			case RegisterF: return m_registers[RegisterAF].higher_byte;
			case RegisterB: return m_registers[RegisterBC].lower_byte;
			case RegisterC: return m_registers[RegisterBC].higher_byte;
			case RegisterD: return m_registers[RegisterDE].lower_byte;
			case RegisterE: return m_registers[RegisterDE].higher_byte;
			case RegisterH: return m_registers[RegisterHL].lower_byte;
			case RegisterL: return m_registers[RegisterHL].higher_byte;
		}
		ASSERT_NOT_REACHED();
	}

	u8& reg8(RegisterIndex8 r)
	{
		switch (r) {
			case RegisterA: return m_registers[RegisterAF].lower_byte;
			case RegisterF: return m_registers[RegisterAF].higher_byte;
			case RegisterB: return m_registers[RegisterBC].lower_byte;
			case RegisterC: return m_registers[RegisterBC].higher_byte;
			case RegisterD: return m_registers[RegisterDE].lower_byte;
			case RegisterE: return m_registers[RegisterDE].higher_byte;
			case RegisterH: return m_registers[RegisterHL].lower_byte;
			case RegisterL: return m_registers[RegisterHL].higher_byte;
		}
		ASSERT_NOT_REACHED();
	}

	u16 reg16(RegisterIndex16 r) const { return m_registers[r].word; }
	u16& reg16(RegisterIndex16 r) { return m_registers[r].word; }

	u16 af() const { return reg16(RegisterAF); }
	u16 bc() const { return reg16(RegisterBC); }
	u16 de() const { return reg16(RegisterDE); }
	u16 hl() const { return reg16(RegisterHL); }
	u16 sp() const { return reg16(RegisterSP); }
	u16 pc() const { return m_pc; }

	u8 a() const { return reg8(RegisterA); }
	u8 f() const { return reg8(RegisterF); }
	u8 b() const { return reg8(RegisterB); }
	u8 c() const { return reg8(RegisterC); }
	u8 d() const { return reg8(RegisterD); }
	u8 e() const { return reg8(RegisterE); }
	u8 h() const { return reg8(RegisterH); }
	u8 l() const { return reg8(RegisterL); }

	void setAF(u16 value) { reg16(RegisterAF) = value; }
	void setBC(u16 value) { reg16(RegisterBC) = value; }
	void setDE(u16 value) { reg16(RegisterDE) = value; }
	void setHL(u16 value) { reg16(RegisterHL) = value; }
	void setSP(u16 value) { reg16(RegisterSP) = value; }

	void setA(u8 value) { reg8(RegisterA) = value; }
	void setF(u8 value) { reg8(RegisterF) = value; }
	void setB(u8 value) { reg8(RegisterB) = value; }
	void setC(u8 value) { reg8(RegisterC) = value; }
	void setD(u8 value) { reg8(RegisterD) = value; }
	void setE(u8 value) { reg8(RegisterE) = value; }
	void setH(u8 value) { reg8(RegisterH) = value; }
	void setL(u8 value) { reg8(RegisterL) = value; }

	bool zf() const { return f() & Zero; }
	bool nf() const { return f() & Substract; }
	bool hf() const { return f() & HalfCarry; }
	bool cf() const { return f() & Carry; }

	void resetFlags() { reg8(RegisterF) = 0; }

	void setFlags(Flags flags, bool value)
	{
		if (value)
			reg8(RegisterF) |= flags;
		else
			reg8(RegisterF) &= ~flags;
	}

private:
	MMU& m_mmu;
	u32 m_cycles = 0;

	Register m_registers[5];
	u16 m_pc = 0x0100;

	std::map<u8, Instruction> m_instruction_map;
	std::map<u8, Instruction> m_cb_instruction_map;
};

}