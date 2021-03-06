
#ifndef SRC_CONFIG_ECF_ENVIRONMENT_H_
#define SRC_CONFIG_ECF_ENVIRONMENT_H_

#include "mpi.h"

#include <string>

namespace espreso {

struct Environment {

	Environment();

	int MPIrank;
	int MPIsize;
	MPI_Comm MPICommunicator;

	std::string executable;

	size_t MKL_NUM_THREADS;
	size_t OMP_NUM_THREADS;
	size_t SOLVER_NUM_THREADS;
	size_t PAR_NUM_THREADS;
	size_t CILK_NWORKERS;

	std::string log_dir;

	size_t verbose_level;
	size_t testing_level;
	size_t measure_level;

	bool print_matrices;
	bool remove_old_results;
};

extern Environment *environment;

}



#endif /* SRC_CONFIG_ECF_ENVIRONMENT_H_ */
