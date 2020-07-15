/*
** Boi, 2020
** DMG / Core.hpp
*/

#pragma once

////////////////////////////////////////////////////////////////////////////////

#include "CPU.hpp"
#include "MMU.hpp"
#include "Utils/MappedFile.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace DMG
{

class Core
{
public:
	Core(MappedFile&& rom_file);

	void run();
	void dump() const;

	CPU& cpu() { return m_cpu; }
	MMU& mmu() { return m_mmu; }

private:
	MMU m_mmu;
	CPU m_cpu;

	bool m_running = false;
};

}