/*
** Boi, 2020
** OptionParser.hpp
*/

#pragma once

////////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <iostream>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

class OptionParser
{
public:
	struct Option
	{
		bool requiresArgument = true;
		const char *help = nullptr;
		const char *longName = nullptr;
		char shortName = 0;
		const char *valueName = nullptr;
		std::function<bool(const char*)> acceptor;
	};

	struct Argument
	{
		const char *help = nullptr;
		const char *name = nullptr;
		int minValues = 0;
		int maxValues = 1;
		std::function<bool(const char*)> acceptor;
	};

public:
	OptionParser();

	bool parse(int argc, char **argv, bool exitOnFailure = true);
	void printHelp(std::ostream &, const char *programName);

	void addOption(Option &&);
	void addOption(bool        &value, char shortName, const char *longName, const char *help);
	void addOption(int         &value, char shortName, const char *longName, const char *help, const char *valueName);
	void addOption(std::string &value, char shortName, const char *longName, const char *help, const char *valueName);

	void addArgument(Argument &&);
	void addArgument(int                      &value, const char *help, const char *name, bool required = true);
	void addArgument(std::string              &value, const char *help, const char *name, bool required = true);
	void addArgument(std::vector<std::string> &value, const char *help, const char *name, bool required = true);

private:
	std::vector<Option> m_options;
	std::vector<Argument> m_args;

	bool m_showHelp = false;
};