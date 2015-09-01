
#ifndef INPUT_MESHGENERATOR_ELEMENTS_2D_SQUARE8_H_
#define INPUT_MESHGENERATOR_ELEMENTS_2D_SQUARE8_H_

#include "esmesh.h"

namespace espreso {
namespace input {

class Square8 {

public:
	static size_t subelements;
	static size_t subnodes[3];

	static void addElements(std::vector<Element*> &elements, const eslocal indices[], const eslocal params[]);

};

}
}


#endif /* INPUT_MESHGENERATOR_ELEMENTS_2D_SQUARE8_H_ */