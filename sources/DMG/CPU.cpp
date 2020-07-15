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

void CPU::xorImpl(u8 value)
{
	resetFlags();

	setA(a() ^ value);
	if (a() == 0)
		setFlags(Zero, true);
}

////////////////////////////////////////////////////////////////////////////////

void CPU::JP_a16()
{
	m_pc = imm16();
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
	m_mmu.write16(reg16(ptr)--, reg8(reg));
}

void CPU::LDI_rp16_r8(RegisterIndex16 ptr, RegisterIndex8 reg)
{
	m_mmu.write16(reg16(ptr)++, reg8(reg));
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
	m_instruction_map.emplace(0x00, Instruction { 0x00, 1, 4,  "NOP", std::bind(&CPU::NOP, this) });
	m_instruction_map.emplace(0x01, Instruction { 0x01, 3, 12, "LD BC,d16", std::bind(&CPU::LD_r16_d16, this, RegisterBC) });
	m_instruction_map.emplace(0x06, Instruction { 0x06, 2, 8,  "LD B,d8", std::bind(&CPU::LD_r8_d8, this, RegisterB) });
	m_instruction_map.emplace(0x0E, Instruction { 0x0E, 2, 8,  "LD C,d8", std::bind(&CPU::LD_r8_d8, this, RegisterC) });
	m_instruction_map.emplace(0x11, Instruction { 0x11, 3, 12, "LD DE,d16", std::bind(&CPU::LD_r16_d16, this, RegisterDE) });
	m_instruction_map.emplace(0x16, Instruction { 0x16, 2, 8,  "LD D,d8", std::bind(&CPU::LD_r8_d8, this, RegisterD) });
	m_instruction_map.emplace(0x1E, Instruction { 0x1E, 2, 8,  "LD E,d8", std::bind(&CPU::LD_r8_d8, this, RegisterE) });
	m_instruction_map.emplace(0x21, Instruction { 0x21, 3, 12, "LD HL,d16", std::bind(&CPU::LD_r16_d16, this, RegisterHL) });
	m_instruction_map.emplace(0x22, Instruction { 0x22, 1, 8,  "LD (HL+),A", std::bind(&CPU::LDI_rp16_r8, this, RegisterHL, RegisterA) });
	m_instruction_map.emplace(0x26, Instruction { 0x26, 2, 8,  "LD H,d8", std::bind(&CPU::LD_r8_d8, this, RegisterH) });
	m_instruction_map.emplace(0x2E, Instruction { 0x2E, 2, 8,  "LD L,d8", std::bind(&CPU::LD_r8_d8, this, RegisterL) });
	m_instruction_map.emplace(0x31, Instruction { 0x31, 3, 12, "LD SP,d16", std::bind(&CPU::LD_r16_d16, this, RegisterSP) });
	m_instruction_map.emplace(0x32, Instruction { 0x32, 1, 8,  "LD (HL-),A", std::bind(&CPU::LDD_rp16_r8, this, RegisterHL, RegisterA) });
	m_instruction_map.emplace(0x3E, Instruction { 0x3E, 2, 8,  "LD A,d8", std::bind(&CPU::LD_r8_d8, this, RegisterA) });
	m_instruction_map.emplace(0xA8, Instruction { 0xA8, 1, 4,  "XOR B", std::bind(&CPU::XOR_r8, this, RegisterB) });
	m_instruction_map.emplace(0xA9, Instruction { 0xA9, 1, 4,  "XOR C", std::bind(&CPU::XOR_r8, this, RegisterC) });
	m_instruction_map.emplace(0xAA, Instruction { 0xAA, 1, 4,  "XOR D", std::bind(&CPU::XOR_r8, this, RegisterD) });
	m_instruction_map.emplace(0xAB, Instruction { 0xAB, 1, 4,  "XOR E", std::bind(&CPU::XOR_r8, this, RegisterE) });
	m_instruction_map.emplace(0xAC, Instruction { 0xAC, 1, 4,  "XOR H", std::bind(&CPU::XOR_r8, this, RegisterH) });
	m_instruction_map.emplace(0xAD, Instruction { 0xAD, 1, 4,  "XOR L", std::bind(&CPU::XOR_r8, this, RegisterL) });
	m_instruction_map.emplace(0xAE, Instruction { 0xAE, 1, 8,  "XOR (HL)", std::bind(&CPU::XOR_rp16, this, RegisterHL) });
	m_instruction_map.emplace(0xAF, Instruction { 0xAF, 1, 4,  "XOR A", std::bind(&CPU::XOR_r8, this, RegisterA) });
	m_instruction_map.emplace(0xC3, Instruction { 0xC3, 3, 16, "JP a16", std::bind(&CPU::JP_a16, this) });
	m_instruction_map.emplace(0xEE, Instruction { 0xEE, 2, 8,  "XOR d8", std::bind(&CPU::XOR_d8, this) });
}

////////////////////////////////////////////////////////////////////////////////

}