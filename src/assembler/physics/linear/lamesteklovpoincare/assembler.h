
#ifndef SRC_ASSEMBLER_PHYSICS_LINEAR_LAMESTEKLOVPOINCARE_ASSEMBLER_H_
#define SRC_ASSEMBLER_PHYSICS_LINEAR_LAMESTEKLOVPOINCARE_ASSEMBLER_H_

#include "../assembler.h"

namespace espreso {

struct LameSteklovPoincare: public LinearPhysics
{
	LameSteklovPoincare(const Mesh &mesh): LinearPhysics(mesh, 3) {};

protected:
	void composeSubdomain(size_t subdomain);
};

}


#endif /* SRC_ASSEMBLER_PHYSICS_LINEAR_LAMESTEKLOVPOINCARE_ASSEMBLER_H_ */