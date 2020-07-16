/*
** Boi, 2020
** DMG / CPU.cpp
*/

#include "CPU.hpp"
#include "Utils/Assertions.hpp"
#include "Utils/TermColors.hpp"

#include <cstdio>

////////////////////////////////////////////////////////////////////////////////

namespace DMG
{

////////////////////////////////////////////////////////////////////////////////

CPU::CPU(MMU& mmu)
: m_mmu(mmu)
{
	fillInstructionsMap();

	setAF(0x01B0);
	setBC(0x0013);
	setDE(0x00D8);
	setHL(0x014D);
	setSP(0xFFFE);
}

////////////////////////////////////////////////////////////////////////////////

void CPU::dump() const
{
	printf(
		FAINT "A=%02X F=%02X B=%02X C=%02X D=%02X E=%02X H=%02X L=%02X  PC=%04X SP=%04X  z=%d n=%d h=%d c=%d" RESET "\n",
		a(), f(), b(), c(), d(), e(), h(), l(),  pc(), sp(),  zf(), nf(), hf(), cf()
	);
}

void CPU::execNextInstructionWithMap(std::map<u8, Instruction>& map)
{
	u8 op_code = m_mmu.silent_read8(m_pc++);
	ASSERT_MSG(map.count(op_code) == 1, "Unknown instruction " BG_WHITE "%02X" RESET, op_code);

	auto insn = map.at(op_code);
	printf(MAGENTA "%02X" RESET " :: " BLUE "%s" RESET "\n", insn.op, insn.mnemonic);

	insn.handler();

	m_cycles += insn.cycles;
}


u8 CPU::imm8()
{
	return m_mmu.read8(m_pc++);
}

u16 CPU::imm16()
{
	u16 value = m_mmu.read16(m_pc);
	m_pc += 2;
	return value;
}

void CPU::push8(u8 value)
{
	setSP(sp() - sizeof(value));
	m_mmu.write8(sp(), value);
}

void CPU::push16(u16 value)
{
	setSP(sp() - sizeof(value));
	m_mmu.write16(sp(), value);
}

u8 CPU::pop8()
{
	auto value = m_mmu.read8(sp());
	setSP(sp() + sizeof(value));
	return value;
}

u16 CPU::pop16()
{
	auto value = m_mmu.read16(sp());
	setSP(sp() + sizeof(value));
	return value;
}


void CPU::bitImpl(u8 bit, u8 value)
{
	ASSERT(bit < 8);
	setFlags(Zero, value & (1 << bit));
	setFlags(Substract, false);
	setFlags(HalfCarry, true);
}

void CPU::cpImpl(u8 value)
{
	setFlags(Zero, a() == value);
	setFlags(Substract, true);
	setFlags(HalfCarry, (a() & 0xF) < (value & 0xF));
	setFlags(Carry, a() < value);
}

void CPU::decImpl(u8& value)
{
	setFlags(Substract, true);

	value--;

	setFlags(Zero, value == 0);
	setFlags(HalfCarry, (value & 0xF) == 0xF);
}

void CPU::incImpl(u8& value)
{
	setFlags(Substract, false);

	value++;

	setFlags(Zero, value == 0);
	setFlags(HalfCarry, (value & 0xF) == 0xF);
}

void CPU::resImpl(u8 bit, u8& value)
{
	ASSERT(bit < 8);
	value &= ~(1 << bit);
}

void CPU::setImpl(u8 bit, u8& value)
{
	ASSERT(bit < 8);
	value |= (1 << bit);
}

void CPU::xorImpl(u8 value)
{
	resetFlags();

	setA(a() ^ value);

	setFlags(Zero, a() == 0);
}

////////////////////////////////////////////////////////////////////////////////

void CPU::BIT_r8(u8 bit, RegisterIndex8 reg) { bitImpl(bit, reg8(reg)); }
void CPU::BIT_rp16(u8 bit, RegisterIndex16 ptr) { bitImpl(bit, m_mmu.read8(reg16(ptr))); }

void CPU::CP_u8() { cpImpl(imm8()); }
void CPU::CP_r8(RegisterIndex8 reg) { cpImpl(reg8(reg)); }
void CPU::CP_rp16(RegisterIndex16 reg) { cpImpl(m_mmu.read8(reg16(reg))); }

void CPU::CALL_u16()
{
	u16 location = imm16();
	push16(pc());
	m_pc = location;
}

void CPU::DEC_r8(RegisterIndex8 reg) { decImpl(reg8(reg)); }
void CPU::DEC_r16(RegisterIndex16 reg) { reg16(reg)--; }
void CPU::DEC_rp16(RegisterIndex16 reg)
{
	u8 value = m_mmu.read8(reg16(reg));
	decImpl(value);
	m_mmu.write8(reg16(reg), value);
}

void CPU::INC_r8(RegisterIndex8 reg) { incImpl(reg8(reg)); }
void CPU::INC_r16(RegisterIndex16 reg) { reg16(reg)++; }
void CPU::INC_rp16(RegisterIndex16 reg)
{
	u8 value = m_mmu.read8(reg16(reg));
	incImpl(value);
	m_mmu.write8(reg16(reg), value);
}

void CPU::JP_u16() { m_pc = imm16(); }
void CPU::JP_r16(RegisterIndex16 reg) { m_pc = reg16(reg); }
void CPU::JP_C_u16(Flags flag)
{
	u16 location = imm16();
	if (f() & flag) {
		m_pc = location;
		m_cycles += 4;
	}
}
void CPU::JP_NC_u16(Flags flag)
{
	u16 location = imm16();
	if (!(f() & flag)) {
		m_pc = location;
		m_cycles += 4;
	}
}

void CPU::JR_i8() { m_pc = m_pc + (i8)imm8(); }
void CPU::JR_C_i8(Flags flag)
{
	i8 relative_location = (i8)imm8();
	if (f() & flag) {
		m_pc += relative_location;
		m_cycles += 4;
	}
}
void CPU::JR_NC_i8(Flags flag)
{
	i8 relative_location = (i8)imm8();
	if (!(f() & flag)) {
		m_pc += relative_location;
		m_cycles += 4;
	}
}

void CPU::LD_r8_u8(RegisterIndex8 reg) { reg8(reg) = imm8(); }
void CPU::LD_r8_r8(RegisterIndex8 r1, RegisterIndex8 r2) { reg8(r1) = reg8(r2); }
void CPU::LD_r8_rp16(RegisterIndex8 reg, RegisterIndex16 ptr) { reg8(reg) = m_mmu.read8(reg16(ptr)); }
void CPU::LD_r8_up16(RegisterIndex8 reg) { reg8(reg) = m_mmu.read8(imm16()); }
void CPU::LD_r16_r16(RegisterIndex16 r1, RegisterIndex16 r2) { reg16(r1) = reg16(r2); }
void CPU::LD_r16_r16i8(RegisterIndex16 r1, RegisterIndex16 r2) { reg16(r1) = reg16(r2) + (i8)imm8(); }
void CPU::LD_r16_u16(RegisterIndex16 reg) { reg16(reg) = imm16(); }
void CPU::LD_rp16_r8(RegisterIndex16 ptr, RegisterIndex8 reg) { m_mmu.write8(reg16(ptr), reg8(reg)); }
void CPU::LD_rp16_u8(RegisterIndex16 ptr) { m_mmu.write8(reg16(ptr), imm8()); }
void CPU::LD_up16_r8(RegisterIndex8 reg) { m_mmu.write8(imm16(), reg8(reg)); }
void CPU::LD_up16_r16(RegisterIndex16 reg) { m_mmu.write16(imm16(), reg16(reg)); }

void CPU::LDD_rp16_r8(RegisterIndex16 ptr, RegisterIndex8 reg) { m_mmu.write8(reg16(ptr)--, reg8(reg)); }

void CPU::LDH_up8_r8(RegisterIndex8 reg) { m_mmu.write8(0xFF00 + imm8(), reg8(reg)); }
void CPU::LDH_r8_up8(RegisterIndex8 reg) { reg8(reg) = m_mmu.read8(0xFF00 + imm8()); }

void CPU::LDI_rp16_r8(RegisterIndex16 ptr, RegisterIndex8 reg) { m_mmu.write8(reg16(ptr)++, reg8(reg)); }

void CPU::RES_r8(u8 bit, RegisterIndex8 reg) { resImpl(bit, reg8(reg)); }
void CPU::RES_rp16(u8 bit, RegisterIndex16 ptr)
{
	auto value = m_mmu.read8(reg16(ptr));
	resImpl(bit, value);
	m_mmu.write8(reg16(ptr), value);
}

void CPU::SET_r8(u8 bit, RegisterIndex8 reg) { setImpl(bit, reg8(reg)); }
void CPU::SET_rp16(u8 bit, RegisterIndex16 ptr)
{
	auto value = m_mmu.read8(reg16(ptr));
	setImpl(bit, value);
	m_mmu.write8(reg16(ptr), value);
}

void CPU::XOR_u8() { xorImpl(imm8()); }
void CPU::XOR_r8(RegisterIndex8 reg) { xorImpl(reg8(reg)); }
void CPU::XOR_rp16(RegisterIndex16 reg) { xorImpl(m_mmu.read16(reg16(reg))); }

////////////////////////////////////////////////////////////////////////////////

void CPU::fillInstructionsMap()
{
	using std::bind;

	fillCBInstructionsMap();

	std::vector<Instruction> instructions = {
		{ 0x00, 1, 4,  "NOP",         bind(&CPU::NOP, this) },
		{ 0x01, 3, 12, "LD BC,d16",   bind(&CPU::LD_r16_u16, this, RegisterBC) },
		{ 0x02, 1, 8,  "LD (BC),A",   bind(&CPU::LD_rp16_r8, this, RegisterBC, RegisterA) },
		{ 0x03, 1, 8,  "INC BC",      bind(&CPU::INC_r16, this, RegisterBC) },
		{ 0x04, 1, 4,  "INC B",       bind(&CPU::INC_r8, this, RegisterB) },
		{ 0x05, 1, 4,  "DEC B",       bind(&CPU::DEC_r8, this, RegisterB) },
		{ 0x06, 2, 8,  "LD B,d8",     bind(&CPU::LD_r8_u8, this, RegisterB) },
		{ 0x07, 1, 4,  "RLC A",       bind(&CPU::RLC_r8, this, RegisterA) },
		{ 0x08, 3, 20, "LD (a16),SP", bind(&CPU::LD_up16_r16, this, RegisterSP) },
		{ 0x09, 1, 8,  "ADD HL,BC",   bind(&CPU::ADD_r16_r16, this, RegisterHL, RegisterBC) },
		{ 0x0A, 1, 8,  "LD A,(BC)",   bind(&CPU::LD_r8_rp16, this, RegisterA, RegisterBC) },
		{ 0x0B, 1, 8,  "DEC BC",      bind(&CPU::DEC_r16, this, RegisterBC) },
		{ 0x0C, 1, 4,  "INC C",       bind(&CPU::INC_r8, this, RegisterC) },
		{ 0x0D, 1, 4,  "DEC C",       bind(&CPU::DEC_r8, this, RegisterC) },
		{ 0x0E, 2, 8,  "LD C,d8",     bind(&CPU::LD_r8_u8, this, RegisterC) },
		{ 0x0F, 1, 4,  "RRC A",       bind(&CPU::RRC_r8, this, RegisterA) },
		{ 0x10, 2, 4,  "STOP",        bind(&CPU::STOP, this) },
		{ 0x11, 3, 12, "LD DE,d16",   bind(&CPU::LD_r16_u16, this, RegisterDE) },
		{ 0x12, 1, 8,  "LD (DE),A",   bind(&CPU::LD_rp16_r8, this, RegisterDE, RegisterA) },
		{ 0x13, 1, 8,  "INC DE",      bind(&CPU::INC_r16, this, RegisterDE) },
		{ 0x14, 1, 4,  "INC D",       bind(&CPU::INC_r8, this, RegisterD) },
		{ 0x15, 1, 4,  "DEC D",       bind(&CPU::DEC_r8, this, RegisterD) },
		{ 0x16, 2, 8,  "LD D,d8",     bind(&CPU::LD_r8_u8, this, RegisterD) },
		{ 0x17, 1, 4,  "RL A",        bind(&CPU::RL_r8, this, RegisterA) },
		{ 0x18, 2, 12, "JR r8",       bind(&CPU::JR_i8, this) },
		{ 0x19, 1, 8,  "ADD HL,DE",   bind(&CPU::ADD_r16_r16, this, RegisterHL, RegisterDE) },
		{ 0x1A, 1, 8,  "LD A,(DE)",   bind(&CPU::LD_r8_rp16, this, RegisterA, RegisterDE) },
		{ 0x1B, 1, 8,  "DEC DE",      bind(&CPU::DEC_r16, this, RegisterDE) },
		{ 0x1C, 1, 4,  "INC E",       bind(&CPU::INC_r8, this, RegisterE) },
		{ 0x1D, 1, 4,  "DEC E",       bind(&CPU::DEC_r8, this, RegisterE) },
		{ 0x1E, 2, 8,  "LD E,d8",     bind(&CPU::LD_r8_u8, this, RegisterE) },
		{ 0x1F, 1, 4,  "RR A",        bind(&CPU::RR_r8, this, RegisterA) },
		{ 0x20, 2, 8,  "JR NZ,r8",    bind(&CPU::JR_NC_i8, this, Zero) },
		{ 0x21, 3, 12, "LD HL,d16",   bind(&CPU::LD_r16_u16, this, RegisterHL) },
		{ 0x22, 1, 8,  "LD (HL+),A",  bind(&CPU::LDI_rp16_r8, this, RegisterHL, RegisterA) },
		{ 0x23, 1, 8,  "INC HL",      bind(&CPU::INC_r16, this, RegisterHL) },
		{ 0x24, 1, 4,  "INC H",       bind(&CPU::INC_r8, this, RegisterH) },
		{ 0x25, 1, 4,  "DEC H",       bind(&CPU::DEC_r8, this, RegisterH) },
		{ 0x26, 2, 8,  "LD H,d8",     bind(&CPU::LD_r8_u8, this, RegisterH) },
		{ 0x27, 1, 4,  "DAA",         bind(&CPU::DAA, this) },
		{ 0x28, 2, 8,  "JR Z,r8",     bind(&CPU::JR_C_i8, this, Zero) },
		{ 0x29, 1, 8,  "ADD HL,HL",   bind(&CPU::ADD_r16_r16, this, RegisterHL, RegisterHL) },
		{ 0x2A, 1, 8,  "LD A,(HL+)",  bind(&CPU::LDI_r8_rp16, this, RegisterA, RegisterHL) },
		{ 0x2B, 1, 8,  "DEC HL",      bind(&CPU::DEC_r16, this, RegisterHL) },
		{ 0x2C, 1, 4,  "INC L",       bind(&CPU::INC_r8, this, RegisterL) },
		{ 0x2D, 1, 4,  "DEC L",       bind(&CPU::DEC_r8, this, RegisterL) },
		{ 0x2E, 2, 8,  "LD L,d8",     bind(&CPU::LD_r8_u8, this, RegisterL) },
		{ 0x2F, 1, 4,  "CPL",         bind(&CPU::CPL, this) },
		{ 0x30, 2, 8,  "JR NC,r8",    bind(&CPU::JR_NC_i8, this, Carry) },
		{ 0x31, 3, 12, "LD SP,d16",   bind(&CPU::LD_r16_u16, this, RegisterSP) },
		{ 0x32, 1, 8,  "LD (HL-),A",  bind(&CPU::LDD_rp16_r8, this, RegisterHL, RegisterA) },
		{ 0x33, 1, 8,  "INC SP",      bind(&CPU::INC_r16, this, RegisterSP) },
		{ 0x34, 1, 12, "INC (HL)",    bind(&CPU::INC_rp16, this, RegisterHL) },
		{ 0x35, 1, 12, "DEC (HL)",    bind(&CPU::DEC_rp16, this, RegisterHL) },
		{ 0x36, 2, 12, "LD (HL),d8",  bind(&CPU::LD_rp16_u8, this, RegisterHL) },
		{ 0x37, 1, 4,  "SCF",         bind(&CPU::SCF, this) },
		{ 0x38, 2, 8,  "JR C,r8",     bind(&CPU::JR_C_i8, this, Carry) },
		{ 0x39, 1, 8,  "ADD HL,SP",   bind(&CPU::ADD_r16_r16, this, RegisterHL, RegisterSP) },
		{ 0x3A, 1, 8,  "LD A,(HL-)",  bind(&CPU::LDD_r8_rp16, this, RegisterA, RegisterHL) },
		{ 0x3B, 1, 8,  "DEC SP",      bind(&CPU::DEC_r16, this, RegisterSP) },
		{ 0x3C, 1, 4,  "INC A",       bind(&CPU::INC_r8, this, RegisterA) },
		{ 0x3D, 1, 4,  "DEC A",       bind(&CPU::DEC_r8, this, RegisterA) },
		{ 0x3E, 2, 8,  "LD A,d8",     bind(&CPU::LD_r8_u8, this, RegisterA) },
		{ 0x3F, 1, 4,  "CCF",         bind(&CPU::CCF, this) },
		{ 0x40, 1, 4,  "LD B,B",      bind(&CPU::LD_r8_r8, this, RegisterB, RegisterB) },
		{ 0x41, 1, 4,  "LD B,C",      bind(&CPU::LD_r8_r8, this, RegisterB, RegisterC) },
		{ 0x42, 1, 4,  "LD B,D",      bind(&CPU::LD_r8_r8, this, RegisterB, RegisterD) },
		{ 0x43, 1, 4,  "LD B,E",      bind(&CPU::LD_r8_r8, this, RegisterB, RegisterE) },
		{ 0x44, 1, 4,  "LD B,H",      bind(&CPU::LD_r8_r8, this, RegisterB, RegisterH) },
		{ 0x45, 1, 4,  "LD B,L",      bind(&CPU::LD_r8_r8, this, RegisterB, RegisterL) },
		{ 0x46, 1, 8,  "LD B,(HL)",   bind(&CPU::LD_r8_rp16, this, RegisterB, RegisterHL) },
		{ 0x47, 1, 4,  "LD B,A",      bind(&CPU::LD_r8_r8, this, RegisterB, RegisterA) },
		{ 0x48, 1, 4,  "LD C,B",      bind(&CPU::LD_r8_r8, this, RegisterC, RegisterB) },
		{ 0x49, 1, 4,  "LD C,C",      bind(&CPU::LD_r8_r8, this, RegisterC, RegisterC) },
		{ 0x4A, 1, 4,  "LD C,D",      bind(&CPU::LD_r8_r8, this, RegisterC, RegisterD) },
		{ 0x4B, 1, 4,  "LD C,E",      bind(&CPU::LD_r8_r8, this, RegisterC, RegisterE) },
		{ 0x4C, 1, 4,  "LD C,H",      bind(&CPU::LD_r8_r8, this, RegisterC, RegisterH) },
		{ 0x4D, 1, 4,  "LD C,L",      bind(&CPU::LD_r8_r8, this, RegisterC, RegisterL) },
		{ 0x4E, 1, 8,  "LD C,(HL)",   bind(&CPU::LD_r8_rp16, this, RegisterC, RegisterHL) },
		{ 0x4F, 1, 4,  "LD C,A",      bind(&CPU::LD_r8_r8, this, RegisterC, RegisterA) },
		{ 0x50, 1, 4,  "LD D,B",      bind(&CPU::LD_r8_r8, this, RegisterD, RegisterB) },
		{ 0x51, 1, 4,  "LD D,C",      bind(&CPU::LD_r8_r8, this, RegisterD, RegisterC) },
		{ 0x52, 1, 4,  "LD D,D",      bind(&CPU::LD_r8_r8, this, RegisterD, RegisterD) },
		{ 0x53, 1, 4,  "LD D,E",      bind(&CPU::LD_r8_r8, this, RegisterD, RegisterE) },
		{ 0x54, 1, 4,  "LD D,H",      bind(&CPU::LD_r8_r8, this, RegisterD, RegisterH) },
		{ 0x55, 1, 4,  "LD D,L",      bind(&CPU::LD_r8_r8, this, RegisterD, RegisterL) },
		{ 0x56, 1, 8,  "LD D,(HL)",   bind(&CPU::LD_r8_rp16, this, RegisterD, RegisterHL) },
		{ 0x57, 1, 4,  "LD D,A",      bind(&CPU::LD_r8_r8, this, RegisterD, RegisterA) },
		{ 0x58, 1, 4,  "LD E,B",      bind(&CPU::LD_r8_r8, this, RegisterE, RegisterB) },
		{ 0x59, 1, 4,  "LD E,C",      bind(&CPU::LD_r8_r8, this, RegisterE, RegisterC) },
		{ 0x5A, 1, 4,  "LD E,D",      bind(&CPU::LD_r8_r8, this, RegisterE, RegisterD) },
		{ 0x5B, 1, 4,  "LD E,E",      bind(&CPU::LD_r8_r8, this, RegisterE, RegisterE) },
		{ 0x5C, 1, 4,  "LD E,H",      bind(&CPU::LD_r8_r8, this, RegisterE, RegisterH) },
		{ 0x5D, 1, 4,  "LD E,L",      bind(&CPU::LD_r8_r8, this, RegisterE, RegisterL) },
		{ 0x5E, 1, 8,  "LD E,(HL)",   bind(&CPU::LD_r8_rp16, this, RegisterE, RegisterHL) },
		{ 0x5F, 1, 4,  "LD E,A",      bind(&CPU::LD_r8_r8, this, RegisterE, RegisterA) },
		{ 0x60, 1, 4,  "LD H,B",      bind(&CPU::LD_r8_r8, this, RegisterH, RegisterB) },
		{ 0x61, 1, 4,  "LD H,C",      bind(&CPU::LD_r8_r8, this, RegisterH, RegisterC) },
		{ 0x62, 1, 4,  "LD H,D",      bind(&CPU::LD_r8_r8, this, RegisterH, RegisterD) },
		{ 0x63, 1, 4,  "LD H,E",      bind(&CPU::LD_r8_r8, this, RegisterH, RegisterE) },
		{ 0x64, 1, 4,  "LD H,H",      bind(&CPU::LD_r8_r8, this, RegisterH, RegisterH) },
		{ 0x65, 1, 4,  "LD H,L",      bind(&CPU::LD_r8_r8, this, RegisterH, RegisterL) },
		{ 0x66, 1, 8,  "LD H,(HL)",   bind(&CPU::LD_r8_rp16, this, RegisterH, RegisterHL) },
		{ 0x67, 1, 4,  "LD H,A",      bind(&CPU::LD_r8_r8, this, RegisterH, RegisterA) },
		{ 0x68, 1, 4,  "LD L,B",      bind(&CPU::LD_r8_r8, this, RegisterL, RegisterB) },
		{ 0x69, 1, 4,  "LD L,C",      bind(&CPU::LD_r8_r8, this, RegisterL, RegisterC) },
		{ 0x6A, 1, 4,  "LD L,D",      bind(&CPU::LD_r8_r8, this, RegisterL, RegisterD) },
		{ 0x6B, 1, 4,  "LD L,E",      bind(&CPU::LD_r8_r8, this, RegisterL, RegisterE) },
		{ 0x6C, 1, 4,  "LD L,H",      bind(&CPU::LD_r8_r8, this, RegisterL, RegisterH) },
		{ 0x6D, 1, 4,  "LD L,L",      bind(&CPU::LD_r8_r8, this, RegisterL, RegisterL) },
		{ 0x6E, 1, 8,  "LD L,(HL)",   bind(&CPU::LD_r8_rp16, this, RegisterL, RegisterHL) },
		{ 0x6F, 1, 4,  "LD L,A",      bind(&CPU::LD_r8_r8, this, RegisterL, RegisterA) },
		{ 0x70, 1, 8,  "LD (HL),B",   bind(&CPU::LD_rp16_r8, this, RegisterHL, RegisterB) },
		{ 0x71, 1, 8,  "LD (HL),C",   bind(&CPU::LD_rp16_r8, this, RegisterHL, RegisterC) },
		{ 0x72, 1, 8,  "LD (HL),D",   bind(&CPU::LD_rp16_r8, this, RegisterHL, RegisterD) },
		{ 0x73, 1, 8,  "LD (HL),E",   bind(&CPU::LD_rp16_r8, this, RegisterHL, RegisterE) },
		{ 0x74, 1, 8,  "LD (HL),H",   bind(&CPU::LD_rp16_r8, this, RegisterHL, RegisterH) },
		{ 0x75, 1, 8,  "LD (HL),L",   bind(&CPU::LD_rp16_r8, this, RegisterHL, RegisterL) },
		{ 0x76, 1, 4,  "HALT",        bind(&CPU::HALT, this) },
		{ 0x77, 1, 8,  "LD (HL),A",   bind(&CPU::LD_rp16_r8, this, RegisterHL, RegisterA) },
		{ 0x78, 1, 4,  "LD A,B",      bind(&CPU::LD_r8_r8, this, RegisterA, RegisterB) },
		{ 0x79, 1, 4,  "LD A,C",      bind(&CPU::LD_r8_r8, this, RegisterA, RegisterC) },
		{ 0x7A, 1, 4,  "LD A,D",      bind(&CPU::LD_r8_r8, this, RegisterA, RegisterD) },
		{ 0x7B, 1, 4,  "LD A,E",      bind(&CPU::LD_r8_r8, this, RegisterA, RegisterE) },
		{ 0x7C, 1, 4,  "LD A,H",      bind(&CPU::LD_r8_r8, this, RegisterA, RegisterH) },
		{ 0x7D, 1, 4,  "LD A,L",      bind(&CPU::LD_r8_r8, this, RegisterA, RegisterL) },
		{ 0x7E, 1, 8,  "LD A,(HL)",   bind(&CPU::LD_r8_rp16, this, RegisterA, RegisterHL) },
		{ 0x7F, 1, 4,  "LD A,A",      bind(&CPU::LD_r8_r8, this, RegisterA, RegisterA) },
		{ 0x80, 1, 4,  "ADD A,B",     bind(&CPU::ADD_r8_r8, this, RegisterA, RegisterB) },
		{ 0x81, 1, 4,  "ADD A,C",     bind(&CPU::ADD_r8_r8, this, RegisterA, RegisterC) },
		{ 0x82, 1, 4,  "ADD A,D",     bind(&CPU::ADD_r8_r8, this, RegisterA, RegisterD) },
		{ 0x83, 1, 4,  "ADD A,E",     bind(&CPU::ADD_r8_r8, this, RegisterA, RegisterE) },
		{ 0x84, 1, 4,  "ADD A,H",     bind(&CPU::ADD_r8_r8, this, RegisterA, RegisterH) },
		{ 0x85, 1, 4,  "ADD A,L",     bind(&CPU::ADD_r8_r8, this, RegisterA, RegisterL) },
		{ 0x86, 1, 8,  "ADD A,(HL)",  bind(&CPU::ADD_r8_rp16, this, RegisterA, RegisterHL) },
		{ 0x87, 1, 4,  "ADD A,A",     bind(&CPU::ADD_r8_r8, this, RegisterA, RegisterA) },
		{ 0x88, 1, 4,  "ADC A,B",     bind(&CPU::ADC_r8_r8, this, RegisterA, RegisterB) },
		{ 0x89, 1, 4,  "ADC A,C",     bind(&CPU::ADC_r8_r8, this, RegisterA, RegisterC) },
		{ 0x8A, 1, 4,  "ADC A,D",     bind(&CPU::ADC_r8_r8, this, RegisterA, RegisterD) },
		{ 0x8B, 1, 4,  "ADC A,E",     bind(&CPU::ADC_r8_r8, this, RegisterA, RegisterE) },
		{ 0x8C, 1, 4,  "ADC A,H",     bind(&CPU::ADC_r8_r8, this, RegisterA, RegisterH) },
		{ 0x8D, 1, 4,  "ADC A,L",     bind(&CPU::ADC_r8_r8, this, RegisterA, RegisterL) },
		{ 0x8E, 1, 8,  "ADC A,(HL)",  bind(&CPU::ADC_r8_rp16, this, RegisterA, RegisterHL) },
		{ 0x8F, 1, 4,  "ADC A,A",     bind(&CPU::ADC_r8_r8, this, RegisterA, RegisterA) },
		{ 0x90, 1, 4,  "SUB B",       bind(&CPU::SUB_r8, this, RegisterB) },
		{ 0x91, 1, 4,  "SUB C",       bind(&CPU::SUB_r8, this, RegisterC) },
		{ 0x92, 1, 4,  "SUB D",       bind(&CPU::SUB_r8, this, RegisterD) },
		{ 0x93, 1, 4,  "SUB E",       bind(&CPU::SUB_r8, this, RegisterE) },
		{ 0x94, 1, 4,  "SUB H",       bind(&CPU::SUB_r8, this, RegisterH) },
		{ 0x95, 1, 4,  "SUB L",       bind(&CPU::SUB_r8, this, RegisterL) },
		{ 0x96, 1, 8,  "SUB (HL)",    bind(&CPU::SUB_rp16, this, RegisterHL) },
		{ 0x97, 1, 4,  "SUB A",       bind(&CPU::SUB_r8, this, RegisterA) },
		{ 0x98, 1, 4,  "SBC A,B",     bind(&CPU::SBC_r8_r8, this, RegisterA, RegisterB) },
		{ 0x99, 1, 4,  "SBC A,C",     bind(&CPU::SBC_r8_r8, this, RegisterA, RegisterC) },
		{ 0x9A, 1, 4,  "SBC A,D",     bind(&CPU::SBC_r8_r8, this, RegisterA, RegisterD) },
		{ 0x9B, 1, 4,  "SBC A,E",     bind(&CPU::SBC_r8_r8, this, RegisterA, RegisterE) },
		{ 0x9C, 1, 4,  "SBC A,H",     bind(&CPU::SBC_r8_r8, this, RegisterA, RegisterH) },
		{ 0x9D, 1, 4,  "SBC A,L",     bind(&CPU::SBC_r8_r8, this, RegisterA, RegisterL) },
		{ 0x9E, 1, 8,  "SBC A,(HL)",  bind(&CPU::SBC_r8_rp16, this, RegisterA, RegisterHL) },
		{ 0x9F, 1, 4,  "SBC A,A",     bind(&CPU::SBC_r8_r8, this, RegisterA, RegisterA) },
		{ 0xA0, 1, 4,  "AND B",       bind(&CPU::AND_r8, this, RegisterB) },
		{ 0xA1, 1, 4,  "AND C",       bind(&CPU::AND_r8, this, RegisterC) },
		{ 0xA2, 1, 4,  "AND D",       bind(&CPU::AND_r8, this, RegisterD) },
		{ 0xA3, 1, 4,  "AND E",       bind(&CPU::AND_r8, this, RegisterE) },
		{ 0xA4, 1, 4,  "AND H",       bind(&CPU::AND_r8, this, RegisterH) },
		{ 0xA5, 1, 4,  "AND L",       bind(&CPU::AND_r8, this, RegisterL) },
		{ 0xA6, 1, 8,  "AND (HL)",    bind(&CPU::AND_rp16, this, RegisterHL) },
		{ 0xA7, 1, 4,  "AND A",       bind(&CPU::AND_r8, this, RegisterA) },
		{ 0xA8, 1, 4,  "XOR B",       bind(&CPU::XOR_r8, this, RegisterB) },
		{ 0xA9, 1, 4,  "XOR C",       bind(&CPU::XOR_r8, this, RegisterC) },
		{ 0xAA, 1, 4,  "XOR D",       bind(&CPU::XOR_r8, this, RegisterD) },
		{ 0xAB, 1, 4,  "XOR E",       bind(&CPU::XOR_r8, this, RegisterE) },
		{ 0xAC, 1, 4,  "XOR H",       bind(&CPU::XOR_r8, this, RegisterH) },
		{ 0xAD, 1, 4,  "XOR L",       bind(&CPU::XOR_r8, this, RegisterL) },
		{ 0xAE, 1, 8,  "XOR (HL)",    bind(&CPU::XOR_rp16, this, RegisterHL) },
		{ 0xAF, 1, 4,  "XOR A",       bind(&CPU::XOR_r8, this, RegisterA) },
		{ 0xB0, 1, 4,  "OR B",        bind(&CPU::OR_r8, this, RegisterB) },
		{ 0xB1, 1, 4,  "OR C",        bind(&CPU::OR_r8, this, RegisterC) },
		{ 0xB2, 1, 4,  "OR D",        bind(&CPU::OR_r8, this, RegisterD) },
		{ 0xB3, 1, 4,  "OR E",        bind(&CPU::OR_r8, this, RegisterE) },
		{ 0xB4, 1, 4,  "OR H",        bind(&CPU::OR_r8, this, RegisterH) },
		{ 0xB5, 1, 4,  "OR L",        bind(&CPU::OR_r8, this, RegisterL) },
		{ 0xB6, 1, 8,  "OR (HL)",     bind(&CPU::OR_rp16, this, RegisterHL) },
		{ 0xB7, 1, 4,  "OR A",        bind(&CPU::OR_r8, this, RegisterA) },
		{ 0xB8, 1, 4,  "CP B",        bind(&CPU::CP_r8, this, RegisterB) },
		{ 0xB9, 1, 4,  "CP C",        bind(&CPU::CP_r8, this, RegisterC) },
		{ 0xBA, 1, 4,  "CP D",        bind(&CPU::CP_r8, this, RegisterD) },
		{ 0xBB, 1, 4,  "CP E",        bind(&CPU::CP_r8, this, RegisterE) },
		{ 0xBC, 1, 4,  "CP H",        bind(&CPU::CP_r8, this, RegisterH) },
		{ 0xBD, 1, 4,  "CP L",        bind(&CPU::CP_r8, this, RegisterL) },
		{ 0xBE, 1, 8,  "CP (HL)",     bind(&CPU::CP_rp16, this, RegisterHL) },
		{ 0xBF, 1, 4,  "CP A",        bind(&CPU::CP_r8, this, RegisterA) },
		{ 0xC0, 1, 8,  "RET NZ",      bind(&CPU::RET_NC, this, Zero) },
		{ 0xC1, 1, 12, "POP BC",      bind(&CPU::POP_r16, this, RegisterBC) },
		{ 0xC2, 3, 12, "JP NZ,a16",   bind(&CPU::JP_NC_u16, this, Zero) },
		{ 0xC3, 3, 16, "JP a16",      bind(&CPU::JP_u16, this) },
		{ 0xC4, 3, 12, "CALL NZ,a16", bind(&CPU::CALL_NC_u16, this, Zero) },
		{ 0xC5, 1, 16, "PUSH BC",     bind(&CPU::PUSH_r16, this, RegisterBC) },
		{ 0xC6, 2, 8,  "ADD A,d8",    bind(&CPU::ADD_r8_u8, this, RegisterA) },
		{ 0xC7, 1, 16, "RST 00",      bind(&CPU::RST, this, 0x00) },
		{ 0xC8, 1, 8,  "RET Z",       bind(&CPU::RET_C, this, Zero) },
		{ 0xC9, 1, 16, "RET",         bind(&CPU::RET, this) },
		{ 0xCA, 3, 12, "JP Z,a16",    bind(&CPU::JP_C_u16, this, Zero) },
		{ 0xCB, 1, 4,  "PREFIX CB",   bind(&CPU::execNextInstructionWithMap, this, m_cb_instruction_map) },
		{ 0xCC, 3, 12, "CALL Z,a16",  bind(&CPU::CALL_C_u16, this, Zero) },
		{ 0xCD, 3, 24, "CALL a16",    bind(&CPU::CALL_u16, this) },
		{ 0xCE, 2, 8,  "ADC A,d8",    bind(&CPU::ADC_r8_u8, this, RegisterA) },
		{ 0xCF, 1, 16, "RST 08",      bind(&CPU::RST, this, 0x08) },
		{ 0xD0, 1, 8,  "RET NC",      bind(&CPU::RET_NC, this, Carry) },
		{ 0xD1, 1, 12, "POP DE",      bind(&CPU::POP_r16, this, RegisterDE) },
		{ 0xD2, 3, 12, "JP NC,a16",   bind(&CPU::JP_NC_u16, this, Carry) },
		{ 0xD4, 3, 12, "CALL NC,a16", bind(&CPU::CALL_NC_u16, this, Carry) },
		{ 0xD5, 1, 16, "PUSH DE",     bind(&CPU::PUSH_r16, this, RegisterDE) },
		{ 0xD6, 2, 8,  "SUB d8",      bind(&CPU::SUB_u8, this) },
		{ 0xD7, 1, 16, "RST 10",      bind(&CPU::RST, this, 0x10) },
		{ 0xD8, 1, 8,  "RET C",       bind(&CPU::RET_C, this, Carry) },
		{ 0xD9, 1, 16, "RETI",        bind(&CPU::RETI, this) },
		{ 0xDA, 3, 12, "JP C,a16",    bind(&CPU::JP_C_u16, this, Carry) },
		{ 0xDC, 3, 12, "CALL C,a16",  bind(&CPU::CALL_C_u16, this, Carry) },
		{ 0xDE, 2, 8,  "SBC A,d8",    bind(&CPU::SBC_r8_u8, this, RegisterA) },
		{ 0xDF, 1, 16, "RST 18",      bind(&CPU::RST, this, 0x18) },
		{ 0xE0, 2, 12, "LDH (a8),A",  bind(&CPU::LDH_up8_r8, this, RegisterA) },
		{ 0xE1, 1, 12, "POP HL",      bind(&CPU::POP_r16, this, RegisterHL) },
		{ 0xE2, 2, 8,  "LD (C),A",    bind(&CPU::LDH_rp8_r8, this, RegisterC, RegisterA) },
		{ 0xE5, 1, 16, "PUSH HL",     bind(&CPU::PUSH_r16, this, RegisterHL) },
		{ 0xE6, 2, 8,  "AND d8",      bind(&CPU::AND_u8, this) },
		{ 0xE7, 1, 16, "RST 20",      bind(&CPU::RST, this, 0x20) },
		{ 0xE8, 2, 16, "ADD SP,r8",   bind(&CPU::ADD_r16_i8, this, RegisterHL) },
		{ 0xE9, 1, 4,  "JP (HL)",     bind(&CPU::JP_r16, this, RegisterHL) },
		{ 0xEA, 3, 16, "LD (a16),A",  bind(&CPU::LD_up16_r8, this, RegisterA) },
		{ 0xEE, 2, 8,  "XOR d8",      bind(&CPU::XOR_u8, this) },
		{ 0xEF, 1, 16, "RST 28",      bind(&CPU::RST, this, 0x28) },
		{ 0xF0, 2, 12, "LDH A,(a8)",  bind(&CPU::LDH_r8_up8, this, RegisterA) },
		{ 0xF1, 1, 12, "POP AF",      bind(&CPU::POP_r16, this, RegisterAF) },
		{ 0xF2, 2, 8,  "LD A,(C)",    bind(&CPU::LDH_r8_rp8, this, RegisterA, RegisterC) },
		{ 0xF3, 1, 4,  "DI",          bind(&CPU::DI, this) },
		{ 0xF5, 1, 16, "PUSH AF",     bind(&CPU::PUSH_r16, this, RegisterAF) },
		{ 0xF6, 2, 8,  "OR d8",       bind(&CPU::OR_u8, this) },
		{ 0xF7, 1, 16, "RST 30",      bind(&CPU::RST, this, 0x30) },
		{ 0xF8, 2, 12, "LD HL,SP+r8", bind(&CPU::LD_r16_r16i8, this, RegisterHL, RegisterSP) },
		{ 0xF9, 1, 8,  "LD SP,HL",    bind(&CPU::LD_r16_r16, this, RegisterSP, RegisterHL) },
		{ 0xFA, 3, 16, "LD A,(a16)",  bind(&CPU::LD_r8_up16, this, RegisterA) },
		{ 0xFB, 1, 4,  "EI",          bind(&CPU::EI, this) },
		{ 0xFE, 2, 8,  "CP d8",       bind(&CPU::CP_u8, this) },
		{ 0xFF, 1, 16, "RST 38",      bind(&CPU::RST, this, 0x38) },
	};

	for (auto i : instructions)
		m_instruction_map.emplace(i.op, i);
}

void CPU::fillCBInstructionsMap()
{
	using std::bind;

	std::vector<Instruction> cb_instructions = {
		{ 0x00, 1, 8,  "RLC B",      bind(&CPU::RLC_r8,    this, RegisterB) },
		{ 0x01, 1, 8,  "RLC C",      bind(&CPU::RLC_r8,    this, RegisterC) },
		{ 0x02, 1, 8,  "RLC D",      bind(&CPU::RLC_r8,    this, RegisterD) },
		{ 0x03, 1, 8,  "RLC E",      bind(&CPU::RLC_r8,    this, RegisterE) },
		{ 0x04, 1, 8,  "RLC H",      bind(&CPU::RLC_r8,    this, RegisterH) },
		{ 0x05, 1, 8,  "RLC L",      bind(&CPU::RLC_r8,    this, RegisterL) },
		{ 0x06, 1, 16, "RLC (HL)",   bind(&CPU::RLC_rp16,  this, RegisterHL) },
		{ 0x07, 1, 8,  "RLC A",      bind(&CPU::RLC_r8,    this, RegisterA) },
		{ 0x08, 1, 8,  "RRC B",      bind(&CPU::RRC_r8,    this, RegisterB) },
		{ 0x09, 1, 8,  "RRC C",      bind(&CPU::RRC_r8,    this, RegisterC) },
		{ 0x0A, 1, 8,  "RRC D",      bind(&CPU::RRC_r8,    this, RegisterD) },
		{ 0x0B, 1, 8,  "RRC E",      bind(&CPU::RRC_r8,    this, RegisterE) },
		{ 0x0C, 1, 8,  "RRC H",      bind(&CPU::RRC_r8,    this, RegisterH) },
		{ 0x0D, 1, 8,  "RRC L",      bind(&CPU::RRC_r8,    this, RegisterL) },
		{ 0x0E, 1, 16, "RRC (HL)",   bind(&CPU::RRC_rp16,  this, RegisterHL) },
		{ 0x0F, 1, 8,  "RRC A",      bind(&CPU::RRC_r8,    this, RegisterA) },
		{ 0x10, 1, 8,  "RL B",       bind(&CPU::RL_r8,     this, RegisterB) },
		{ 0x11, 1, 8,  "RL C",       bind(&CPU::RL_r8,     this, RegisterC) },
		{ 0x12, 1, 8,  "RL D",       bind(&CPU::RL_r8,     this, RegisterD) },
		{ 0x13, 1, 8,  "RL E",       bind(&CPU::RL_r8,     this, RegisterE) },
		{ 0x14, 1, 8,  "RL H",       bind(&CPU::RL_r8,     this, RegisterH) },
		{ 0x15, 1, 8,  "RL L",       bind(&CPU::RL_r8,     this, RegisterL) },
		{ 0x16, 1, 16, "RL (HL)",    bind(&CPU::RL_rp16,   this, RegisterHL) },
		{ 0x17, 1, 8,  "RL A",       bind(&CPU::RL_r8,     this, RegisterA) },
		{ 0x18, 1, 8,  "RR B",       bind(&CPU::RR_r8,     this, RegisterB) },
		{ 0x19, 1, 8,  "RR C",       bind(&CPU::RR_r8,     this, RegisterC) },
		{ 0x1A, 1, 8,  "RR D",       bind(&CPU::RR_r8,     this, RegisterD) },
		{ 0x1B, 1, 8,  "RR E",       bind(&CPU::RR_r8,     this, RegisterE) },
		{ 0x1C, 1, 8,  "RR H",       bind(&CPU::RR_r8,     this, RegisterH) },
		{ 0x1D, 1, 8,  "RR L",       bind(&CPU::RR_r8,     this, RegisterL) },
		{ 0x1E, 1, 16, "RR (HL)",    bind(&CPU::RR_rp16,   this, RegisterHL) },
		{ 0x1F, 1, 8,  "RR A",       bind(&CPU::RR_r8,     this, RegisterA) },
		{ 0x20, 1, 8,  "SLA B",      bind(&CPU::SLA_r8,    this, RegisterB) },
		{ 0x21, 1, 8,  "SLA C",      bind(&CPU::SLA_r8,    this, RegisterC) },
		{ 0x22, 1, 8,  "SLA D",      bind(&CPU::SLA_r8,    this, RegisterD) },
		{ 0x23, 1, 8,  "SLA E",      bind(&CPU::SLA_r8,    this, RegisterE) },
		{ 0x24, 1, 8,  "SLA H",      bind(&CPU::SLA_r8,    this, RegisterH) },
		{ 0x25, 1, 8,  "SLA L",      bind(&CPU::SLA_r8,    this, RegisterL) },
		{ 0x26, 1, 16, "SLA (HL)",   bind(&CPU::SLA_rp16,  this, RegisterHL) },
		{ 0x27, 1, 8,  "SLA A",      bind(&CPU::SLA_r8,    this, RegisterA) },
		{ 0x28, 1, 8,  "SRA B",      bind(&CPU::SRA_r8,    this, RegisterB) },
		{ 0x29, 1, 8,  "SRA C",      bind(&CPU::SRA_r8,    this, RegisterC) },
		{ 0x2A, 1, 8,  "SRA D",      bind(&CPU::SRA_r8,    this, RegisterD) },
		{ 0x2B, 1, 8,  "SRA E",      bind(&CPU::SRA_r8,    this, RegisterE) },
		{ 0x2C, 1, 8,  "SRA H",      bind(&CPU::SRA_r8,    this, RegisterH) },
		{ 0x2D, 1, 8,  "SRA L",      bind(&CPU::SRA_r8,    this, RegisterL) },
		{ 0x2E, 1, 16, "SRA (HL)",   bind(&CPU::SRA_rp16,  this, RegisterHL) },
		{ 0x2F, 1, 8,  "SRA A",      bind(&CPU::SRA_r8,    this, RegisterA) },
		{ 0x30, 1, 8,  "SWAP B",     bind(&CPU::SWAP_r8,   this, RegisterB) },
		{ 0x31, 1, 8,  "SWAP C",     bind(&CPU::SWAP_r8,   this, RegisterC) },
		{ 0x32, 1, 8,  "SWAP D",     bind(&CPU::SWAP_r8,   this, RegisterD) },
		{ 0x33, 1, 8,  "SWAP E",     bind(&CPU::SWAP_r8,   this, RegisterE) },
		{ 0x34, 1, 8,  "SWAP H",     bind(&CPU::SWAP_r8,   this, RegisterH) },
		{ 0x35, 1, 8,  "SWAP L",     bind(&CPU::SWAP_r8,   this, RegisterL) },
		{ 0x36, 1, 16, "SWAP (HL)",  bind(&CPU::SWAP_rp16, this, RegisterHL) },
		{ 0x37, 1, 8,  "SWAP A",     bind(&CPU::SWAP_r8,   this, RegisterA) },
		{ 0x38, 1, 8,  "SRL B",      bind(&CPU::SRL_r8,    this, RegisterB) },
		{ 0x39, 1, 8,  "SRL C",      bind(&CPU::SRL_r8,    this, RegisterC) },
		{ 0x3A, 1, 8,  "SRL D",      bind(&CPU::SRL_r8,    this, RegisterD) },
		{ 0x3B, 1, 8,  "SRL E",      bind(&CPU::SRL_r8,    this, RegisterE) },
		{ 0x3C, 1, 8,  "SRL H",      bind(&CPU::SRL_r8,    this, RegisterH) },
		{ 0x3D, 1, 8,  "SRL L",      bind(&CPU::SRL_r8,    this, RegisterL) },
		{ 0x3E, 1, 16, "SRL (HL)",   bind(&CPU::SRL_rp16,  this, RegisterHL) },
		{ 0x3F, 1, 8,  "SRL A",      bind(&CPU::SRL_r8,    this, RegisterA) },
		{ 0x40, 1, 8,  "BIT 0,B",    bind(&CPU::BIT_r8,    this, 0, RegisterB) },
		{ 0x41, 1, 8,  "BIT 0,C",    bind(&CPU::BIT_r8,    this, 0, RegisterC) },
		{ 0x42, 1, 8,  "BIT 0,D",    bind(&CPU::BIT_r8,    this, 0, RegisterD) },
		{ 0x43, 1, 8,  "BIT 0,E",    bind(&CPU::BIT_r8,    this, 0, RegisterE) },
		{ 0x44, 1, 8,  "BIT 0,H",    bind(&CPU::BIT_r8,    this, 0, RegisterH) },
		{ 0x45, 1, 8,  "BIT 0,L",    bind(&CPU::BIT_r8,    this, 0, RegisterL) },
		{ 0x46, 1, 16, "BIT 0,(HL)", bind(&CPU::BIT_rp16,  this, 0, RegisterHL) },
		{ 0x47, 1, 8,  "BIT 0,A",    bind(&CPU::BIT_r8,    this, 0, RegisterA) },
		{ 0x48, 1, 8,  "BIT 1,B",    bind(&CPU::BIT_r8,    this, 1, RegisterB) },
		{ 0x49, 1, 8,  "BIT 1,C",    bind(&CPU::BIT_r8,    this, 1, RegisterC) },
		{ 0x4A, 1, 8,  "BIT 1,D",    bind(&CPU::BIT_r8,    this, 1, RegisterD) },
		{ 0x4B, 1, 8,  "BIT 1,E",    bind(&CPU::BIT_r8,    this, 1, RegisterE) },
		{ 0x4C, 1, 8,  "BIT 1,H",    bind(&CPU::BIT_r8,    this, 1, RegisterH) },
		{ 0x4D, 1, 8,  "BIT 1,L",    bind(&CPU::BIT_r8,    this, 1, RegisterL) },
		{ 0x4E, 1, 16, "BIT 1,(HL)", bind(&CPU::BIT_rp16,  this, 1, RegisterHL) },
		{ 0x4F, 1, 8,  "BIT 1,A",    bind(&CPU::BIT_r8,    this, 1, RegisterA) },
		{ 0x50, 1, 8,  "BIT 2,B",    bind(&CPU::BIT_r8,    this, 2, RegisterB) },
		{ 0x51, 1, 8,  "BIT 2,C",    bind(&CPU::BIT_r8,    this, 2, RegisterC) },
		{ 0x52, 1, 8,  "BIT 2,D",    bind(&CPU::BIT_r8,    this, 2, RegisterD) },
		{ 0x53, 1, 8,  "BIT 2,E",    bind(&CPU::BIT_r8,    this, 2, RegisterE) },
		{ 0x54, 1, 8,  "BIT 2,H",    bind(&CPU::BIT_r8,    this, 2, RegisterH) },
		{ 0x55, 1, 8,  "BIT 2,L",    bind(&CPU::BIT_r8,    this, 2, RegisterL) },
		{ 0x56, 1, 16, "BIT 2,(HL)", bind(&CPU::BIT_rp16,  this, 2, RegisterHL) },
		{ 0x57, 1, 8,  "BIT 2,A",    bind(&CPU::BIT_r8,    this, 2, RegisterA) },
		{ 0x58, 1, 8,  "BIT 3,B",    bind(&CPU::BIT_r8,    this, 3, RegisterB) },
		{ 0x59, 1, 8,  "BIT 3,C",    bind(&CPU::BIT_r8,    this, 3, RegisterC) },
		{ 0x5A, 1, 8,  "BIT 3,D",    bind(&CPU::BIT_r8,    this, 3, RegisterD) },
		{ 0x5B, 1, 8,  "BIT 3,E",    bind(&CPU::BIT_r8,    this, 3, RegisterE) },
		{ 0x5C, 1, 8,  "BIT 3,H",    bind(&CPU::BIT_r8,    this, 3, RegisterH) },
		{ 0x5D, 1, 8,  "BIT 3,L",    bind(&CPU::BIT_r8,    this, 3, RegisterL) },
		{ 0x5E, 1, 16, "BIT 3,(HL)", bind(&CPU::BIT_rp16,  this, 3, RegisterHL) },
		{ 0x5F, 1, 8,  "BIT 3,A",    bind(&CPU::BIT_r8,    this, 3, RegisterA) },
		{ 0x60, 1, 8,  "BIT 4,B",    bind(&CPU::BIT_r8,    this, 4, RegisterB) },
		{ 0x61, 1, 8,  "BIT 4,C",    bind(&CPU::BIT_r8,    this, 4, RegisterC) },
		{ 0x62, 1, 8,  "BIT 4,D",    bind(&CPU::BIT_r8,    this, 4, RegisterD) },
		{ 0x63, 1, 8,  "BIT 4,E",    bind(&CPU::BIT_r8,    this, 4, RegisterE) },
		{ 0x64, 1, 8,  "BIT 4,H",    bind(&CPU::BIT_r8,    this, 4, RegisterH) },
		{ 0x65, 1, 8,  "BIT 4,L",    bind(&CPU::BIT_r8,    this, 4, RegisterL) },
		{ 0x66, 1, 16, "BIT 4,(HL)", bind(&CPU::BIT_rp16,  this, 4, RegisterHL) },
		{ 0x67, 1, 8,  "BIT 4,A",    bind(&CPU::BIT_r8,    this, 4, RegisterA) },
		{ 0x68, 1, 8,  "BIT 5,B",    bind(&CPU::BIT_r8,    this, 5, RegisterB) },
		{ 0x69, 1, 8,  "BIT 5,C",    bind(&CPU::BIT_r8,    this, 5, RegisterC) },
		{ 0x6A, 1, 8,  "BIT 5,D",    bind(&CPU::BIT_r8,    this, 5, RegisterD) },
		{ 0x6B, 1, 8,  "BIT 5,E",    bind(&CPU::BIT_r8,    this, 5, RegisterE) },
		{ 0x6C, 1, 8,  "BIT 5,H",    bind(&CPU::BIT_r8,    this, 5, RegisterH) },
		{ 0x6D, 1, 8,  "BIT 5,L",    bind(&CPU::BIT_r8,    this, 5, RegisterL) },
		{ 0x6E, 1, 16, "BIT 5,(HL)", bind(&CPU::BIT_rp16,  this, 5, RegisterHL) },
		{ 0x6F, 1, 8,  "BIT 5,A",    bind(&CPU::BIT_r8,    this, 5, RegisterA) },
		{ 0x70, 1, 8,  "BIT 6,B",    bind(&CPU::BIT_r8,    this, 6, RegisterB) },
		{ 0x71, 1, 8,  "BIT 6,C",    bind(&CPU::BIT_r8,    this, 6, RegisterC) },
		{ 0x72, 1, 8,  "BIT 6,D",    bind(&CPU::BIT_r8,    this, 6, RegisterD) },
		{ 0x73, 1, 8,  "BIT 6,E",    bind(&CPU::BIT_r8,    this, 6, RegisterE) },
		{ 0x74, 1, 8,  "BIT 6,H",    bind(&CPU::BIT_r8,    this, 6, RegisterH) },
		{ 0x75, 1, 8,  "BIT 6,L",    bind(&CPU::BIT_r8,    this, 6, RegisterL) },
		{ 0x76, 1, 16, "BIT 6,(HL)", bind(&CPU::BIT_rp16,  this, 6, RegisterHL) },
		{ 0x77, 1, 8,  "BIT 6,A",    bind(&CPU::BIT_r8,    this, 6, RegisterA) },
		{ 0x78, 1, 8,  "BIT 7,B",    bind(&CPU::BIT_r8,    this, 7, RegisterB) },
		{ 0x79, 1, 8,  "BIT 7,C",    bind(&CPU::BIT_r8,    this, 7, RegisterC) },
		{ 0x7A, 1, 8,  "BIT 7,D",    bind(&CPU::BIT_r8,    this, 7, RegisterD) },
		{ 0x7B, 1, 8,  "BIT 7,E",    bind(&CPU::BIT_r8,    this, 7, RegisterE) },
		{ 0x7C, 1, 8,  "BIT 7,H",    bind(&CPU::BIT_r8,    this, 7, RegisterH) },
		{ 0x7D, 1, 8,  "BIT 7,L",    bind(&CPU::BIT_r8,    this, 7, RegisterL) },
		{ 0x7E, 1, 16, "BIT 7,(HL)", bind(&CPU::BIT_rp16,  this, 7, RegisterHL) },
		{ 0x7F, 1, 8,  "BIT 7,A",    bind(&CPU::BIT_r8,    this, 7, RegisterA) },
		{ 0x80, 1, 8,  "RES 0,B",    bind(&CPU::RES_r8,    this, 0, RegisterB) },
		{ 0x81, 1, 8,  "RES 0,C",    bind(&CPU::RES_r8,    this, 0, RegisterC) },
		{ 0x82, 1, 8,  "RES 0,D",    bind(&CPU::RES_r8,    this, 0, RegisterD) },
		{ 0x83, 1, 8,  "RES 0,E",    bind(&CPU::RES_r8,    this, 0, RegisterE) },
		{ 0x84, 1, 8,  "RES 0,H",    bind(&CPU::RES_r8,    this, 0, RegisterH) },
		{ 0x85, 1, 8,  "RES 0,L",    bind(&CPU::RES_r8,    this, 0, RegisterL) },
		{ 0x86, 1, 16, "RES 0,(HL)", bind(&CPU::RES_rp16,  this, 0, RegisterHL) },
		{ 0x87, 1, 8,  "RES 0,A",    bind(&CPU::RES_r8,    this, 0, RegisterA) },
		{ 0x88, 1, 8,  "RES 1,B",    bind(&CPU::RES_r8,    this, 1, RegisterB) },
		{ 0x89, 1, 8,  "RES 1,C",    bind(&CPU::RES_r8,    this, 1, RegisterC) },
		{ 0x8A, 1, 8,  "RES 1,D",    bind(&CPU::RES_r8,    this, 1, RegisterD) },
		{ 0x8B, 1, 8,  "RES 1,E",    bind(&CPU::RES_r8,    this, 1, RegisterE) },
		{ 0x8C, 1, 8,  "RES 1,H",    bind(&CPU::RES_r8,    this, 1, RegisterH) },
		{ 0x8D, 1, 8,  "RES 1,L",    bind(&CPU::RES_r8,    this, 1, RegisterL) },
		{ 0x8E, 1, 16, "RES 1,(HL)", bind(&CPU::RES_rp16,  this, 1, RegisterHL) },
		{ 0x8F, 1, 8,  "RES 1,A",    bind(&CPU::RES_r8,    this, 1, RegisterA) },
		{ 0x90, 1, 8,  "RES 2,B",    bind(&CPU::RES_r8,    this, 2, RegisterB) },
		{ 0x91, 1, 8,  "RES 2,C",    bind(&CPU::RES_r8,    this, 2, RegisterC) },
		{ 0x92, 1, 8,  "RES 2,D",    bind(&CPU::RES_r8,    this, 2, RegisterD) },
		{ 0x93, 1, 8,  "RES 2,E",    bind(&CPU::RES_r8,    this, 2, RegisterE) },
		{ 0x94, 1, 8,  "RES 2,H",    bind(&CPU::RES_r8,    this, 2, RegisterH) },
		{ 0x95, 1, 8,  "RES 2,L",    bind(&CPU::RES_r8,    this, 2, RegisterL) },
		{ 0x96, 1, 16, "RES 2,(HL)", bind(&CPU::RES_rp16,  this, 2, RegisterHL) },
		{ 0x97, 1, 8,  "RES 2,A",    bind(&CPU::RES_r8,    this, 2, RegisterA) },
		{ 0x98, 1, 8,  "RES 3,B",    bind(&CPU::RES_r8,    this, 3, RegisterB) },
		{ 0x99, 1, 8,  "RES 3,C",    bind(&CPU::RES_r8,    this, 3, RegisterC) },
		{ 0x9A, 1, 8,  "RES 3,D",    bind(&CPU::RES_r8,    this, 3, RegisterD) },
		{ 0x9B, 1, 8,  "RES 3,E",    bind(&CPU::RES_r8,    this, 3, RegisterE) },
		{ 0x9C, 1, 8,  "RES 3,H",    bind(&CPU::RES_r8,    this, 3, RegisterH) },
		{ 0x9D, 1, 8,  "RES 3,L",    bind(&CPU::RES_r8,    this, 3, RegisterL) },
		{ 0x9E, 1, 16, "RES 3,(HL)", bind(&CPU::RES_rp16,  this, 3, RegisterHL) },
		{ 0x9F, 1, 8,  "RES 3,A",    bind(&CPU::RES_r8,    this, 3, RegisterA) },
		{ 0xA0, 1, 8,  "RES 4,B",    bind(&CPU::RES_r8,    this, 4, RegisterB) },
		{ 0xA1, 1, 8,  "RES 4,C",    bind(&CPU::RES_r8,    this, 4, RegisterC) },
		{ 0xA2, 1, 8,  "RES 4,D",    bind(&CPU::RES_r8,    this, 4, RegisterD) },
		{ 0xA3, 1, 8,  "RES 4,E",    bind(&CPU::RES_r8,    this, 4, RegisterE) },
		{ 0xA4, 1, 8,  "RES 4,H",    bind(&CPU::RES_r8,    this, 4, RegisterH) },
		{ 0xA5, 1, 8,  "RES 4,L",    bind(&CPU::RES_r8,    this, 4, RegisterL) },
		{ 0xA6, 1, 16, "RES 4,(HL)", bind(&CPU::RES_rp16,  this, 4, RegisterHL) },
		{ 0xA7, 1, 8,  "RES 4,A",    bind(&CPU::RES_r8,    this, 4, RegisterA) },
		{ 0xA8, 1, 8,  "RES 5,B",    bind(&CPU::RES_r8,    this, 5, RegisterB) },
		{ 0xA9, 1, 8,  "RES 5,C",    bind(&CPU::RES_r8,    this, 5, RegisterC) },
		{ 0xAA, 1, 8,  "RES 5,D",    bind(&CPU::RES_r8,    this, 5, RegisterD) },
		{ 0xAB, 1, 8,  "RES 5,E",    bind(&CPU::RES_r8,    this, 5, RegisterE) },
		{ 0xAC, 1, 8,  "RES 5,H",    bind(&CPU::RES_r8,    this, 5, RegisterH) },
		{ 0xAD, 1, 8,  "RES 5,L",    bind(&CPU::RES_r8,    this, 5, RegisterL) },
		{ 0xAE, 1, 16, "RES 5,(HL)", bind(&CPU::RES_rp16,  this, 5, RegisterHL) },
		{ 0xAF, 1, 8,  "RES 5,A",    bind(&CPU::RES_r8,    this, 5, RegisterA) },
		{ 0xB0, 1, 8,  "RES 6,B",    bind(&CPU::RES_r8,    this, 6, RegisterB) },
		{ 0xB1, 1, 8,  "RES 6,C",    bind(&CPU::RES_r8,    this, 6, RegisterC) },
		{ 0xB2, 1, 8,  "RES 6,D",    bind(&CPU::RES_r8,    this, 6, RegisterD) },
		{ 0xB3, 1, 8,  "RES 6,E",    bind(&CPU::RES_r8,    this, 6, RegisterE) },
		{ 0xB4, 1, 8,  "RES 6,H",    bind(&CPU::RES_r8,    this, 6, RegisterH) },
		{ 0xB5, 1, 8,  "RES 6,L",    bind(&CPU::RES_r8,    this, 6, RegisterL) },
		{ 0xB6, 1, 16, "RES 6,(HL)", bind(&CPU::RES_rp16,  this, 6, RegisterHL) },
		{ 0xB7, 1, 8,  "RES 6,A",    bind(&CPU::RES_r8,    this, 6, RegisterA) },
		{ 0xB8, 1, 8,  "RES 7,B",    bind(&CPU::RES_r8,    this, 7, RegisterB) },
		{ 0xB9, 1, 8,  "RES 7,C",    bind(&CPU::RES_r8,    this, 7, RegisterC) },
		{ 0xBA, 1, 8,  "RES 7,D",    bind(&CPU::RES_r8,    this, 7, RegisterD) },
		{ 0xBB, 1, 8,  "RES 7,E",    bind(&CPU::RES_r8,    this, 7, RegisterE) },
		{ 0xBC, 1, 8,  "RES 7,H",    bind(&CPU::RES_r8,    this, 7, RegisterH) },
		{ 0xBD, 1, 8,  "RES 7,L",    bind(&CPU::RES_r8,    this, 7, RegisterL) },
		{ 0xBE, 1, 16, "RES 7,(HL)", bind(&CPU::RES_rp16,  this, 7, RegisterHL) },
		{ 0xBF, 1, 8,  "RES 7,A",    bind(&CPU::RES_r8,    this, 7, RegisterA) },
		{ 0xC0, 1, 8,  "SET 0,B",    bind(&CPU::SET_r8,    this, 0, RegisterB) },
		{ 0xC1, 1, 8,  "SET 0,C",    bind(&CPU::SET_r8,    this, 0, RegisterC) },
		{ 0xC2, 1, 8,  "SET 0,D",    bind(&CPU::SET_r8,    this, 0, RegisterD) },
		{ 0xC3, 1, 8,  "SET 0,E",    bind(&CPU::SET_r8,    this, 0, RegisterE) },
		{ 0xC4, 1, 8,  "SET 0,H",    bind(&CPU::SET_r8,    this, 0, RegisterH) },
		{ 0xC5, 1, 8,  "SET 0,L",    bind(&CPU::SET_r8,    this, 0, RegisterL) },
		{ 0xC6, 1, 16, "SET 0,(HL)", bind(&CPU::SET_rp16,  this, 0, RegisterHL) },
		{ 0xC7, 1, 8,  "SET 0,A",    bind(&CPU::SET_r8,    this, 0, RegisterA) },
		{ 0xC8, 1, 8,  "SET 1,B",    bind(&CPU::SET_r8,    this, 1, RegisterB) },
		{ 0xC9, 1, 8,  "SET 1,C",    bind(&CPU::SET_r8,    this, 1, RegisterC) },
		{ 0xCA, 1, 8,  "SET 1,D",    bind(&CPU::SET_r8,    this, 1, RegisterD) },
		{ 0xCB, 1, 8,  "SET 1,E",    bind(&CPU::SET_r8,    this, 1, RegisterE) },
		{ 0xCC, 1, 8,  "SET 1,H",    bind(&CPU::SET_r8,    this, 1, RegisterH) },
		{ 0xCD, 1, 8,  "SET 1,L",    bind(&CPU::SET_r8,    this, 1, RegisterL) },
		{ 0xCE, 1, 16, "SET 1,(HL)", bind(&CPU::SET_rp16,  this, 1, RegisterHL) },
		{ 0xCF, 1, 8,  "SET 1,A",    bind(&CPU::SET_r8,    this, 1, RegisterA) },
		{ 0xD0, 1, 8,  "SET 2,B",    bind(&CPU::SET_r8,    this, 2, RegisterB) },
		{ 0xD1, 1, 8,  "SET 2,C",    bind(&CPU::SET_r8,    this, 2, RegisterC) },
		{ 0xD2, 1, 8,  "SET 2,D",    bind(&CPU::SET_r8,    this, 2, RegisterD) },
		{ 0xD3, 1, 8,  "SET 2,E",    bind(&CPU::SET_r8,    this, 2, RegisterE) },
		{ 0xD4, 1, 8,  "SET 2,H",    bind(&CPU::SET_r8,    this, 2, RegisterH) },
		{ 0xD5, 1, 8,  "SET 2,L",    bind(&CPU::SET_r8,    this, 2, RegisterL) },
		{ 0xD6, 1, 16, "SET 2,(HL)", bind(&CPU::SET_rp16,  this, 2, RegisterHL) },
		{ 0xD7, 1, 8,  "SET 2,A",    bind(&CPU::SET_r8,    this, 2, RegisterA) },
		{ 0xD8, 1, 8,  "SET 3,B",    bind(&CPU::SET_r8,    this, 3, RegisterB) },
		{ 0xD9, 1, 8,  "SET 3,C",    bind(&CPU::SET_r8,    this, 3, RegisterC) },
		{ 0xDA, 1, 8,  "SET 3,D",    bind(&CPU::SET_r8,    this, 3, RegisterD) },
		{ 0xDB, 1, 8,  "SET 3,E",    bind(&CPU::SET_r8,    this, 3, RegisterE) },
		{ 0xDC, 1, 8,  "SET 3,H",    bind(&CPU::SET_r8,    this, 3, RegisterH) },
		{ 0xDD, 1, 8,  "SET 3,L",    bind(&CPU::SET_r8,    this, 3, RegisterL) },
		{ 0xDE, 1, 16, "SET 3,(HL)", bind(&CPU::SET_rp16,  this, 3, RegisterHL) },
		{ 0xDF, 1, 8,  "SET 3,A",    bind(&CPU::SET_r8,    this, 3, RegisterA) },
		{ 0xE0, 1, 8,  "SET 4,B",    bind(&CPU::SET_r8,    this, 4, RegisterB) },
		{ 0xE1, 1, 8,  "SET 4,C",    bind(&CPU::SET_r8,    this, 4, RegisterC) },
		{ 0xE2, 1, 8,  "SET 4,D",    bind(&CPU::SET_r8,    this, 4, RegisterD) },
		{ 0xE3, 1, 8,  "SET 4,E",    bind(&CPU::SET_r8,    this, 4, RegisterE) },
		{ 0xE4, 1, 8,  "SET 4,H",    bind(&CPU::SET_r8,    this, 4, RegisterH) },
		{ 0xE5, 1, 8,  "SET 4,L",    bind(&CPU::SET_r8,    this, 4, RegisterL) },
		{ 0xE6, 1, 16, "SET 4,(HL)", bind(&CPU::SET_rp16,  this, 4, RegisterHL) },
		{ 0xE7, 1, 8,  "SET 4,A",    bind(&CPU::SET_r8,    this, 4, RegisterA) },
		{ 0xE8, 1, 8,  "SET 5,B",    bind(&CPU::SET_r8,    this, 5, RegisterB) },
		{ 0xE9, 1, 8,  "SET 5,C",    bind(&CPU::SET_r8,    this, 5, RegisterC) },
		{ 0xEA, 1, 8,  "SET 5,D",    bind(&CPU::SET_r8,    this, 5, RegisterD) },
		{ 0xEB, 1, 8,  "SET 5,E",    bind(&CPU::SET_r8,    this, 5, RegisterE) },
		{ 0xEC, 1, 8,  "SET 5,H",    bind(&CPU::SET_r8,    this, 5, RegisterH) },
		{ 0xED, 1, 8,  "SET 5,L",    bind(&CPU::SET_r8,    this, 5, RegisterL) },
		{ 0xEE, 1, 16, "SET 5,(HL)", bind(&CPU::SET_rp16,  this, 5, RegisterHL) },
		{ 0xEF, 1, 8,  "SET 5,A",    bind(&CPU::SET_r8,    this, 5, RegisterA) },
		{ 0xF0, 1, 8,  "SET 6,B",    bind(&CPU::SET_r8,    this, 6, RegisterB) },
		{ 0xF1, 1, 8,  "SET 6,C",    bind(&CPU::SET_r8,    this, 6, RegisterC) },
		{ 0xF2, 1, 8,  "SET 6,D",    bind(&CPU::SET_r8,    this, 6, RegisterD) },
		{ 0xF3, 1, 8,  "SET 6,E",    bind(&CPU::SET_r8,    this, 6, RegisterE) },
		{ 0xF4, 1, 8,  "SET 6,H",    bind(&CPU::SET_r8,    this, 6, RegisterH) },
		{ 0xF5, 1, 8,  "SET 6,L",    bind(&CPU::SET_r8,    this, 6, RegisterL) },
		{ 0xF6, 1, 16, "SET 6,(HL)", bind(&CPU::SET_rp16,  this, 6, RegisterHL) },
		{ 0xF7, 1, 8,  "SET 6,A",    bind(&CPU::SET_r8,    this, 6, RegisterA) },
		{ 0xF8, 1, 8,  "SET 7,B",    bind(&CPU::SET_r8,    this, 7, RegisterB) },
		{ 0xF9, 1, 8,  "SET 7,C",    bind(&CPU::SET_r8,    this, 7, RegisterC) },
		{ 0xFA, 1, 8,  "SET 7,D",    bind(&CPU::SET_r8,    this, 7, RegisterD) },
		{ 0xFB, 1, 8,  "SET 7,E",    bind(&CPU::SET_r8,    this, 7, RegisterE) },
		{ 0xFC, 1, 8,  "SET 7,H",    bind(&CPU::SET_r8,    this, 7, RegisterH) },
		{ 0xFD, 1, 8,  "SET 7,L",    bind(&CPU::SET_r8,    this, 7, RegisterL) },
		{ 0xFE, 1, 16, "SET 7,(HL)", bind(&CPU::SET_rp16,  this, 7, RegisterHL) },
		{ 0xFF, 1, 8,  "SET 7,A",    bind(&CPU::SET_r8,    this, 7, RegisterA) },
	};

	for (auto i : cb_instructions)
		m_cb_instruction_map.emplace(i.op, i);
}

////////////////////////////////////////////////////////////////////////////////

}