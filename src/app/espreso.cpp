
#include <signal.h>
#include <csignal>

#include "factory/factory.h"

using namespace espreso;

static void signalHandler(int signal)
{
	switch (signal) {
	case SIGSEGV:
		ESINFO(ERROR) << "Invalid memory reference";
		break;
	case SIGFPE:
		ESINFO(ERROR) << "Erroneous arithmetic operation";
		break;
	}
}


int main(int argc, char **argv)
{
	std::signal(SIGFPE, signalHandler);
	std::signal(SIGSEGV, signalHandler);

	MPI_Init(&argc, &argv);

	Factory factory(ParametersReader::arguments(&argc, &argv));

	factory.solve();
	factory.store("result");

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	return 0;
}

