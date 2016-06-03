
#include "esconfig.h"
#include "esbasis.h"

namespace espreso {

namespace config {

//////////////////////////// ENVIRONMENT ///////////////////////////////////////

int env::MPIrank = 0;
int env::MPIsize = 1;

size_t env::MKL_NUM_THREADS    = Esutils::getEnv<size_t>("MKL_NUM_THREADS");
size_t env::OMP_NUM_THREADS    = Esutils::getEnv<size_t>("OMP_NUM_THREADS");
size_t env::SOLVER_NUM_THREADS = Esutils::getEnv<size_t>("SOLVER_NUM_THREADS");
size_t env::PAR_NUM_THREADS    = Esutils::getEnv<size_t>("PAR_NUM_THREADS");
size_t env::CILK_NWORKERS      = Esutils::getEnv<size_t>("CILK_NWORKERS");

std::string env::executable	   = Esutils::getEnv<std::string>("_");
std::string env::configurationFile = "espreso.config";


///////////////////////////////// MESH /////////////////////////////////////////

std::string mesh::PATH;
int mesh::INPUT = GENERATOR;

size_t mesh::SUBDOMAINS = 8;
size_t mesh::FIX_POINTS  = 8;

size_t mesh::CORNERS       = 1;
bool   mesh::VERTEX_CORNERS = true;
bool   mesh::EDGE_CORNERS   = true;
bool   mesh::FACE_CORNERS   = false;

bool   mesh::AVERAGE_EDGES  = false;
bool   mesh::AVERAGE_FACES  = false;

/////////////////////////////// SOLVER /////////////////////////////////////////

double   solver::epsilon                = 1e-5;
size_t   solver::maxIterations          = 1000;
size_t   solver::FETI_METHOD            = TOTAL_FETI;
size_t   solver::PRECONDITIONER         = LUMPED;
size_t   solver::CG_SOLVER              = STANDARD;
size_t   solver::REGULARIZATION         = FIX_POINTS;

bool     solver::REDUNDANT_LAGRANGE     = true;
size_t   solver::B0_TYPE                = B0Type::KERNELS;
bool     solver::USE_SCHUR_COMPLEMENT   = false;
size_t   solver::SCHUR_COMPLEMENT_PREC  = SC_DOUBLE_PRECISION;
size_t   solver::SCHUR_COMPLEMENT_TYPE  = GENERAL;
bool     solver::COMBINE_SC_AND_SPDS    = true;
bool     solver::KEEP_FACTORS           = true;

size_t   solver::KSOLVER                = DIRECT_DOUBLE_PRECISION;
size_t   solver::KSOLVER_SP_iter_steps  = 1000;
double   solver::KSOLVER_SP_iter_norm   = 1e-12;
size_t   solver::F0_SOLVER              = KSOLVER_PRECISION;

size_t   solver::N_MICS                 = 2;
size_t 	 solver::SA_SOLVER				= 0;
/////////////////////////////// ASSEMBLER //////////////////////////////////////

int    assembler::discretization = FEM;
int    assembler::physics        = LinearElasticity;
size_t assembler::timeSteps      = 1;

/////////////////////////////// OUTPUT /////////////////////////////////////////

bool output::saveMesh      = false;
bool output::saveFixPoints = false;
bool output::saveFaces     = false;
bool output::saveLines     = false;
bool output::saveCorners   = false;
bool output::saveDirichlet = false;
bool output::saveAveraging = false;
bool output::saveResults   = true;

double output::subdomainShrinkRatio = .95;
double output::clusterShrinkRatio   = .9;

//////////////////////////////// INFO //////////////////////////////////////////

std::string info::output = "log";

size_t info::verboseLevel = 0;
size_t info::testingLevel = 0;
size_t info::measureLevel = 0;

bool info::printMatrices = false;


/////////////////////////////// DESCRIPTION ////////////////////////////////////


std::vector<espreso::Parameter> parameters = {

	// MESH DESCRIPTION
	{ "PATH", mesh::PATH, "A path to an example.", Parameter::Help::WRITE},
	{ "INPUT", mesh::INPUT, "A format of an input.", {
			{ "MATSOL", "IT4I internal library" },
			{ "WORKBENCH", "Ansys Workbench input file" },
			{ "OPENFOAM", "OpenFOAM input format" },
			{ "ESDATA", "ESPRESO binary format" },
			{ "GENERATOR", "ESPRESO internal generator" } },  Parameter::Help::WRITE},

	{ "SUBDOMAINS", mesh::SUBDOMAINS, "Number of subdomains in a cluster.", Parameter::Help::WRITE },
	{ "FIX_POINTS" , mesh::FIX_POINTS , "Number of fix points in a subdomain." },

	{ "CORNERS"        , mesh::CORNERS      , "Number of corners on an edge or a face." },
	{ "VERTEX_CORNERS" , mesh::VERTEX_CORNERS, "Set corners to vertices." },
	{ "EDGE_CORNERS"   , mesh::EDGE_CORNERS  , "Set corners on edges. The number is defined by parameter CORNERS." },
	{ "FACE_CORNERS"   , mesh::FACE_CORNERS  , "Set corners on faces. The number is defined by parameter CORNERS." },

	{ "AVERAGE_EDGES"  , mesh::AVERAGE_EDGES, "Average nodes on edges." },
	{ "AVERAGE_FACES"  , mesh::AVERAGE_FACES, "Average nodes on faces." },

	// ASSEMBLER DESCRIPTION
	{ "DISCRETIZATION", config::assembler::discretization, "A used discretization.",{
			{ "FEM", "desc" },
			{ "BEM", "desc" } }, Parameter::Help::WRITE },

	// SOLVER DESCRIPTION
	{ "EPSILON", solver::epsilon, "Solver requested precision.", Parameter::Help::WRITE },
	{ "ITERATIONS", solver::maxIterations, "Solver maximum iterations.", Parameter::Help::WRITE },
	{ "FETI_METHOD", solver::FETI_METHOD, "The FETI method used by ESPRESO.", {
			{ "Total FETI", "desc" },
			{ "Hybrid Total FETI", "desc" } }, Parameter::Help::WRITE },

	{ "PRECONDITIONER", solver::PRECONDITIONER, "Preconditioner.", {
			{ "NO preconditioner", "desc" },
			{ "Lumped", "desc" },
			{ "weight function", "desc" },
			{ "Dirichlet", "desc" } }, Parameter::Help::WRITE },

	{ "CGSOLVER", solver::CG_SOLVER, "Conjugate gradients solver", {
			{ "standard", "desc" },
			{ "pipelined", "desc" } }, Parameter::Help::WRITE },


	{ "REGULARIZATION", solver::REGULARIZATION, "Regularization of stiffness matrix.", {
			{ "fix points", "desc" },
			{ "random detection of null pivots", "desc" } }},

	{ "KSOLVER", solver::KSOLVER, "K solver precision.", {
			{ "directly with double precision", "desc" },
			{ "iteratively", "desc" },
			{ "directly with single precision", "desc" },
			{ "directly with mixed precision", "desc" } }},

	{ "F0SOLVER", solver::F0_SOLVER, "F0 solver precision.", {
			{ "with the same precision as KSOLVER", "desc" },
			{ "always with double precision.", "desc" } }},

	{ "SASOLVER", solver::SA_SOLVER, "SA solver type.", {
			{ "DENSE solver on CPU", "desc" },
			{ "DENSE solver on ACC", "desc" },
			{ "SPARSE solver on CPU.", "desc" } }},


	{ "REDUNDANT_LAGRANGE", solver::REDUNDANT_LAGRANGE, "Set Lagrange multipliers also among HFETI corners." },
	{ "B0_TYPE", solver::B0_TYPE, "The source for B0 assembler." },
	{ "USE_SCHUR_COMPLEMENT", solver::USE_SCHUR_COMPLEMENT, "Use schur complement for stiffness matrix processing" },
	{ "SCHUR_COMPLEMENT_PREC", solver::SCHUR_COMPLEMENT_PREC, "Schur complement precision." },
	{ "SCHUR_COMPLEMENT_TYPE", solver::SCHUR_COMPLEMENT_TYPE, "Schur complement matrix type.", {
			{ "general", "desc" },
			{ "symmetric", "desc" } }},

	{ "COMBINE_SC_AND_SPDS", solver::COMBINE_SC_AND_SPDS, "Combine Schur complement for GPU and sparse direct solver for CPU." },
	{ "KEEP_FACTORS", solver::KEEP_FACTORS, "Keep factors for whole iteration process." },

	{ "KSOLVER_SP_iter_steps", solver::KSOLVER_SP_iter_steps, "Number of reiteration steps for SP direct solver." },
	{ "KSOLVER_SP_iter_norm", solver::KSOLVER_SP_iter_norm , "Number of reiteration steps for SP direct solver." },

	{ "N_MICS", solver::N_MICS, "Number of MIC accelerators.", Parameter::Help::WRITE },

	// OUTPUT DESCRIPTION
	{ "SAVE_MESH"      , output::saveMesh     , "Save an input mesh.", Parameter::Help::WRITE },
	{ "SAVE_FIXPOINTS" , output::saveFixPoints, "Save a mesh fix points." },
	{ "SAVE_FACES"     , output::saveFaces    , "Save faces between subdomains." },
	{ "SAVE_EDGES"     , output::saveLines    , "Save edges among subdomains." },
	{ "SAVE_CORNERS"   , output::saveCorners  , "Save corner nodes." },
	{ "SAVE_DIRICHLET" , output::saveDirichlet, "Save nodes with a dirichlet condition.", Parameter::Help::WRITE },
	{ "SAVE_AVERAGING" , output::saveAveraging, "Save averaged nodes." },
	{ "SAVE_RESULTS"   , output::saveResults  , "Save the results.", Parameter::Help::WRITE },

	{ "SUBDOMAIN_SHRINK_RATIO", output::subdomainShrinkRatio, "Shrink ratio for subdomains.", Parameter::Help::WRITE },
	{ "CLUSTER_SHRINK_RATIO"  , output::clusterShrinkRatio  , "Shrink ratio for clusters.", Parameter::Help::WRITE },

	// INFO DESCRIPTION
	{ "OUTPUT", info::output, "A location for saving output informations.", Parameter::Help::WRITE },
	{ "VERBOSE_LEVEL", info::verboseLevel, "ESPRESO verbose level.", Parameter::Help::WRITE },
	{ "TESTING_LEVEL", info::verboseLevel, "ESPRESO testing level.", Parameter::Help::WRITE },
	{ "MEASURE_LEVEL", info::verboseLevel, "ESPRESO measure level.", Parameter::Help::WRITE },
	{ "PRINT_MATRICES", info::printMatrices, "ESPRESO print solver input matrices." }
};

}

}

