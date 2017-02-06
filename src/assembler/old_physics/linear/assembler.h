
#ifndef SRC_ASSEMBLER_OLD_PHYSICS_LINEAR_ASSEMBLER_H_
#define SRC_ASSEMBLER_OLD_PHYSICS_LINEAR_ASSEMBLER_H_

#include "../assembler.h"

namespace espreso {

struct LinearPhysics: public OldPhysics {

	virtual bool singular() const
	{
		return true;
	}

	virtual void assembleStiffnessMatrices();

	LinearPhysics(
			Mesh &mesh,
			Constraints &constraints,
			const ESPRESOSolver &configuration,
			MatrixType mtype,
			const std::vector<Property> elementDOFs,
			const std::vector<Property> faceDOFs,
			const std::vector<Property> edgeDOFs,
			const std::vector<Property> pointDOFs,
			const std::vector<Property> midPointDOFs)
	: OldPhysics(mesh, constraints, configuration, mtype, elementDOFs, faceDOFs, edgeDOFs, pointDOFs, midPointDOFs) {};
	virtual ~LinearPhysics() {};

protected:
	virtual void composeSubdomain(size_t subdomain) =0;
};

}


#endif /* SRC_ASSEMBLER_OLD_PHYSICS_LINEAR_ASSEMBLER_H_ */