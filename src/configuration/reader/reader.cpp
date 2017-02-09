
#include "reader.h"

#include <getopt.h>
#include <stack>
#include <functional>

#include "../basis/logging/logging.h"
#include "../configuration/globalconfiguration.h"
#include "tokenizer.h"

using namespace espreso;

static struct option long_options[] = {
		{"configuration",  no_argument, 0, 'c'},
		{"default",  no_argument, 0, 'd'},
		{"help",  no_argument, 0, 'h'},
		{0, 0, 0, 0}
};

static std::string spaces(size_t size) {
	std::stringstream _indent;
	for (size_t i = 0; i < size; i++) {
		_indent << " ";
	}
	return _indent.str();
};

static std::string uppercase(const std::string &str) {
	std::string upper = str;
	for (auto & c: upper) { c = toupper(c); }
	return upper;
};

void Reader::_read(Configuration &configuration, int* argc, char ***argv)
{
	environment->executable = *argv[0];
	int option_index, option;
	std::string options("c:dhvtm");

	std::vector<struct option> opts;
	std::vector<std::pair<std::string, ParameterBase*> > parameters;
	std::vector<std::string> nameless;

	std::function<void(const Configuration &conf, std::vector<std::string> path)>
	recurse = [&] (const Configuration &conf, std::vector<std::string> path) {
		for (auto it = conf.parameters.begin(); it != conf.parameters.end(); ++it) {
			std::string prefix;
			std::for_each(path.begin(), path.end(), [&] (const std::string &p) { prefix += p + "::"; });
			parameters.push_back(std::make_pair(prefix + uppercase(it->first), it->second));
		}
		for (auto it = conf.subconfigurations.begin(); it != conf.subconfigurations.end(); ++it) {
			path.push_back(uppercase(it->first));
			recurse(*it->second, path);
			path.pop_back();
		}
	};
	recurse(configuration, {});

	opts.reserve(parameters.size() + 3);
	for (size_t i = 0; i < parameters.size(); i++) {
		opts.push_back({ parameters[i].first.c_str(), required_argument, 0, 'p' });
	}

	option_index = 0;
	while (long_options[option_index].name != '\0') {
		opts.push_back(long_options[option_index++]);
	}

	// read the rest parameters
	size_t helpVerboseLevel = 0;

	std::string confFile = "espreso.ecf";
	if (StringCompare::caseSensitiveSuffix(std::string(*argv[0]), "espreso")) {
		confFile = "espreso.ecf";
	}
	if (StringCompare::caseSensitiveSuffix(std::string(*argv[0]), "decomposer")) {
		confFile = "decomposer.ecf";
	}

	while ((option = getopt_long(*argc, *argv, "c:dhvtm", opts.data(), &option_index)) != -1) {
		switch (option) {
		case 'p':
			// parameters will be read after configuration file
			break;
		case 'h':
			helpVerboseLevel++;
			break;
		case 'd':
			store(configuration);
			exit(EXIT_SUCCESS);
		case 'c':
			confFile = optarg;
			break;
		}
	}

	if (helpVerboseLevel) {
		std::cout << "PRINT HELP\n";
		exit(0);
	}

	// read nameless parameters
	while (optind < *argc) {
		nameless.push_back(std::string((*argv)[optind++]));
	}

	_read(configuration, confFile, nameless);

	optind = 0;
	while ((option = getopt_long(*argc, *argv, "c:dhvtm", opts.data(), &option_index)) != -1) {
		switch (option) {
		case 'p':
			if (!parameters[option_index].second->set(optarg)) {
				ESINFO(GLOBAL_ERROR) << "Parameter '" << parameters[option_index].first << "' has wrong value '" << optarg << "'";
			}
			break;
		case 'v':
			environment->verbose_level++;
			break;
		case 't':
			environment->testing_level++;
			break;
		case 'm':
			environment->measure_level++;
			break;
		}
	}
}

