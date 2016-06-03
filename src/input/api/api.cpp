
#include "api.h"

using namespace espreso::input;


void API::points(Coordinates &coordinates, size_t &DOFs)
{
	DOFs = _DOFs;
	Point p;

	eslocal max = 0;
	for (eslocal e = 0; e < _eIndices.size(); e++) {
		max = std::max(max, *std::max_element(_eIndices[e].begin(), _eIndices[e].end()));
	}
	max /= _DOFs;
	coordinates.reserve(max + 1);

	for (size_t i = 0; i <= max; i++) {
		coordinates.add(p, i, (_ids[i * _DOFs] - 1) / _DOFs);
	}
}

void API::elements(std::vector<Element*> &elements)
{
	elements.reserve(_eIndices.size());
	eslocal indices[20], params[6];

	for (eslocal e = 0; e < _eIndices.size(); e++) {
		for (eslocal i = 0; i < _eIndices[e].size(); i += _DOFs) {
			indices[i / _DOFs] = _eIndices[e][i] / _DOFs;
		}
		switch(_eIndices[e].size() / _DOFs) {
		case Tetrahedron4NodesCount:
			elements.push_back(new Tetrahedron4(indices, _eIndices[e].size() / _DOFs, params));
			break;
		case Tetrahedron10NodesCount:
			elements.push_back(new Tetrahedron10(indices, _eIndices[e].size() / _DOFs, params));
			break;
		case Pyramid5NodesCount:
			elements.push_back(new Pyramid5(indices, _eIndices[e].size() / _DOFs, params));
			break;
		case Pyramid13NodesCount:
			elements.push_back(new Pyramid13(indices, _eIndices[e].size() / _DOFs, params));
			break;
		case Prisma6NodesCount:
			elements.push_back(new Prisma6(indices, _eIndices[e].size() / _DOFs, params));
			break;
		case Prisma15NodesCount:
			elements.push_back(new Prisma15(indices, _eIndices[e].size() / _DOFs, params));
			break;
		case Hexahedron8NodesCount:
			elements.push_back(new Hexahedron8(indices, _eIndices[e].size() / _DOFs, params));
			break;
		case Hexahedron20NodesCount:
			elements.push_back(new Hexahedron20(indices, _eIndices[e].size() / _DOFs, params));
			break;
		default:
			ESINFO(ERROR) << "Unknown element with " << _eIndices[e].size() / _DOFs << " indices.";
		}
	}
}

void API::clusterBoundaries(Boundaries &boundaries, std::vector<int> &neighbours)
{
	// TODO: check neighbours correctness

	std::vector<esglobal> sBuffer;
	std::vector<std::vector<esglobal> > rBuffer(_neighbours.size());
	std::vector<MPI_Request> req(2 * _neighbours.size());
	std::vector<size_t> sizes(_neighbours.size());

	_size /= _DOFs;

	for (size_t n = 0; n < _neighbours.size(); n++) {
		MPI_Isend(&_size,           sizeof(size_t), MPI_BYTE, _neighbours[n], 0, MPI_COMM_WORLD, req.data() + 2 * n);
		MPI_Irecv(sizes.data() + n, sizeof(size_t), MPI_BYTE, _neighbours[n], 0, MPI_COMM_WORLD, req.data() + 2 * n + 1);
	}
	MPI_Waitall(2 * _neighbours.size(), req.data(), MPI_STATUSES_IGNORE);

	sBuffer.reserve(_size);
	for (size_t i = 0; i < _size; i++) {
		sBuffer.push_back(_ids[i * _DOFs] / _DOFs);
	}
	std::sort(sBuffer.begin(), sBuffer.end());

	for (size_t n = 0; n < _neighbours.size(); n++) {
		if (_neighbours[n] != config::env::MPIrank) {
			rBuffer[n].resize(sizes[n]);
		}
	}

	size_t rCounter = 0;
	for (size_t n = 0; n < _neighbours.size(); n++) {
		if (_neighbours[n] != config::env::MPIrank) {
			MPI_Isend(sBuffer.data(),       _size * sizeof(esglobal), MPI_BYTE, _neighbours[n], 0, MPI_COMM_WORLD, req.data() + rCounter++);
			MPI_Irecv(rBuffer[n].data(), sizes[n] * sizeof(esglobal), MPI_BYTE, _neighbours[n], 0, MPI_COMM_WORLD, req.data() + rCounter++);
		}
	}
	MPI_Waitall(rCounter, req.data(), MPI_STATUSES_IGNORE);

	size_t threads = Esutils::getEnv<size_t>("CILK_NWORKERS");
	std::vector<size_t> distribution = Esutils::getDistribution(threads, _size);
	std::vector<std::set<int> > realNeighbour(threads);

	size_t pushMyRank = std::lower_bound(_neighbours.begin(), _neighbours.end(), config::env::MPIrank) - _neighbours.begin();

	boundaries.resize(_size);
	#pragma cilk grainsize = 1
	cilk_for (size_t t = 0; t < threads; t++) {
		for (size_t i = distribution[t]; i < distribution[t + 1]; i++) {

			for (size_t n = 0; n < _neighbours.size(); n++) {
				if (n == pushMyRank) {
					boundaries[i].push_back(config::env::MPIrank);
				}
				auto it = std::lower_bound(rBuffer[n].begin(), rBuffer[n].end(), _ids[i * _DOFs] / _DOFs);
				if (it != rBuffer[n].end() && *it == _ids[i * _DOFs] / _DOFs) {
					boundaries[i].push_back(_neighbours[n]);
					realNeighbour[t].insert(_neighbours[n]);
				}
			}
			if (_neighbours.size() == pushMyRank) {
				boundaries[i].push_back(config::env::MPIrank);
			}

		}
	}

	for (size_t t = 1; t < threads; t++) {
		realNeighbour[0].insert(realNeighbour[t].begin(), realNeighbour[t].end());
	}

	neighbours = std::vector<int>(realNeighbour[0].begin(), realNeighbour[0].end());
}