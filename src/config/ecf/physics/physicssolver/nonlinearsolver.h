
#ifndef SRC_CONFIG_ECF_PHYSICS_PHYSICSSOLVER_NONLINEARSOLVER_H_
#define SRC_CONFIG_ECF_PHYSICS_PHYSICSSOLVER_NONLINEARSOLVER_H_

#include "../../../configuration.h"

namespace espreso {

struct NonLinearSolverConfiguration: public ECFObject {

	enum class METHOD {
		NEWTON_RAPHSON,
		MODIFIED_NEWTON_RAPHSON
	};

	enum class STEPPINGG {
		TRUE,
		FALSE,
		AUTO
	};

	METHOD method;
	STEPPINGG stepping;

	size_t max_iterations, substeps;
	bool line_search, tangent_matrix_correction, adaptive_precision;

	bool check_first_residual, check_second_residual;
	double requested_first_residual, requested_second_residual;

	double r_tol, c_fact;

	NonLinearSolverConfiguration(const std::string &firstResidualName, const std::string &secondResidualName);
};

}



#endif /* SRC_CONFIG_ECF_PHYSICS_PHYSICSSOLVER_NONLINEARSOLVER_H_ */