void Reader::_read(Configuration &configuration, const std::string &file, const std::vector<std::string> &args)
{
	std::vector<std::string> prefix;
	std::vector<std::string> values;
	std::stack<Configuration*> confStack;
	std::stack<Tokenizer*> tokenStack;

	bool correctlyLoaded = true;
	std::map<size_t, std::vector<std::string> > arguments;

	confStack.push(&configuration);
	tokenStack.push(new Tokenizer(file));
	while (tokenStack.size()) {
		switch (tokenStack.top()->next()) {
		case Tokenizer::Token::END:
			delete tokenStack.top();
			tokenStack.pop();
			break;
		case Tokenizer::Token::STRING:
			values.push_back(tokenStack.top()->value());
			break;
		case Tokenizer::Token::LINK:
		{
			std::string value = tokenStack.top()->value();
			if (value.size() > 4 && StringCompare::caseInsensitiveSuffix(value, ".ecf")) {
				tokenStack.push(new Tokenizer(value));
			}
			if (value.size() > 2 && StringCompare::caseInsensitivePreffix("ARG", value)) {
				std::stringstream ss(std::string(value.begin() + 3, value.end()));
				size_t index = args.size();
				ss >> index;
				if (!ss.fail() && ss.eof() && index < args.size()) {
					values.push_back(args[index]);
					std::string parameter;
					std::for_each(prefix.begin(), prefix.end(), [&] (const std::string &s) { parameter += s + "::"; });
					arguments[index].push_back(parameter + values.front());
				} else {
					if (index < args.size()) {
						ESINFO(GLOBAL_ERROR) << "Invalid argument '" << value << "'";
					} else {
						correctlyLoaded = false;
						if (values.size()) {
							std::string parameter;
							std::for_each(prefix.begin(), prefix.end(), [&] (const std::string &s) { parameter += s + "::"; });
							arguments[index].push_back(parameter + values.front());
						} else {
							ESINFO(GLOBAL_ERROR) << "parameter cannot be the [ARG].\n" << tokenStack.top()->lastLines(2);
						}
					}
				}
			}
			if (value.size() > 4 && StringCompare::caseInsensitiveSuffix(value, ".csv")) {
				std::cout << "READ CSV file\n";
			}
			break;
		}
		case Tokenizer::Token::OBJECT_OPEN:
			if (values.size() == 0) {
				ESINFO(GLOBAL_ERROR) << "PARSE ERROR: Opening of an unnamed region is not allowed.\n" << tokenStack.top()->lastLines(2);
			}
			if (values.size() > 1) {
				ESINFO(GLOBAL_ERROR) << "PARSE ERROR: Multiple names for a region are not allowed.\n" << tokenStack.top()->lastLines(2);
			}
			prefix.push_back(values[0]);
			confStack.push(&confStack.top()->operator [](values[0]));
			values.clear();
			break;
		case Tokenizer::Token::OBJECT_CLOSE:
			if (!confStack.size()) {
				ESINFO(GLOBAL_ERROR) << "PARSE ERROR: Unexpected region end.\n" << tokenStack.top()->lastLines(2);
			}
			prefix.pop_back();
			confStack.pop();
			break;
		case Tokenizer::Token::ASSIGN:
			break;
		case Tokenizer::Token::DELIMITER:
			values.push_back(",");
			break;
		case Tokenizer::Token::EXPRESSION_END:
		{
			if (!correctlyLoaded) {
				values.clear();
				break;
			}
			if (values.size() < 2) {
				ESINFO(GLOBAL_ERROR) << "PARSE ERROR: Incorrect assignment format on line " << tokenStack.top()->line() << ". Use 'PARAMETER' 'VALUE';\n" << tokenStack.top()->lastLines(2);
			}
			std::stringstream ss;
			ss << values[1];
			for (size_t i = 2; i < values.size(); i++) {
				ss << " " << values[i];
			}
			if (!confStack.top()->set(values[0], ss.str())) {
				ESINFO(GLOBAL_ERROR) << "PARSE ERROR: Parameter '" << values[0] << "' has wrong value '" << ss.str() << "'";
			}

			values.clear();
			break;
		}
		case Tokenizer::Token::LINE_END:
			if (values.size() > 1) {
				ESINFO(GLOBAL_ERROR) << "PARSE ERROR: Expected ';' at the end of each expression.\n" << tokenStack.top()->lastLines(1);
			}
			break;
		default:
			ESINFO(GLOBAL_ERROR) << "PARSE ERROR: Unknown token in configuration file";
		}
	}
	if (confStack.size() != 1) {
		ESINFO(GLOBAL_ERROR) << "PARSE ERROR: Unexpected EOF before close all regions.";
	}

	if (!correctlyLoaded) {
		std::string error = "Configuration file is not correctly loaded.\nUse ./espreso ";
		size_t i = 0;
		for (auto it = arguments.begin(); it != arguments.end(); ++it) {
			error += "[ARG" + std::to_string(i++) + "] ";
		}
		error += "\nWhere ARGs are the following:\n";
		i = 0;
		for (auto it = arguments.begin(); it != arguments.end(); ++it) {
			error += "ARG" + std::to_string(i++) + " = { ";
			for (size_t j = 0; j < it->second.size(); j++) {
				error += it->second[j];
				if (j != it->second.size() - 1) {
					error += ", ";
				}
			}
			error += " }\n";
		}
		ESINFO(GLOBAL_ERROR) << error;
	}
}

