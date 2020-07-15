/*
** Boi, 2020
** Main entry point
*/

#include "DMG/Core.hpp"
#include "Utils/MappedFile.hpp"
#include "Utils/OptionParser.hpp"

#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	std::string rom_filename;

	OptionParser opt;
	opt.addArgument(rom_filename, "Filename of the ROM to play", "ROM");
	if (!opt.parse(argc, argv))
		return EXIT_FAILURE;

	MappedFile rom_file(rom_filename);
	if (!rom_file.isMapped()) {
		std::cerr << "Unable to map contents of file \"" << rom_filename << '"' << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "ROM size: " << rom_file.size() << std::endl;

	DMG::Core core(std::move(rom_file));
	core.run();

	return EXIT_SUCCESS;
}