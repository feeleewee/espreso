
#ifndef SRC_ASSEMBLER_PHYSICS_PRECOMPUTED_UNIFORMSYMMETRIC3DOFS_ASSEMBLER_H_
#define SRC_ASSEMBLER_PHYSICS_PRECOMPUTED_UNIFORMSYMMETRIC3DOFS_ASSEMBLER_H_

#include "../assembler.h"

namespace espreso {

struct UniformSymmetric3DOFs: public PrecomputedPhysics
{
	UniformSymmetric3DOFs(const APIMesh &mesh, double *rhs, size_t rhs_size)
	: PrecomputedPhysics(
			mesh,
			{ Property::DISPLACEMENT_X, Property::DISPLACEMENT_Y, Property::DISPLACEMENT_Z },
			SparseMatrix::MatrixType::REAL_SYMMETRIC_POSITIVE_DEFINITE, rhs, rhs_size) {};

	void init();

protected:
	void composeSubdomain(size_t subdomain);
};

}


#endif /* SRC_ASSEMBLER_PHYSICS_PRECOMPUTED_UNIFORMSYMMETRIC3DOFS_ASSEMBLER_H_ */
