
#ifndef SRC_CONFIG_ECF_PYTHONTESTGENERATOR_H_
#define SRC_CONFIG_ECF_PYTHONTESTGENERATOR_H_

#include "../configuration.h"

namespace espreso {

struct PythonTestGenerator: public ECFObject {

	std::string output, env, warmup, run, exe;
	size_t measure_repetition, gather_level;

	std::map<size_t, std::string> levels, args;
	std::map<std::string, std::string> variables;

	std::map<std::string, std::map<std::string, std::string> > tables;

	PythonTestGenerator();
};

}


#endif /* SRC_CONFIG_ECF_PYTHONTESTGENERATOR_H_ */
