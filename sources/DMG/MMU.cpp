/*
** Boi, 2020
** DMG / MMU.cpp
*/

#include "MMU.hpp"
#include "Utils/Assertions.hpp"

#include <cstring>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////

namespace DMG
{

////////////////////////////////////////////////////////////////////////////////

const u8 MMU::s_logo_header[] = {
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
};

const MMU::Region MMU::s_regions[] = {
	{0x0000, 0x3FFF, "ROM0"},
	{0x4000, 0x7FFF, "ROMX"},
	{0x8000, 0x9FFF, "VRAM"},
	{0xA000, 0xBFFF, "SRAM"},
	{0xC000, 0xCFFF, "WRAM0"},
	{0xD000, 0xDFFF, "WRAMX"},
	{0xE000, 0xFDFF, "ECHO"},
	{0xFE00, 0xFE9F, "OAM"},
	{0xFEA0, 0xFEFF, "UNUSED"},
	{0xFF00, 0xFF7F, "IOREG"},
	{0xFF80, 0xFFFE, "HRAM"},
	{0xFFFF, 0xFFFF, "IEREG"},
};

////////////////////////////////////////////////////////////////////////////////

MMU::MMU(const u8* rom_data, size_t rom_size)
{
	memcpy(m_map, rom_data, std::min(rom_size, 0x8000UL));
}

////////////////////////////////////////////////////////////////////////////////

u8 MMU::read8(u16 address) const
{
	u8 value = m_map[address];
	printf(GREEN "READ " CYAN "[%04X]" RESET " -> " MAGENTA "%02X" RESET " (%s)\n", address, value, findRegion(address).name);
	return value;
}

u8 MMU::silent_read8(u16 address) const
{
	return m_map[address];
}

u16 MMU::read16(u16 address) const
{
	u16 value = m_map[address] | (m_map[address + 1] << 8);
	printf(GREEN "READ " CYAN "[%04X]" RESET " -> " MAGENTA "%04X" RESET " (%s)\n", address, value, findRegion(address).name);
	return value;
}

void MMU::write8(u16 address, u8 value)
{
	printf(YELLOW "WRITE " CYAN "[%04X]" RESET " <- " MAGENTA "%02X" RESET " (%s)\n", address, value, findRegion(address).name);
	m_map[address] = value;
}

void MMU::write16(u16 address, u16 value)
{
	printf(YELLOW "WRITE " CYAN "[%04X]" RESET " <- " MAGENTA "%04X" RESET " (%s)\n", address, value, findRegion(address).name);
	m_map[address] = value & 0xFF;
	m_map[address + 1] = value >> 8;
}

const u8* MMU::slot(u16 address) const
{
	return &m_map[address];
}

u8* MMU::slot(u16 address)
{
	return &m_map[address];
}

bool MMU::testLogoHeader() const
{
	return memcmp(s_logo_header, slot(0x104), sizeof(s_logo_header)) == 0;
}

////////////////////////////////////////////////////////////////////////////////

const MMU::Region& MMU::findRegion(u16 address)
{
	for (size_t i = 0; i < sizeof(s_regions) / sizeof(Region); ++i) {
		if (s_regions[i].begin <= address && address <= s_regions[i].end)
			return s_regions[i];
	}
	ASSERT_NOT_REACHED();
}

////////////////////////////////////////////////////////////////////////////////

}