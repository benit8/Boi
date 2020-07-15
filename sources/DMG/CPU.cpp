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

void CPU::execNextInstruction()
{
	u16 op_code = imm8();
	ASSERT(m_instruction_map.count(op_code) == 1, "Unknown instruction " BG_WHITE "%02X" RESET, op_code);

	auto insn = m_instruction_map.at(op_code);
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

void CPU::xorImpl(u8 value)
{
	resetFlags();

	setA(a() ^ value);

	setFlags(Zero, a() == 0);
}

////////////////////////////////////////////////////////////////////////////////

void CPU::CP_d8()
{
	cpImpl(imm8());
}

void CPU::CP_r8(RegisterIndex8 reg)
{
	cpImpl(reg8(reg));
}

void CPU::CP_rp16(RegisterIndex16 reg)
{
	cpImpl(m_mmu.read8(reg16(reg)));
}

void CPU::DEC_r16(RegisterIndex16 reg)
{
	reg16(reg)--;
}

void CPU::DEC_r8(RegisterIndex8 reg)
{
	decImpl(reg8(reg));
}

void CPU::DEC_rp16(RegisterIndex16 reg)
{
	u8 value = m_mmu.read8(reg16(reg));
	decImpl(value);
	m_mmu.write8(reg16(reg), value);
}

void CPU::DI()
{
	printf(BYELLOW "TODO" RESET "\n");
}

void CPU::EI()
{
	printf(BYELLOW "TODO" RESET "\n");
}

void CPU::JP_a16()
{
	m_pc = imm16();
}

void CPU::JR_C_s8(Flags flag)
{
	i8 rel = (i8)imm8();
	if (f() & flag) {
		m_pc += rel;
		m_cycles += 4;
	}
}

void CPU::JR_NC_s8(Flags flag)
{
	i8 rel = (i8)imm8();
	if (!(f() & flag)) {
		m_pc += rel;
		m_cycles += 4;
	}
}

void CPU::JR_s8()
{
	m_pc = m_pc + (i8)imm8();
}

void CPU::LD_r16_d16(RegisterIndex16 reg)
{
	reg16(reg) = imm16();
}

void CPU::LD_r8_d8(RegisterIndex8 reg)
{
	reg8(reg) = imm8();
}

void CPU::LDD_rp16_r8(RegisterIndex16 ptr, RegisterIndex8 reg)
{
	m_mmu.write8(reg16(ptr)--, reg8(reg));
}

void CPU::LDH_dp8_r8(RegisterIndex8 reg)
{
	m_mmu.write8(0xFF00 + imm8(), reg8(reg));
}

void CPU::LDH_r8_dp8(RegisterIndex8 reg)
{
	reg8(reg) = m_mmu.read8(0xFF00 + imm8());
}

void CPU::LDI_rp16_r8(RegisterIndex16 ptr, RegisterIndex8 reg)
{
	m_mmu.write8(reg16(ptr)++, reg8(reg));
}

void CPU::XOR_d8()
{
	xorImpl(imm8());
}

void CPU::XOR_r8(RegisterIndex8 reg)
{
	xorImpl(reg8(reg));
}

void CPU::XOR_rp16(RegisterIndex16 reg)
{
	xorImpl(m_mmu.read16(reg16(reg)));
}

////////////////////////////////////////////////////////////////////////////////

void CPU::fillInstructionsMap()
{
	using std::bind;

	m_instruction_map.emplace(0x00, Instruction { 0x00, 1, 4,  "NOP", bind(&CPU::NOP, this) });
	m_instruction_map.emplace(0x01, Instruction { 0x01, 3, 12, "LD BC,d16", bind(&CPU::LD_r16_d16, this, RegisterBC) });
	m_instruction_map.emplace(0x05, Instruction { 0x05, 1, 4,  "DEC B", bind(&CPU::DEC_r8, this, RegisterB) });
	m_instruction_map.emplace(0x06, Instruction { 0x06, 2, 8,  "LD B,d8", bind(&CPU::LD_r8_d8, this, RegisterB) });
	m_instruction_map.emplace(0x0B, Instruction { 0x0B, 1, 8,  "DEC BC", bind(&CPU::DEC_r16, this, RegisterBC) });
	m_instruction_map.emplace(0x0D, Instruction { 0x0D, 1, 4,  "DEC C", bind(&CPU::DEC_r8, this, RegisterC) });
	m_instruction_map.emplace(0x0E, Instruction { 0x0E, 2, 8,  "LD C,d8", bind(&CPU::LD_r8_d8, this, RegisterC) });
	m_instruction_map.emplace(0x11, Instruction { 0x11, 3, 12, "LD DE,d16", bind(&CPU::LD_r16_d16, this, RegisterDE) });
	m_instruction_map.emplace(0x15, Instruction { 0x15, 1, 4,  "DEC D", bind(&CPU::DEC_r8, this, RegisterD) });
	m_instruction_map.emplace(0x16, Instruction { 0x16, 2, 8,  "LD D,d8", bind(&CPU::LD_r8_d8, this, RegisterD) });
	m_instruction_map.emplace(0x18, Instruction { 0x18, 2, 12, "JR r8", bind(&CPU::JR_s8, this) });
	m_instruction_map.emplace(0x1B, Instruction { 0x1B, 1, 8,  "DEC DE", bind(&CPU::DEC_r16, this, RegisterDE) });
	m_instruction_map.emplace(0x1B, Instruction { 0x1B, 1, 8,  "DEC HL", bind(&CPU::DEC_r16, this, RegisterDE) });
	m_instruction_map.emplace(0x1D, Instruction { 0x1D, 1, 4,  "DEC E", bind(&CPU::DEC_r8, this, RegisterE) });
	m_instruction_map.emplace(0x1E, Instruction { 0x1E, 2, 8,  "LD E,d8", bind(&CPU::LD_r8_d8, this, RegisterE) });
	m_instruction_map.emplace(0x20, Instruction { 0x20, 2, 8,  "JR NZ,r8", bind(&CPU::JR_NC_s8, this, Zero) });
	m_instruction_map.emplace(0x21, Instruction { 0x21, 3, 12, "LD HL,d16", bind(&CPU::LD_r16_d16, this, RegisterHL) });
	m_instruction_map.emplace(0x22, Instruction { 0x22, 1, 8,  "LD (HL+),A", bind(&CPU::LDI_rp16_r8, this, RegisterHL, RegisterA) });
	m_instruction_map.emplace(0x25, Instruction { 0x25, 1, 4,  "DEC H", bind(&CPU::DEC_r8, this, RegisterH) });
	m_instruction_map.emplace(0x26, Instruction { 0x26, 2, 8,  "LD H,d8", bind(&CPU::LD_r8_d8, this, RegisterH) });
	m_instruction_map.emplace(0x28, Instruction { 0x28, 2, 8,  "JR Z,r8", bind(&CPU::JR_C_s8, this, Zero) });
	m_instruction_map.emplace(0x2B, Instruction { 0x2B, 1, 8,  "DEC HL", bind(&CPU::DEC_r16, this, RegisterHL) });
	m_instruction_map.emplace(0x2D, Instruction { 0x2D, 1, 4,  "DEC L", bind(&CPU::DEC_r8, this, RegisterL) });
	m_instruction_map.emplace(0x2E, Instruction { 0x2E, 2, 8,  "LD L,d8", bind(&CPU::LD_r8_d8, this, RegisterL) });
	m_instruction_map.emplace(0x30, Instruction { 0x30, 2, 8,  "JR NC,r8", bind(&CPU::JR_NC_s8, this, Carry) });
	m_instruction_map.emplace(0x31, Instruction { 0x31, 3, 12, "LD SP,d16", bind(&CPU::LD_r16_d16, this, RegisterSP) });
	m_instruction_map.emplace(0x32, Instruction { 0x32, 1, 8,  "LD (HL-),A", bind(&CPU::LDD_rp16_r8, this, RegisterHL, RegisterA) });
	m_instruction_map.emplace(0x35, Instruction { 0x35, 1, 12, "DEC (HL)", bind(&CPU::DEC_rp16, this, RegisterHL) });
	m_instruction_map.emplace(0x38, Instruction { 0x38, 2, 8,  "JR C,r8", bind(&CPU::JR_C_s8, this, Carry) });
	m_instruction_map.emplace(0x3B, Instruction { 0x3B, 1, 8,  "DEC SP", bind(&CPU::DEC_r16, this, RegisterSP) });
	m_instruction_map.emplace(0x3D, Instruction { 0x3D, 1, 4,  "DEC A", bind(&CPU::DEC_r8, this, RegisterA) });
	m_instruction_map.emplace(0x3E, Instruction { 0x3E, 2, 8,  "LD A,d8", bind(&CPU::LD_r8_d8, this, RegisterA) });
	m_instruction_map.emplace(0xA8, Instruction { 0xA8, 1, 4,  "XOR B", bind(&CPU::XOR_r8, this, RegisterB) });
	m_instruction_map.emplace(0xA9, Instruction { 0xA9, 1, 4,  "XOR C", bind(&CPU::XOR_r8, this, RegisterC) });
	m_instruction_map.emplace(0xAA, Instruction { 0xAA, 1, 4,  "XOR D", bind(&CPU::XOR_r8, this, RegisterD) });
	m_instruction_map.emplace(0xAB, Instruction { 0xAB, 1, 4,  "XOR E", bind(&CPU::XOR_r8, this, RegisterE) });
	m_instruction_map.emplace(0xAC, Instruction { 0xAC, 1, 4,  "XOR H", bind(&CPU::XOR_r8, this, RegisterH) });
	m_instruction_map.emplace(0xAD, Instruction { 0xAD, 1, 4,  "XOR L", bind(&CPU::XOR_r8, this, RegisterL) });
	m_instruction_map.emplace(0xAE, Instruction { 0xAE, 1, 8,  "XOR (HL)", bind(&CPU::XOR_rp16, this, RegisterHL) });
	m_instruction_map.emplace(0xAF, Instruction { 0xAF, 1, 4,  "XOR A", bind(&CPU::XOR_r8, this, RegisterA) });
	m_instruction_map.emplace(0xB8, Instruction { 0xB8, 1, 4,  "CP B", bind(&CPU::CP_r8, this, RegisterB) });
	m_instruction_map.emplace(0xB9, Instruction { 0xB9, 1, 4,  "CP C", bind(&CPU::CP_r8, this, RegisterC) });
	m_instruction_map.emplace(0xBA, Instruction { 0xBA, 1, 4,  "CP D", bind(&CPU::CP_r8, this, RegisterD) });
	m_instruction_map.emplace(0xBB, Instruction { 0xBB, 1, 4,  "CP E", bind(&CPU::CP_r8, this, RegisterE) });
	m_instruction_map.emplace(0xBC, Instruction { 0xBC, 1, 4,  "CP H", bind(&CPU::CP_r8, this, RegisterH) });
	m_instruction_map.emplace(0xBD, Instruction { 0xBD, 1, 4,  "CP L", bind(&CPU::CP_r8, this, RegisterL) });
	m_instruction_map.emplace(0xBE, Instruction { 0xBE, 1, 8,  "CP (HL)", bind(&CPU::CP_rp16, this, RegisterHL) });
	m_instruction_map.emplace(0xBF, Instruction { 0xBF, 1, 4,  "CP A", bind(&CPU::CP_r8, this, RegisterA) });
	m_instruction_map.emplace(0xC3, Instruction { 0xC3, 3, 16, "JP a16", bind(&CPU::JP_a16, this) });
	m_instruction_map.emplace(0xE0, Instruction { 0xE0, 2, 12, "LDH (a8),A", bind(&CPU::LDH_dp8_r8, this, RegisterA) });
	m_instruction_map.emplace(0xEE, Instruction { 0xEE, 2, 8,  "XOR d8", bind(&CPU::XOR_d8, this) });
	m_instruction_map.emplace(0xF0, Instruction { 0xF0, 2, 12, "LDH A,(a8)", bind(&CPU::LDH_r8_dp8, this, RegisterA) });
	m_instruction_map.emplace(0xF3, Instruction { 0xF3, 1, 4,  "DI", bind(&CPU::DI, this) });
	m_instruction_map.emplace(0xFB, Instruction { 0xFB, 1, 4,  "EI", bind(&CPU::EI, this) });
	m_instruction_map.emplace(0xFE, Instruction { 0xFE, 2, 8,  "CP d8", bind(&CPU::CP_d8, this) });
}

////////////////////////////////////////////////////////////////////////////////

}