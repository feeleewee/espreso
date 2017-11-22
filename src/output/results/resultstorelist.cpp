
#include "resultstorelist.h"

#include "../../config/ecf/environment.h"
#include "../../config/ecf/output.h"
#include "../../basis/logging/logging.h"

#include "executor/directexecutor.h"
#include "executor/asyncexecutor.h"

#include "async/Dispatcher.h"

using namespace espreso;

ResultStoreList* ResultStoreList::_resultStoreList = NULL;
async::Dispatcher* ResultStoreList::_dispatcher = NULL;

ResultStoreList* ResultStoreList::createAsynchronizedStore(const OutputConfiguration &configuration)
{
	if (_resultStoreList != NULL) {
		ESINFO(GLOBAL_ERROR) << "ESPRESO internal error: try to create multiple ASYNC instances.";
	}
	_resultStoreList = new ResultStoreList();
	_dispatcher = new async::Dispatcher();

	switch (configuration.mode) {
	case OutputConfiguration::MODE::SYNC:
		std::cout << "SYNC\n";
		_resultStoreList->add(new DirectExecutor(configuration));
		async::Config::setMode(async::SYNC);
		break;
	case OutputConfiguration::MODE::THREAD:
		_resultStoreList->add(new AsyncStore(configuration));
		async::Config::setMode(async::THREAD);
		break;
	case OutputConfiguration::MODE::MPI:
		_resultStoreList->add(new AsyncStore(configuration));
		if (environment->MPIsize == 1) {
			ESINFO(GLOBAL_ERROR) << "Invalid number of MPI processes. OUTPUT::MODE==MPI required at least two MPI processes.";
		}
		if (configuration.output_node_group_size == 0) {
			ESINFO(GLOBAL_ERROR) << "OUTPUT::OUTPUT_NODE_GROUP_SIZE cannot be 0.";
		}
		async::Config::setMode(async::MPI);
		async::Config::setGroupSize(configuration.output_node_group_size);
		_dispatcher->setGroupSize(configuration.output_node_group_size);
		break;
	}

	_dispatcher->init();
	_dispatcher->dispatch();
//
//	if (isComputeNode()) {
//		if (configuration.monitoring.size()) {
//			_resultStoreList->_results.push_back(new Monitoring(configuration, mesh));
//		}
//	}

	if (configuration.mode == OutputConfiguration::MODE::MPI) {
		int computeSize;
		MPI_Comm_size(_dispatcher->commWorld(), &computeSize);
		ESINFO(OVERVIEW) << "Reserve " << environment->MPIsize - computeSize << " MPI process(es) for storing solution.";
	}

	environment->MPICommunicator = _dispatcher->commWorld();
	MPI_Comm_rank(environment->MPICommunicator, &environment->MPIrank);
	MPI_Comm_size(environment->MPICommunicator, &environment->MPIsize);

	return _resultStoreList;
}

void ResultStoreList::destroyAsynchronizedStore()
{
	delete _resultStoreList;
	delete _dispatcher;

	_resultStoreList = NULL;
	_dispatcher = NULL;
}

bool ResultStoreList::isStoreNode()
{
	return _dispatcher == NULL || _dispatcher->isExecutor();
}

bool ResultStoreList::isComputeNode()
{
	return !isStoreNode();
}



