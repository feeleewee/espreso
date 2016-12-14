
#ifndef SRC_CONFIG_INPUTGENERATORELEMENTS_H_
#define SRC_CONFIG_INPUTGENERATORELEMENTS_H_

#include "configuration.h"

namespace espreso {

enum class ELEMENT_TYPE {
	HEXA8,
	HEXA20,
	TETRA4,
	TETRA10,
	PRISMA6,
	PRISMA15,
	PYRAMID5,
	PYRAMID13,

	SQUARE4,
	SQUARE8,
	TRIANGLE3,
	TRIANGLE6
};

}


#endif /* SRC_CONFIG_INPUTGENERATORELEMENTS_H_ */
