
#ifndef SRC_CONFIG_DESCRIPTION_H_
#define SRC_CONFIG_DESCRIPTION_H_

#include "configuration.h"

namespace espreso {

enum class INPUT {
	/// Ansys generated by Matsol library
	MATSOL = 0,
	/// Ansys Workbench format
	WORKBENCH = 1,
	/// OpenFOAM format
	OPENFOAM = 2,
	/// ESPRESO binary format
	ESDATA = 3,
	/// ESPRESO internal problem generator
	GENERATOR = 4
};

struct Environment: public Configuration {

	PARAMETER(int, MPIrank, "Rank of an MPI process.", 0);
	PARAMETER(int, MPIsize, "Size of an MPI communicator (MPI_COMM_WORLD).", 1);

	PARAMETER(size_t, MKL_NUM_THREADS, "Number of MKL threads.", Esutils::getEnv<size_t>("MKL_NUM_THREADS"));
	PARAMETER(size_t, OMP_NUM_THREADS, "Number of OMP threads.", Esutils::getEnv<size_t>("OMP_NUM_THREADS"));
	PARAMETER(size_t, SOLVER_NUM_THREADS, "Number of threads used in ESPRESO solver.", Esutils::getEnv<size_t>("SOLVER_NUM_THREADS"));
	PARAMETER(size_t, PAR_NUM_THREADS, "Number of parallel threads.", Esutils::getEnv<size_t>("PAR_NUM_THREADS"));
	PARAMETER(size_t, CILK_NWORKERS, "Number of cilk++ threads.", Esutils::getEnv<size_t>("CILK_NWORKERS"));

	std::string executable;

};


struct ESPRESOGenerator: public Configuration {

	PARAMETER(size_t, x, "gx", 2);
	PARAMETER(size_t, y, "gy", 3);
};

struct FETISolver: public Configuration {

	PARAMETER(size_t, iterations, "solver iterations", 100);
};

struct GlobalConfiguration: public Configuration {

	PARAMETER(double, x, "test1", 1.5);
	PARAMETER(double, y, "test2", 2.5);
	PARAMETER(int   , z, "test3", 2);

	OPTION(INPUT, input, "test input", INPUT::GENERATOR, OPTIONS({
			{ "MATSOL", INPUT::MATSOL, "IT4I internal library" },
			{ "WORKBENCH", INPUT::WORKBENCH, "Ansys Workbench input file" },
			{ "OPENFOAM", INPUT::OPENFOAM, "OpenFOAM input format" },
			{ "ESDATA", INPUT::ESDATA, "ESPRESO binary format" },
			{ "GENERATOR", INPUT::GENERATOR, "ESPRESO internal generator" }
	}));

	SUBCONFIG(Environment     , env);
	SUBCONFIG(ESPRESOGenerator, generator);
	SUBCONFIG(FETISolver      , solver);
};

extern GlobalConfiguration configuration;

}


#endif /* SRC_CONFIG_DESCRIPTION_H_ */
