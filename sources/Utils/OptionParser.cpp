/*
** Boi, 2020
** OptionParser.cpp
*/

#include "OptionParser.hpp"

#include <algorithm>
#include <cassert>
#include <getopt.h>

////////////////////////////////////////////////////////////////////////////////

OptionParser::OptionParser()
{
	addOption(m_showHelp, 'h', "help", "Display this message");
}

////////////////////////////////////////////////////////////////////////////////

bool OptionParser::parse(int argc, char **argv, bool exitOnFailure)
{
	auto printHelpAndExit = [this, argv, exitOnFailure] {
		printHelp(std::cerr, argv[0]);
		if (exitOnFailure)
			exit(1);
	};

	opterr = 0;
	optind = 0;

	std::vector<struct option> longopt;
	std::string shortopt;
	for (auto &opt : m_options) {
		if (opt.longName)
			longopt.push_back({opt.longName, opt.requiresArgument ? required_argument : no_argument, nullptr, 0});
		if (opt.shortName) {
			shortopt.push_back(opt.shortName);
			if (opt.requiresArgument)
				shortopt.push_back(':');
		}
	}
	longopt.push_back({ NULL, 0, NULL, 0 });

	while (true) {
		int longIndex = -1;
		int c = getopt_long(argc, argv, shortopt.c_str(), longopt.data(), &longIndex);
		if (c == -1)
			break;
		else if (c == '?') {
			printHelpAndExit();
			return false;
		}

		auto it = m_options.begin();
		if (c == 0)
			it += longIndex;
		else
			it = std::find_if(m_options.begin(), m_options.end(), [c] (auto &o) { return c == o.shortName; });
		assert(it != m_options.end());

		const char *arg = it->requiresArgument ? optarg : nullptr;
		if (!it->acceptor(arg)) {
			std::cerr << "Invalid value for option " << argv[optind - 1] << std::endl;
			printHelpAndExit();
			return false;
		}
	}

	int leftValuesCount = argc - optind;
	int argsValuesCount[m_args.size()];
	int totalRequiredValues = 0;
	for (size_t i = 0; i < m_args.size(); i++) {
		argsValuesCount[i] = m_args[i].minValues;
		totalRequiredValues += m_args[i].minValues;
	}

	if (totalRequiredValues > leftValuesCount) {
		printHelpAndExit();
		return false;
	}
	int extraValuesToDistribute = leftValuesCount - totalRequiredValues;

	for (size_t i = 0; i < m_args.size(); i++) {
		int argValuesCount = std::min(m_args[i].maxValues - m_args[i].minValues, extraValuesToDistribute);
		argsValuesCount[i] += argValuesCount;
		extraValuesToDistribute -= argValuesCount;
		if (extraValuesToDistribute == 0)
			break;
	}

	if (extraValuesToDistribute > 0) {
		printHelpAndExit();
		return false;
	}

	for (size_t i = 0; i < m_args.size(); i++) {
		for (int j = 0; j < argsValuesCount[i]; j++) {
			const char *value = argv[optind++];
			if (!m_args[i].acceptor(value)) {
				std::cerr << "Invalid value for argument " << m_args[i].name << std::endl;
				printHelpAndExit();
				return false;
			}
		}
	}

	if (m_showHelp) {
		printHelp(std::cout, argv[0]);
		exit(0);
	}

	return true;
}

void OptionParser::printHelp(std::ostream &os, const char *programName)
{
	auto prettyOptionName = [] (const Option &opt)
	{
		std::string name("\t");
		if (opt.shortName) {
			name.push_back('-');
			name.push_back(opt.shortName);
			if (opt.longName)
				name.append(", --").append(opt.longName);
		}
		else {
			assert(opt.longName != nullptr);
			name.append("    --").append(opt.longName);
		}
		if (opt.valueName) {
			name.push_back('=');
			name.append(opt.valueName);
		}
		return name;
	};


	os << "Usage:" << std::endl << '\t' << programName;
	for (auto &arg : m_args) {
		bool required = arg.minValues > 0;
		bool repeated = arg.maxValues > 1;

		if (required && repeated)
			os << ' ' << arg.name << "...";
		else if (required && !repeated)
			os << ' ' << arg.name;
		else if (!required && repeated)
			os << " [" << arg.name << "...]";
		else if (!required && !repeated)
			os << " [" << arg.name << ']';
	}

	if (m_args.size())
		os << std::endl << std::endl << "Arguments:" << std::endl;
	for (auto &arg : m_args) {
		std::string name = arg.name;
		if (name.length() < 28)
			name.append(28 - name.length(), ' ');
		os << '\t' << name << ' ' << arg.help << std::endl;
	}

	if (m_options.size())
		os << std::endl << "Options:" << std::endl;
	for (auto &opt : m_options) {
		auto name = prettyOptionName(opt);
		if (name.length() < 30)
			name.append(30 - name.length(), ' ');
		os << name;
		if (name.back() != ' ')
			os << std::endl << "\t                             ";
		os << opt.help << std::endl;
	}
}

////////////////////////////////////////////////////////////////////////////////

void OptionParser::addOption(Option &&option)
{
	m_options.push_back(std::move(option));
}

void OptionParser::addOption(bool &value, char shortName, const char *longName, const char *help)
{
	m_options.push_back({
		false,
		help,
		longName,
		shortName,
		nullptr,
		[&value] (const char *s) {
			assert(s == nullptr);
			value = true;
			return true;
		}
	});
}

void OptionParser::addOption(int &value, char shortName, const char *longName, const char *help, const char *valueName)
{
	m_options.push_back({
		true,
		help,
		longName,
		shortName,
		valueName,
		[&value](const char *s) {
			value = atoi(s);
			return true;
		}
	});
}

void OptionParser::addOption(std::string &value, char shortName, const char *longName, const char *help, const char *valueName)
{
	m_options.push_back({
		true,
		help,
		longName,
		shortName,
		valueName,
		[&value] (const char *s) {
			value = s;
			return true;
		}
	});
}

////////////////////////////////////////////////////////////////////////////////

void OptionParser::addArgument(Argument &&arg)
{
	m_args.push_back(std::move(arg));
}

void OptionParser::addArgument(int &value, const char *help, const char *name, bool required)
{
	m_args.push_back({
		help,
		name,
		required ? 1 : 0,
		1,
		[&value] (const char *s) {
			value = atoi(s);
			return true;
		}
	});
}

void OptionParser::addArgument(std::string &value, const char *help, const char *name, bool required)
{
	m_args.push_back({
		help,
		name,
		required ? 1 : 0,
		1,
		[&value] (const char *s) {
			value = s;
			return true;
		}
	});
}

void OptionParser::addArgument(std::vector<std::string> &values, const char *help, const char *name, bool required)
{
	m_args.push_back({
		help,
		name,
		required ? 1 : 0,
		__INT_MAX__,
		[&values] (const char *s) {
			values.push_back(s);
			return true;
		}
	});
}