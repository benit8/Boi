/*
** Boi, 2020
** MMU.hpp
*/

#pragma once

////////////////////////////////////////////////////////////////////////////////

#include "Utils/Types.hpp"

#include <array>
#include <cstddef>

////////////////////////////////////////////////////////////////////////////////

namespace DMG
{

class MMU
{
public:
	struct Region
	{
		u16 begin, end;
		const char* name;
	};

public:
	MMU(const u8* rom_data, size_t rom_size);

	u8 read8(u16 address) const;
	u16 read16(u16 address) const;
	void write8(u16 address, u8);
	void write16(u16 address, u16);

	const u8* slot(u16 address) const;
	u8* slot(u16 address);

	bool testLogoHeader() const;

	static const Region& findRegion(u16 address);

private:
	u8 m_map[0x10000] { 0 };

	static const u8 s_logo_header[];
	static const Region s_regions[];
};

}