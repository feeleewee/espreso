
#ifndef SRC_ASSEMBLER_PHYSICS_PRECOMPUTED_SINGULAR_ASSEMBLER_H_
#define SRC_ASSEMBLER_PHYSICS_PRECOMPUTED_SINGULAR_ASSEMBLER_H_

#include "../assembler.h"

namespace espreso {

struct SingularSystem: public PrecomputedPhysics
{
	SingularSystem(APIMesh &mesh, Constraints &constraints, SparseMatrix::MatrixType type, double *rhs, size_t rhs_size)
	: PrecomputedPhysics(
			mesh, constraints, type,
			{}, {}, {}, {}, {},
			rhs, rhs_size) {};

	void prepareMeshStructures();
	void assembleStiffnessMatrix(const Element* e, DenseMatrix &Ke, std::vector<double> &fe, std::vector<eslocal> &dofs) const;
	void makeStiffnessMatricesRegular();
	void assembleGluingMatrices();

protected:
	void composeSubdomain(size_t subdomain);
};

}


#endif /* SRC_ASSEMBLER_PHYSICS_PRECOMPUTED_SINGULAR_ASSEMBLER_H_ */