void Reader::set(const Environment &env)
{
	Test::setLevel(env.testing_level);
	Info::setLevel(env.verbose_level, env.testing_level);
	Measure::setLevel(env.measure_level);
	Logging::output = env.log_dir;
	Logging::rank = env.MPIrank;
}

static void printConfiguration(const Configuration &configuration, size_t indent)
{
	for (size_t i = 0; i < configuration.orderedParameters.size(); i++) {
		ParameterBase *parameter = configuration.orderedParameters[i];
		ESINFO(ALWAYS) << spaces(indent) << uppercase(parameter->name) << " = " << parameter->get();
	}

	for (size_t i = 0; i < configuration.orderedSubconfiguration.size(); i++) {
		ESINFO(ALWAYS) << spaces(indent) << uppercase(configuration.orderedSubconfiguration[i]->name) << " {";
		printConfiguration(*configuration.orderedSubconfiguration[i], indent + 2);
		ESINFO(ALWAYS) << spaces(indent) << "}";
	}
}

static void storeConfiguration(std::ofstream &os, const Configuration &configuration, size_t indent)
{
	for (size_t i = 0; i < configuration.storeParameters().size(); i++) {
		ParameterBase *parameter = configuration.storeParameters()[i];
		os << "\n" << spaces(indent) << "# " << parameter->description << " [" << parameter->allowedValue << "]\n";
		os << spaces(indent) << uppercase(parameter->name) << " " << parameter->get() << ";\n";
	}

	for (size_t i = 0; i < configuration.storeConfigurations().size(); i++) {
		os << "\n" << spaces(indent) << uppercase(configuration.storeConfigurations()[i]->name) << " { ";
		os << "# " << configuration.storeConfigurations()[i]->description << "\n";
		storeConfiguration(os, *configuration.storeConfigurations()[i], indent + 2);
		os << spaces(indent) << "}\n\n";
	}
}

void Reader::print(const Configuration &configuration)
{
	ESINFO(ALWAYS) << "ESPRESO configuration:";
	printConfiguration(configuration, 4);
}

void Reader::store(const Configuration &configuration)
{
	std::ofstream os("espreso.ecf.default");

	os << "|*****************************************************************************|\n";
	os << "|-----------------------------------------------------------------------------|\n";
	os << "|                                      |                                      |\n";
	os << "|     ESPRESO CONFIGURATION FILE       |   ESPRESO Version:   1.0             |\n";
	os << "|                                      |   http://espreso.it4i.cz             |\n";
	os << "|-----------------------------------------------------------------------------|\n";
	os << "|  Case Description:    Default ESPRESO configuration                         |\n";
	os << "|                                                                             |\n";
	os << "|-----------------------------------------------------------------------------|\n";
	os << "|*****************************************************************************|\n";
	os << "                                                                               \n";
	os << "                                                                               \n";
	os << "|*****************************************************************************|\n";
	os << "|-------------------------  INPUT/OUTPUT DEFINITION --------------------------|\n\n";

	storeConfiguration(os, configuration, 0);
	ESINFO(ALWAYS) << "configuration stored to 'espreso.ecf.default'";
}



