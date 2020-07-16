/*
** Boi, 2020
** Utils / Assertions.hpp
*/

#pragma once

////////////////////////////////////////////////////////////////////////////////

#include "TermColors.hpp"

#include <cstdio>

////////////////////////////////////////////////////////////////////////////////

#define ASSERT(x) do { \
	if (!(x)) { \
		fprintf(stderr, BG_BLACK BRED "Assertion failed" RESET ": " RED #x RESET " (%s:%d)\n", __FILE__, __LINE__); \
		abort(); \
	} \
} while (0)

#define ASSERT_MSG(x, fmt, ...) do { \
	if (!(x)) { \
		fprintf(stderr, BG_BLACK BRED "Assertion failed" RESET ": " RED fmt RESET " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__); \
		abort(); \
	} \
} while (0)

#define ASSERT_NOT_REACHED() ASSERT_MSG(false, "Unreachable point reached")
#define TODO() ASSERT_MSG(false, "TODO: %s",  __PRETTY_FUNCTION__)