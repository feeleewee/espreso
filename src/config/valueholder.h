
#ifndef SRC_CONFIG_VALUEHOLDER_H_
#define SRC_CONFIG_VALUEHOLDER_H_

#include "configuration.h"
#include "../basis/utilities/parser.h"
#include "../basis/logging/logging.h"
#include <sstream>
#include <cmath>

namespace espreso {

template <typename Ttype>
struct ECFValueHolder: public ECFValue {
	Ttype &value;

	ECFValueHolder(Ttype &value): value(value) {}

	std::string getValue() const
	{
		std::stringstream ss;
		ss << value;
		return ss.str();
	}

	bool setValue(const std::string &value)
	{
		std::stringstream ss(value);
		ss >> this->value;
		return ss.eof() && !ss.fail();
	}
};

template <>
inline std::string ECFValueHolder<std::string>::getValue() const
{
	return value;
}

template <>
inline std::string ECFValueHolder<ECFExpression>::getValue() const
{
	return value.value;
}

template <>
inline bool ECFValueHolder<std::string>::setValue(const std::string &value)
{
	this->value = value;
	return true;
}

template <>
inline bool ECFValueHolder<ECFExpression>::setValue(const std::string &value)
{
	this->value.value = Parser::uppercase(value);
	return true;
}

template <>
inline bool ECFValueHolder<bool>::setValue(const std::string &value)
{
	if (value.size() == 0) {
		this->value = true;
		return true;
	} else {
		if (StringCompare::caseInsensitiveEq(value, "FALSE")) {
			this->value = false;
			return true;
		}
		if (StringCompare::caseInsensitiveEq(value, "TRUE")) {
			this->value = true;
			return true;
		}
		std::stringstream ss(value);
		ss >> this->value;
		return ss.eof();
	}
}

template <>
inline std::string ECFValueHolder<bool>::getValue() const
{
	if (value) {
		return "TRUE";
	} else {
		return "FALSE";
	}
}

template <typename Ttype>
struct ECFEnumHolder: public ECFValue {
	Ttype &value;

	ECFEnumHolder(Ttype &value): value(value) {}

	std::string getValue() const {
		if (metadata.datatype.front() == ECFDataType::OPTION) {
			return metadata.options[static_cast<int>(value)].name;
		}
		if (metadata.datatype.front() == ECFDataType::ENUM_FLAGS) {
			std::string flags;
			for (size_t i = 0; i < metadata.options.size(); i++) {
				if (static_cast<int>(value) & static_cast<int>(std::pow(2, i))) {
					if (flags.size()) {
						flags += " & ";
					}
					flags += metadata.options[i].name;
				}
			}
			return flags;
		}
		ESINFO(ERROR) << "ESPRESO internal error: get value from ECFEnumHolder.";
		return "";
	}

	bool setValue(const std::string &value)
	{
		size_t index = -1;
		for (size_t i = 0; i < metadata.options.size(); i++) {
			if (StringCompare::caseInsensitiveEq(value, metadata.options[i].name)) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			if (!ECFValueHolder<size_t>(index).setValue(value)) {
				return false;
			}
		}

		if (metadata.datatype.front() == ECFDataType::OPTION) {
			this->value = static_cast<Ttype>(index);
			return true;
		}
		if (metadata.datatype.front() == ECFDataType::ENUM_FLAGS) {
			this->value = static_cast<Ttype>(std::pow(2, index));
			return true;
		}
		ESINFO(ERROR) << "ESPRESO internal error: set value to ECFEnumHolder.";
		return false;
	}

};

}



#endif /* SRC_CONFIG_VALUEHOLDER_H_ */
