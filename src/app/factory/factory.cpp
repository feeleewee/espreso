
#include <signal.h>
#include <csignal>

#include "factory.h"

#include "heattransferfactory.h"
#include "structuralmechanicsfactory.h"

#include "../../assembler/physicssolver/timestep/timestepsolver.h"
#include "../../assembler/physicssolver/loadstep/loadstepsolver.h"
#include "../../assembler/physicssolver/assembler.h"
#include "../../assembler/physics/physics.h"
#include "../../assembler/step.h"
#include "../../assembler/instance.h"
#include "../../config/ecf/root.h"
#include "../../input/input.h"
#include "../../mesh/mesh.h"
#include "../../mesh/preprocessing/meshpreprocessing.h"
#include "../../output/result/resultstore.h"
#include "../../output/data/espresobinaryformat.h"
#include "../../solver/generic/FETISolver.h"


namespace espreso {

static void signalHandler(int signal)
{
	switch (signal) {
	case SIGTERM:
		ESINFO(ERROR) << "SIGTERM -termination request, sent to the program";
		break;
	case SIGSEGV:
		ESINFO(ERROR) << "SIGSEGV - invalid memory access (segmentation fault)";
		break;
	case SIGINT:
		ESINFO(ERROR) << "SIGINT - external interrupt, usually initiated by the user";
		break;
	case SIGILL:
		ESINFO(ERROR) << "SIGILL - invalid program image, such as invalid instruction";
		break;
	case SIGABRT:
		ESINFO(ERROR) << "SIGABRT - abnormal termination condition, as is e.g. initiated by std::abort()";
		break;
	case SIGFPE:
		ESINFO(ERROR) << "SIGFPE - erroneous arithmetic operation such as divide by zero";
		break;
	default:
		ESINFO(ERROR) << "ESPRESO trigger error " << signal << ".";
	}
}

void ESPRESO::run(int *argc, char ***argv)
{
	std::signal(SIGTERM, signalHandler);
	std::signal(SIGSEGV, signalHandler);
	std::signal(SIGINT, signalHandler);
	std::signal(SIGILL, signalHandler);
	std::signal(SIGABRT, signalHandler);
	std::signal(SIGFPE, signalHandler);

	ECFRoot configuration(argc, argv);
	Mesh mesh(configuration);
	ResultStore* solutionStore = ResultStore::createAsynchronizedStore(mesh, configuration.output);

	std::string processes, threads;
	if (solutionStore->storeProcesses) {
		processes = std::to_string(solutionStore->computeProcesses) + " + " + std::to_string(solutionStore->storeProcesses);
	} else {
		processes = std::to_string(solutionStore->computeProcesses);
	}
	if (solutionStore->storeThreads) {
		threads = std::to_string(environment->OMP_NUM_THREADS) + " + " + std::to_string(solutionStore->storeThreads);
	} else {
		threads = std::to_string(environment->OMP_NUM_THREADS);
	}

	ESINFO(OVERVIEW) << "Run ESPRESO SOLVER using " << processes << " MPI and " << threads << " threads.";

	auto computeSolution = [&] () {
		switch (configuration.input) {
		case INPUT_FORMAT::WORKBENCH:
			return !configuration.workbench.convert_database;
		case INPUT_FORMAT::OPENFOAM:
			return !configuration.openfoam.convert_database;
		default:
			return true;
		}
	};

	if (ResultStore::isComputeNode()) {
		Factory factory(configuration, mesh, *solutionStore);
		if (computeSolution()) {
			factory.solve();
		} else {
			ESPRESOBinaryFormat::store(mesh, configuration);
		}
	}
	ResultStore::destroyAsynchronizedStore();
}

Factory::Factory(const ECFRoot &configuration, Mesh &mesh, ResultStore &store)
: _mesh(&mesh), _store(&store), _loader(NULL)
{
	_step = new Step();
	Logging::step = _step;
	Input::load(configuration, mesh);

	// LOAD PHYSICS
	switch (configuration.physics) {
	case PHYSICS::HEAT_TRANSFER_2D:
		_loader = new HeatTransferFactory(_step, configuration.heat_transfer_2d, configuration.output.results_selection, _mesh);
		break;
	case PHYSICS::HEAT_TRANSFER_3D:
		_loader = new HeatTransferFactory(_step, configuration.heat_transfer_3d, configuration.output.results_selection, _mesh);
		break;
	case PHYSICS::STRUCTURAL_MECHANICS_2D:
		_loader = new StructuralMechanicsFactory(_step, configuration.structural_mechanics_2d, configuration.output.results_selection, _mesh);
		break;
	case PHYSICS::STRUCTURAL_MECHANICS_3D:
		_loader = new StructuralMechanicsFactory(_step, configuration.structural_mechanics_3d, configuration.output.results_selection, _mesh);
		break;
	default:
		ESINFO(GLOBAL_ERROR) << "Unknown PHYSICS in configuration file";
	}

	for (size_t step = 0; step < _loader->loadSteps(); step++) {
		_loadSteps.push_back(_loader->getLoadStepSolver(step, _mesh, _store));
	}

	_loader->preprocessMesh();
	mesh.initNodeData();

	mesh.preprocessing->finishPreprocessing();

	_store->updateMesh();
}

void Factory::solve()
{
	for (_step->step = 0; _step->step < _loadSteps.size(); _step->step++) {
		_loadSteps[_step->step]->run();
	}
}

template <class TType>
static void clear(std::vector<TType> &vector)
{
	for (size_t i = 0; i < vector.size(); i++) {
		delete vector[i];
	}
}

FactoryLoader::~FactoryLoader()
{
	clear(_instances);
	clear(_physics);
	clear(_linearSolvers);
	clear(_assemblers);
	clear(_timeStepSolvers);
	clear(_loadStepSolvers);
}

LinearSolver* FactoryLoader::getLinearSolver(const LoadStepConfiguration &settings, Instance *instance) const
{
	switch (settings.solver) {
	case LoadStepConfiguration::SOLVER::FETI:
		return new FETISolver(instance, settings.feti);
	default:
		ESINFO(GLOBAL_ERROR) << "Not implemented requested SOLVER.";
		return NULL;
	}
}

void FactoryLoader::preprocessMesh()
{
	// TODO: generalize it !!

	for (size_t i = 0; i < _physics.size(); i++) {
		_physics[i]->prepare();
	}
}

Factory::~Factory()
{
	delete _loader;
}

void FactoryLoader::printError(const std::string &error) const
{
	ESINFO(GLOBAL_ERROR) << error;
}

}


