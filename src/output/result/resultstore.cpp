
#include "resultstore.h"

#include "../../config/ecf/environment.h"
#include "../../config/ecf/output.h"
#include "../../basis/logging/logging.h"
#include "../../basis/utilities/utils.h"

#include "executor/asyncexecutor.h"
#include "executor/directexecutor.h"
#include "async/Dispatcher.h"

#include "monitors/monitoring.h"
#include "visualization/collected/ensight.h"
#include "visualization/collected/stl.h"
#include "visualization/separated/insitu.h"
#include "visualization/separated/vtklegacy.h"


using namespace espreso;

ResultStore* ResultStore::_asyncStore = NULL;
async::Dispatcher* ResultStore::_dispatcher = NULL;

ResultStoreBase::ResultStoreBase(const Mesh &mesh): _mesh(mesh), _directory("PREPOSTDATA/")
{

}

void ResultStoreBase::createOutputDirectory()
{
	Esutils::createDirectory({ Logging::outputRoot(), _directory });
}

ResultStore* ResultStore::createAsynchronizedStore(const Mesh &mesh, const OutputConfiguration &configuration)
{
	if (_asyncStore != NULL) {
		ESINFO(GLOBAL_ERROR) << "ESPRESO internal error: try to create multiple ASYNC instances.";
	}


	_asyncStore = new ResultStore();
	_asyncStore->storeThreads = 0;
	_asyncStore->storeProcesses = 0;
	_asyncStore->computeProcesses = environment->MPIsize;

	_asyncStore->_direct = new DirectExecutor(mesh, configuration);
	ResultStoreExecutor *executor = _asyncStore->_direct;
	switch (configuration.mode) {
	case OutputConfiguration::MODE::SYNC:
		break;
	case OutputConfiguration::MODE::THREAD:
		_dispatcher = new async::Dispatcher();
		_asyncStore->_async = new AsyncStore(mesh, configuration);
		async::Config::setMode(async::THREAD);
		_asyncStore->storeThreads = 1;
		executor = _asyncStore->_async;
		break;
//	case OutputConfiguration::MODE::MPI:
//		ESINFO(GLOBAL_ERROR) << "ESPRESO internal error: not implemented OUTPUT::MODE==MPI.";
//		_dispatcher = new async::Dispatcher();
//		_asyncStore->_async = new AsyncStore(mesh, configuration);
//		if (environment->MPIsize == 1) {
//			ESINFO(GLOBAL_ERROR) << "Invalid number of MPI processes. OUTPUT::MODE==MPI required at least two MPI processes.";
//		}
//		if (configuration.output_node_group_size == 0) {
//			ESINFO(GLOBAL_ERROR) << "OUTPUT::OUTPUT_NODE_GROUP_SIZE cannot be 0.";
//		}
//		async::Config::setMode(async::MPI);
//		async::Config::setGroupSize(configuration.output_node_group_size);
//		_dispatcher->setGroupSize(configuration.output_node_group_size);
//		break;
	}

	// TODO: optimize
	if (configuration.results_store_frequency != OutputConfiguration::STORE_FREQUENCY::NEVER) {
		switch (configuration.format) {
		case OutputConfiguration::FORMAT::ENSIGHT:
			executor->addResultStore(new EnSightWithDecomposition(Logging::name, executor->mesh(), configuration));
			break;
		case OutputConfiguration::FORMAT::STL_SURFACE:
			executor->addResultStore(new STL(Logging::name, mesh, configuration));
			break;
		default:
			ESINFO(GLOBAL_ERROR) << "ESPRESO internal error: implement the selected output format.";
		}

	}
	if (configuration.monitors_store_frequency != OutputConfiguration::STORE_FREQUENCY::NEVER && configuration.monitoring.size()) {
		executor->addResultStore(new Monitoring(executor->mesh(), configuration, true));
	}
	if (configuration.catalyst) {
		_asyncStore->_direct->addResultStore(new InSitu(mesh, configuration));
	}
	if (configuration.debug) {
		_asyncStore->_direct->addResultStore(new VTKLegacyDebugInfo(mesh, configuration));
	}

	if (!_asyncStore->_direct->hasStore()) {
		delete _asyncStore->_direct;
		_asyncStore->_direct = NULL;
	}

	if (_asyncStore->_async != NULL && !_asyncStore->_async->hasStore()) {
		delete _asyncStore->_dispatcher;
		delete _asyncStore->_async;
		_asyncStore->_dispatcher = NULL;
		_asyncStore->_async = NULL;
	}

	if (_dispatcher != NULL) {
		_dispatcher->init();
		_dispatcher->dispatch();

//		if (configuration.mode == OutputConfiguration::MODE::MPI) {
//			int computeSize;
//			MPI_Comm_size(_dispatcher->commWorld(), &computeSize);
//			_asyncStore->computeProcesses = computeSize;
//			_asyncStore->storeProcesses = environment->MPIsize - computeSize;
//		}

		environment->MPICommunicator = _dispatcher->commWorld();
		MPI_Comm_rank(environment->MPICommunicator, &environment->MPIrank);
		MPI_Comm_size(environment->MPICommunicator, &environment->MPIsize);
	}

	return _asyncStore;
}

void ResultStore::destroyAsynchronizedStore()
{
	if (_asyncStore) {
		delete _asyncStore;
	}
	if (_dispatcher) {
		delete _dispatcher;
	}

	_asyncStore = NULL;
	_dispatcher = NULL;
}

bool ResultStore::isStoreNode()
{
	return _dispatcher != NULL && _dispatcher->isExecutor();
}

bool ResultStore::isComputeNode()
{
	return !isStoreNode();
}

bool ResultStore::isCollected()
{
	bool store = false;
	if (_async) store |= _async->isCollected();
	if (_direct) store |= _direct->isCollected();
	return store;
}

bool ResultStore::isSeparated()
{
	bool store = false;
	if (_async) store |= _async->isSeparated();
	if (_direct) store |= _direct->isSeparated();
	return store;
}

bool ResultStore::storeStep(const Step &step)
{
	bool store = false;
	if (_async) store |= _async->storeStep(step);
	if (_direct) store |= _direct->storeStep(step);
	return store;
}

void ResultStore::updateMesh()
{
	if (_async && _async->hasStore()) _async->updateMesh();
	if (_direct && _async->hasStore()) _direct->updateMesh();
}

void ResultStore::updateSolution(const Step &step)
{
	if (_async && storeStep(step)) _async->updateSolution(step);
	if (_direct && storeStep(step)) _direct->updateSolution(step);
}

ResultStore::ResultStore(): _async(NULL), _direct(NULL)
{

}

ResultStore::~ResultStore()
{
	if (_async) delete _async;
	if (_direct) delete _direct;
}


