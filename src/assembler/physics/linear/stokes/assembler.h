
#ifndef SRC_ASSEMBLER_PHYSICS_LINEAR_STOKES_ASSEMBLER_H_
#define SRC_ASSEMBLER_PHYSICS_LINEAR_STOKES_ASSEMBLER_H_

#include "../assembler.h"

namespace espreso {

struct Stokes: public LinearPhysics
{
	Stokes(const Mesh &mesh)
	: LinearPhysics(
			mesh,
			{ Property::DISPLACEMENT_X, Property::DISPLACEMENT_Y, Property::PRESSURE },
			{ Property::FIXED_DISPLACEMENT_X, Property::FIXED_DISPLACEMENT_Y, Property::FIXED_PRESSURE },
			SparseMatrix::MatrixType::REAL_SYMMETRIC_INDEFINITE) {};

protected:
	void composeSubdomain(size_t subdomain);
};

}



#endif /* SRC_ASSEMBLER_PHYSICS_LINEAR_STOKES_ASSEMBLER_H_ */
