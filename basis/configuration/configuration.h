
#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "parameter.h"

#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <stdlib.h>

namespace espreso {
namespace input {

class Configuration {

public:
	Configuration(std::vector<Description> &description, const Options &options);
	Configuration(std::vector<Description> &description, const std::string &path);
	~Configuration();

	void print() const;

	bool isSet(const std::string &parameter) const
	{
		if (_parameters.find(parameter) != _parameters.end()) {
			return _parameters.find(parameter)->second->isSet();
		} else {
			return false;
		}
	}

	template<class ParameterType>
	const ParameterType value(const std::string &parameter, ParameterType defaultValue) const
	{
		if (_parameters.find(parameter) != _parameters.end()) {
			return static_cast<ParameterType>(_getValue(parameter, defaultValue));
		} else {
			return defaultValue;
		}
	}

private:

	eslocal _getValue(const std::string &parameter, eslocal defaultValue) const;
	double _getValue(const std::string &parameter, double defaultValue) const;
	const char* _getValue(const std::string &parameter, const char* defaultValue) const;
	std::string _getValue(const std::string &parameter, const std::string &defaultValue) const;
	bool _getValue(const std::string &parameter, bool defaultValue) const;

	void load(const Options &options);

	std::map<std::string, Parameter*, ParameterCompare> _parameters;
};

}
}

#endif /* CONFIGURATION_H_ */