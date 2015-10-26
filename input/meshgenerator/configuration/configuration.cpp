
#include "configuration.h"

namespace esinput {

Configuration::Configuration(std::vector<Description> &description, int argc, char **argv)
{
	_parameters["CMD_LINE_ARGUMENTS"] = new StringParameter(
		"CMD_LINE_ARGUMENTS",
		"Arguments set from the command line."
	);
	for (size_t i = 0; i < description.size(); i++) {
		switch (description[i].type) {
		case STRING_PARAMETER: {
			_parameters[description[i].name] = new StringParameter(
				description[i].name,
				description[i].description
			);
			break;
		}
		case INTEGER_PARAMETER: {
			_parameters[description[i].name] = new IntegerParameter(
				description[i].name,
				description[i].description
			);
			break;
		}
		case DOUBLE_PARAMETER: {
			_parameters[description[i].name] = new DoubleParameter(
				description[i].name,
				description[i].description
			);
			break;
		}
		case BOOLEAN_PARAMETER: {
			_parameters[description[i].name] = new BooleanParameter(
				description[i].name,
				description[i].description
			);
			break;
		}
		}
	}

	load(argc, argv);
}

void Configuration::load(int argc, char** argv)
{
	if (argc < 2) {
		std::cerr << "Specify the path to an example as the first command line attribute.\n";
		exit(EXIT_FAILURE);
	}
	std::ifstream file(argv[1]);
	std::map<std::string, Parameter*>::iterator it;

	if (file.is_open()) {
		std::string line;

		while (getline(file, line, '\n')) {
			line.erase(0, line.find_first_not_of(" "));
			if (!line.size() || line.compare(0, 1, "#") == 0) {
				continue;
			}
			size_t pos = line.find("#");
			if (pos < std::string::npos) {
				line = line.substr(0, pos);
			}
			for (it = _parameters.begin(); it != _parameters.end(); ++it) {
				if (it->second->match(line)) {
					if (it->second->isSet()) {
						std::cout << "Warning: parameter " << it->second->name() << " is set more than once.";
					}
					it->second->set(line);
					break;
				}
			}
		}

		file.close();
	} else {
		std::cout << "The example on path '" << argv[1] << "' not found.\n";
	}

	// Read attributes from command line
	if (_parameters.find("CMD_LINE_ARGUMENTS") == _parameters.end()) {
		return;
	}

	std::vector<std::pair<std::string, int> > cmdLine;
	size_t cmdLineSize = 0;
	if (_parameters["CMD_LINE_ARGUMENTS"]->isSet()) {
		std::string val = value<std::string>("CMD_LINE_ARGUMENTS", "");
		while(true) {
			size_t pos = val.find(" ");
			std::string argument = val.substr(0, pos);
			for (it = _parameters.begin(); it != _parameters.end(); ++it) {
				if (it->second->match(val)) {
					cmdLine.push_back(std::pair<std::string, int>(it->second->name(), cmdLineSize));
					break;
				}
			}
			cmdLineSize++;
			val = val.erase(0, pos);
			val = val.erase(0, val.find_first_not_of(" "));
			if (!val.size()) {
				break;
			}
		}
	}

	if (argc - 2 < cmdLineSize) {
		std::cerr << "Too few command line arguments. ESPRESO assumes " << value<std::string>("CMD_LINE_ARGUMENTS", "") << "\n";
		exit(EXIT_FAILURE);
	}
	if (argc - 2 > cmdLineSize) {
		std::cout << "Warning: ESPRESO omits some command line arguments.\n";
	}
	for (size_t i = 0; i < cmdLine.size(); i++) {
		_parameters[cmdLine[i].first]->set(std::string(_parameters[cmdLine[i].first]->name() + "=" + argv[cmdLine[i].second + 2]));
	}
}

void Configuration::print() const
{
	std::map<std::string, Parameter*>::const_iterator it;
	for (it = _parameters.begin(); it != _parameters.end(); ++it) {
		if (it->second->isSet()) {
			continue;
		}
		std::cout << it->second->name() << " = ";
		switch (it->second->type()) {
			case STRING_PARAMETER: {
				std::cout << "'" << static_cast<StringParameter*>(it->second)->get() << "'";
				break;
			}
			case INTEGER_PARAMETER: {
				std::cout << "'" << static_cast<IntegerParameter*>(it->second)->get() << "'";
				break;
			}
			case DOUBLE_PARAMETER: {
				std::cout << "'" << static_cast<DoubleParameter*>(it->second)->get() << "'";
				break;
			}
			case BOOLEAN_PARAMETER: {
				if (static_cast<BooleanParameter*>(it->second)->get()) {
					std::cout << "true";
				} else {
					std::cout << "false";
				}
				break;
			}
		}
		std::cout << "\n";
	}
}

eslocal Configuration::_getValue(const std::string &parameter, eslocal defaultValue) const
{
	if (_parameters.find(parameter)->second->isSet()) {
		return static_cast<IntegerParameter*>(_parameters.find(parameter)->second)->get();
	} else {
		return defaultValue;
	}
}

double Configuration::_getValue(const std::string &parameter, double defaultValue) const
{
	if (_parameters.find(parameter)->second->isSet()) {
		return static_cast<DoubleParameter*>(_parameters.find(parameter)->second)->get();
	} else {
		return defaultValue;
	}
}

std::string Configuration::_getValue(const std::string &parameter, std::string &defaultValue) const
{
	if (_parameters.find(parameter)->second->isSet()) {
		return static_cast<StringParameter*>(_parameters.find(parameter)->second)->get();
	} else {
		return defaultValue;
	}
}

bool Configuration::_getValue(const std::string &parameter, bool defaultValue) const
{
	if (_parameters.find(parameter)->second->isSet()) {
		return static_cast<BooleanParameter*>(_parameters.find(parameter)->second)->get();
	} else {
		return defaultValue;
	}
}

Configuration::~Configuration()
{
	std::map<std::string, Parameter*>::iterator it;
	for (it = _parameters.begin(); it != _parameters.end(); ++it) {
		delete it->second;
	}
}

}