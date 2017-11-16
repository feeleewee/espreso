
#ifndef SRC_MESH_STORE_STORE_H_
#define SRC_MESH_STORE_STORE_H_

#include <cstddef>
#include <string>
#include <vector>
#include <fstream>

#include "../../basis/utilities/communication.h"

namespace espreso {

template <typename TEBoundaries, typename TEData> class serializededata;

struct Store {

	template <typename TBoundaries, typename TData>
	static void storedata(std::ofstream &os, const std::string &head, const serializededata<TBoundaries, TData> *data)
	{
		if (data == NULL) {
			return;
		}

		os << head << "\n";
		for (auto elem = data->begin(); elem != data->end(); ++elem) {
			os << "[ ";
			for (auto i = elem->begin(); i != elem->end(); ++i) {
				os << *i << " ";
			}
			os << "] ";
		}
		os << "\n";
	}

	static std::vector<eslocal> gatherDistribution(eslocal size)
	{
		std::vector<eslocal> result(environment->MPIsize + 1);
		eslocal esize = size;
		Communication::exscan(esize);

		MPI_Allgather(&esize, sizeof(eslocal), MPI_BYTE, result.data(), sizeof(eslocal), MPI_BYTE, environment->MPICommunicator);
		result.back() = esize + size;
		MPI_Bcast(&result.back(), sizeof(eslocal), MPI_BYTE, environment->MPIsize - 1, environment->MPICommunicator);

		return result;
	}
};

}


#endif /* SRC_MESH_STORE_STORE_H_ */
