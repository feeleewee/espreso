
#include "mpi.h"

#include "esinput.h"
#include "esoutput.h"
#include "esmesh.h"
#include "factory/factory.h"

using namespace espreso;

int main(int argc, char** argv)
{
	if (argc < 4) {
		ESINFO(GLOBAL_ERROR) << "Specify parameters: INPUT_LOCATION  OUTPUT_LOCATION  [ NUMBER_OF_PARTS ]";
	}

	MPI_Init(&argc, &argv);

	ArgsConfiguration configuration;
	configuration.path = argv[1];
	for (int i = 2; i < argc; i++) {
		configuration.nameless.push_back(argv[i]);
	}

	if (environment->MPIsize > 1) {
		ESINFO(GLOBAL_ERROR) << "Not implemented decomposition of ESDATA";
		config::mesh::INPUT = config::mesh::INPUTalternative::ESDATA;
	} else {
		config::mesh::INPUT = config::mesh::INPUTalternative::WORKBENCH;
	}
	config::mesh::SUBDOMAINS = 1;
	config::info::VERBOSE_LEVEL = 2;
	config::info::MEASURE_LEVEL = 2;

	Factory factory(configuration);
	std::cout << "Mesh loaded\n";

	for (size_t i = 1; i < configuration.nameless.size(); i++) {
		int parts = atoi(configuration.nameless[i].c_str());
		std::stringstream ss;
		ss << configuration.nameless[0] << parts * environment->MPIsize;

		factory.mesh.partitiate(parts);
		std::cout << "Mesh partitiated to " << parts * environment->MPIsize << " parts\n";
		std::vector<size_t> sizes(factory.mesh.parts());
		for (size_t p = 0; p < factory.mesh.parts(); p++) {
			sizes[p] = factory.mesh.coordinates().localSize(p);
		}
		std::cout << "Nodes in subdomains: " << Info::averageValues(sizes) << "\n";
		output::Esdata::mesh(factory.mesh, ss.str());
		std::cout << "Mesh partitiated to " << parts * environment->MPIsize << " parts saved\n";
	}

	MPI_Finalize();
}


