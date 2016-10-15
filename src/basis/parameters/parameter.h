
#ifndef SRC_BASIS_PARAMETERS_PARAMETER_H_
#define SRC_BASIS_PARAMETERS_PARAMETER_H_

#include <cstdlib>

#include "../logging/logging.h"
#include "parser.h"

#include "envelopes/data.h"
#include "envelopes/options.h"
#include "envelopes/stringoptions.h"
#include "envelopes/map.h"

namespace espreso {

struct ArgsConfiguration {
	std::string path;
	std::vector<std::string> nameless;
};


struct Parameter {
	std::string name;
	Envelope *data;
	std::string description;
	std::vector<std::string> options;
	size_t verboseLevel;

	bool operator<(const Parameter &other)
	{
		return StringCompare::caseInsensitive(name, other.name);
	}

	bool operator<(Parameter &other) const
	{
		return StringCompare::caseInsensitive(name, other.name);
	}

	bool operator<(Parameter &other)
	{
		return StringCompare::caseInsensitive(name, other.name);
	}

	bool operator<(const std::string &str) const
	{
		return StringCompare::caseInsensitive(name, str);
	}

	template <typename TParameter>
	Parameter(std::string name, TParameter &defaultValue, std::string description, size_t verboseLevel = 2)
	: name(name), description(description), verboseLevel(verboseLevel)
	{
		data = new DataEnvelope<TParameter>(defaultValue);
	}

	template <typename TParameter>
	Parameter(std::string name, TParameter &defaultValue, std::string description, std::vector<std::pair<std::string, std::string> > options, size_t verboseLevel = 2)
	: name(name), description(description), verboseLevel(verboseLevel)
	{
		data = new StringOptionEnvelope<TParameter>(defaultValue, options);
	}

	template <typename TParameter>
	Parameter(std::string name, TParameter &defaultValue, std::string description, std::vector<Option2<TParameter> > options, size_t verboseLevel = 2)
	: name(name), description(description), verboseLevel(verboseLevel)
	{
		data = new OptionEnvelope<TParameter>(defaultValue, options);
	}

	template <typename TParameter>
	Parameter(std::string name, std::map<std::string, TParameter> &defaultValue, std::string description, size_t verboseLevel = 2)
	: name(name), description(description), verboseLevel(verboseLevel)
	{
		data = new MapEnvelope<TParameter>(defaultValue);
	}

	Parameter(const Parameter &other)
	{
		name = other.name;
		data = other.data != NULL ? other.data->copy() : NULL;
		description = other.description;
		verboseLevel = other.verboseLevel;
	}

	Parameter& operator=(const Parameter &other)
	{
		if (this != &other) {
			name = other.name;
			if (data != NULL) {
				delete data;
			}
			data = other.data != NULL ? other.data->copy() : NULL;
			description = other.description;
			verboseLevel = other.verboseLevel;
		}
		return *this;
	}

	Parameter(): data(NULL), verboseLevel(-1) {};
	~Parameter()
	{
		if (data != NULL) {
			delete data;
		}
	}

	void set(const std::string &value)
	{
		if (!data->set(value)) {
			ESINFO(GLOBAL_ERROR) << "Parameter '" << name << "' has a wrong value '" << value << "'.";
		}
	}

	void set(const std::string &attribute, const std::string &value)
	{
		if (!data->set(attribute, value)) {
			ESINFO(GLOBAL_ERROR) << "Parameter '" << name << "', attribute '" << attribute << "' has a wrong value '" << value << "'.";
		}
	}

	std::string get() const
	{
		return data->get();
	}

	std::string get(const std::string &parameter) const
	{
		return data->get(parameter);
	}
};

}


#endif /* SRC_BASIS_PARAMETERS_PARAMETER_H_ */
