/*
** Boi, 2020
** MappedFile.cpp
*/

#include "MappedFile.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////

MappedFile::MappedFile(const std::string& filename)
{
	int fd = ::open(filename.c_str(), O_RDONLY, 0);
	if (fd < 0) {
		perror("open");
		return;
	}

	struct stat st;
	::fstat(fd, &st);
	m_size = st.st_size;
	m_map = ::mmap(NULL, m_size, PROT_READ, MAP_SHARED, fd, 0);
	if (m_map == MAP_FAILED)
		perror("mmap");
	else
		m_mapped = true;

	::close(fd);
}

MappedFile::~MappedFile()
{
	unmap();
}

////////////////////////////////////////////////////////////////////////////////

void MappedFile::unmap()
{
	if (!isMapped())
		return;

	if (::munmap(m_map, m_size) < 0) {
		perror("unmap");
		return;
	}
	m_size = 0;
	m_map = nullptr;
	m_mapped = false;
}