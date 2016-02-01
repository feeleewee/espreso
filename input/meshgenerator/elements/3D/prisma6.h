
#ifndef INPUT_MESHGENERATOR_ELEMENTS_3D_PRISMA6_H_
#define INPUT_MESHGENERATOR_ELEMENTS_3D_PRISMA6_H_

#include "esmesh.h"

namespace esinput {

class Prisma6 {

public:
	static size_t subelements;
	static size_t subnodes[3];

	static void addElements(std::vector<mesh::Element*> &elements, const eslocal indices[], const eslocal params[]);

};

}



#endif /* INPUT_MESHGENERATOR_ELEMENTS_3D_PRISMA6_H_ */
