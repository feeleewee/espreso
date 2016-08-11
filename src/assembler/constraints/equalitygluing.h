
#ifndef SRC_ASSEMBLER_CONSTRAINTS_EQUALITYGLUING_H_
#define SRC_ASSEMBLER_CONSTRAINTS_EQUALITYGLUING_H_

#include "dirichlet.h"

namespace espreso {

class EqualityGluing: public Dirichlet
{
public:
	EqualityGluing(Mesh &mesh): Dirichlet(mesh) {};

	void insertElementGluingToB1(const std::vector<Element*> &elements, const std::vector<Property> &DOFs);

	void insertDomainGluingToB0(const std::vector<Element*> &elements, const std::vector<Property> &DOFs);

private:
	std::vector<esglobal> computeLambdasID(const std::vector<Element*> &elements, const std::vector<Property> &DOFs);

};

}



#endif /* SRC_ASSEMBLER_CONSTRAINTS_EQUALITYGLUING_H_ */
