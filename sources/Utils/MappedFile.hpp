/*
** Boi, 2020
** MappedFile.hpp
*/

#pragma once

////////////////////////////////////////////////////////////////////////////////

#include <string>

////////////////////////////////////////////////////////////////////////////////

class MappedFile
{
public:
	explicit MappedFile(const std::string& filename);
	~MappedFile();

	void unmap();

	void* data() { return m_map; }
	const void* data() const { return m_map; }
	size_t size() const { return m_size; }
	bool isMapped() const { return m_mapped; }

private:
	void *m_map = nullptr;
	size_t m_size = 0;
	bool m_mapped = false;
};