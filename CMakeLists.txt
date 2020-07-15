##
## Boi, 2020
## CMakeLists.txt
##

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
	cmake_policy(SET CMP0076 NEW)
endif()

project(
	"Boi"
	VERSION 0.1.0
	LANGUAGES CXX
)

add_executable(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME}
PUBLIC
	cxx_std_20
)

target_compile_options(${PROJECT_NAME}
PUBLIC
	-W -Wall -Wextra
PRIVATE
	-fPIC
	-g3
)

target_include_directories(${PROJECT_NAME}
PUBLIC
	sources
)

target_sources(${PROJECT_NAME}
PUBLIC
	sources/DMG/Core.hpp
	sources/DMG/CPU.hpp
	sources/DMG/MMU.hpp

	sources/Utils/Assertions.hpp
	sources/Utils/MappedFile.hpp
	sources/Utils/OptionParser.hpp
	sources/Utils/TermColors.hpp
	sources/Utils/Types.hpp

PRIVATE
	sources/Main.cpp

	sources/DMG/Core.cpp
	sources/DMG/CPU.cpp
	sources/DMG/MMU.cpp

	sources/Utils/MappedFile.cpp
	sources/Utils/OptionParser.cpp
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
	# SDL2
)