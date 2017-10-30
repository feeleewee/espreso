
#include "transformations.h"

#include "../newmesh.h"
#include "../elements/newelement.h"
#include "../elements/elementstore.h"
#include "../store/domainstore.h"
#include "../store/boundarystore.h"

#include "../../basis/containers/serializededata.h"
#include "../../basis/logging/logging.h"
#include "../../basis/utilities/utils.h"
#include "../../config/ecf/environment.h"

#include <numeric>
#include <algorithm>
#include <iostream>

#include "../../basis/utilities/communication.h"

using namespace espreso;

void Transformation::computeProcessBoundaries(NewMesh &mesh)
{
	ESINFO(TVERBOSITY) << std::string(2 * level++, ' ') << "MESH::computation of process boundaries started.";

	if (mesh._nodes->elems == NULL) {
		Transformation::addLinkFromTo(mesh, TFlags::ELEVEL::NODE, TFlags::ELEVEL::ELEMENT);
	}

	if (mesh._elems->dual == NULL) {
		Transformation::computeDual(mesh);
	}

	std::vector<esglobal> IDBoundaries = mesh._elems->gatherSizes();
	esglobal begine = IDBoundaries[environment->MPIrank];
	esglobal ende   = IDBoundaries[environment->MPIrank + 1];

	size_t threads = environment->OMP_NUM_THREADS;

	std::vector<std::vector<eslocal> > elementData(threads), faceDistribution(threads), faceData(threads);
	std::vector<std::vector<NewElement*> > faceCodes(threads);
	std::vector<std::vector<int> > faceNeighbors(threads);

	faceDistribution.front().push_back(0);
	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		std::vector<esglobal> common;
		size_t ncommons, counter, neighbor;
		auto dual = mesh._elems->dual->cbegin(t);
		auto epointer = mesh._elems->epointers->cbegin(t);
		esglobal eID = mesh._elems->distribution[t];

		for (auto e = mesh._elems->nodes->cbegin(t); e != mesh._elems->nodes->cend(t); ++e, ++dual, ++epointer, ++eID) {
			if (dual->size() < epointer->front()->faces->structures() || dual->front() < begine || dual->back() >= ende) {

				auto facepointer = epointer->front()->facepointers->cbegin(t);
				for (auto face = epointer->front()->faces->cbegin(t); face != epointer->front()->faces->cend(t); ++face, ++facepointer) {

					neighbor = -1;
					common.clear();
					for (auto n = face->begin(); n != face->end(); ++n) {
						auto nelements = mesh._nodes->elems->cbegin() + (*e)[*n];
						for (auto ne = nelements->begin(); ne != nelements->end(); ++ne) {
							common.push_back(*ne);
						}
					}
					std::sort(common.begin(), common.end());

					ncommons = counter = 0;
					for (size_t i = 1; i < common.size(); i++) {
						if (common[i - 1] == common[i]) {
							++counter;
						} else {
							if (face->size() == counter + 1) {
								if (begine <= common[i - 1] && common[i - 1] < ende) {
									++ncommons;
								} else {
									neighbor = std::lower_bound(IDBoundaries.begin(), IDBoundaries.end(), common[i - 1] + 1) - IDBoundaries.begin() - 1;
								}
							}
							counter = 0;
						}
					}
					if (face->size() == counter + 1) {
						if (begine <= common.back() && common.back() < ende) {
							++ncommons;
						} else {
							neighbor = std::lower_bound(IDBoundaries.begin(), IDBoundaries.end(), common.back() + 1) - IDBoundaries.begin() - 1;
						}
					}

					if (ncommons == 1) {
						faceDistribution[t].push_back(face->size());
						faceCodes[t].push_back(facepointer->front());
						if (faceDistribution[t].size() > 1) {
							faceDistribution[t].back() += *(faceDistribution[t].end() - 2);
						}
						for (auto n = face->begin(); n != face->end(); ++n) {
							faceData[t].push_back((*e)[*n]);
						}
						faceNeighbors[t].push_back(neighbor);
						if (neighbor != -1 && (elementData[t].size() == 0 || elementData[t].back() != eID)) {
							elementData[t].push_back(eID);
						}
					}
				}
			}
		}
	}

	// Distribute elements to intervals
	serializededata<eslocal, eslocal>::balance(1, elementData);
	mesh._processBoundaries->elems = new serializededata<eslocal, eslocal>(1, elementData);
	std::vector<eslocal> permutation(mesh._processBoundaries->elems->structures());
	std::iota(permutation.begin(), permutation.end(), 0);

	auto moveToNextNeigh = [&] (const eslocal* &ne, const eslocal *end) {
		while (++ne != end) {
			int neigh = std::lower_bound(IDBoundaries.begin(), IDBoundaries.end(), *ne + 1) - IDBoundaries.begin() - 1;
			if (neigh != environment->MPIrank) {
				return neigh;
			}
		}
		return -1;
	};
	std::sort(permutation.begin(), permutation.end(), [&] (eslocal i, eslocal j) {
		auto di = mesh._elems->dual->cbegin() + mesh._processBoundaries->elems->datatarray()[i];
		auto dj = mesh._elems->dual->cbegin() + mesh._processBoundaries->elems->datatarray()[j];

		auto diit = di->begin();
		auto djit = dj->begin();
		int ni = moveToNextNeigh(--diit, di->end());
		int nj = moveToNextNeigh(--djit, dj->end());
		while (diit != di->end() && djit != dj->end()) {
			if (ni == -1 && nj == -1) {
				break;
			}
			if (ni == -1) {
				return true;
			}
			if (nj == -1) {
				return false;
			}
			if (ni != nj) {
				return ni < nj;
			}
			ni = moveToNextNeigh(diit, di->end());
			nj = moveToNextNeigh(djit, dj->end());
		}
		return mesh._processBoundaries->elems->datatarray()[i] < mesh._processBoundaries->elems->datatarray()[j];
	});

	mesh._processBoundaries->elems->permute(permutation);

	// Distribute faces to intervals
	for (size_t t = 1; t < threads; t++) {
		faceNeighbors[0].insert(faceNeighbors[0].end(), faceNeighbors[t].begin(), faceNeighbors[t].end());
	}
	Esutils::threadDistributionToFullDistribution(faceDistribution);
	mesh._processBoundaries->faces = new serializededata<eslocal, eslocal>(faceDistribution, faceData);
	mesh._processBoundaries->facepointers = new serializededata<eslocal, NewElement*>(1, faceCodes);
	permutation.resize(mesh._processBoundaries->faces->structures());
	std::vector<size_t> fdistribution = tarray<eslocal>::distribute(threads, permutation.size());
	std::iota(permutation.begin(), permutation.end(), 0);
	std::sort(permutation.begin(), permutation.end(), [&] (eslocal i, eslocal j) { return faceNeighbors[0][i] < faceNeighbors[0][j]; });
	mesh._processBoundaries->faces->permute(permutation, &fdistribution);

	std::vector<std::vector<BoundaryInterval> > fintervals(threads);
	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		for (size_t i = fdistribution[t]; i < fdistribution[t + 1]; ++i) {
			if (i > fdistribution[t] && faceNeighbors[0][permutation[i - 1]] == faceNeighbors[0][permutation[i]]) {
				++fintervals[t].back().end;
			} else {
				fintervals[t].push_back(BoundaryInterval(i, i + 1, { faceNeighbors[0][permutation[i]] }));
			}
		}
	}

	for (size_t t = 1; t < threads; t++) {
		if (fintervals[t].size() && fintervals[0].back().neighbors.front() == fintervals[t].front().neighbors.front()) {
			fintervals[0].back().end = fintervals[t].front().end;
			fintervals[0].insert(fintervals[0].end(), fintervals[t].begin() + 1, fintervals[t].end());
		} else {
			fintervals[0].insert(fintervals[0].end(), fintervals[t].begin(), fintervals[t].end());
		}
	}
	mesh._processBoundaries->facesIntervals = fintervals[0];


	// Distribute nodes to interval
	std::vector<std::vector<eslocal> > bnodes(mesh._processBoundaries->facesIntervals.size() + 1);

	#pragma omp parallel for
	for (size_t i = 0; i < mesh._processBoundaries->facesIntervals.size() + 1; ++i) {
		if (i == bnodes.size() - 1) {
			auto begin = mesh._processBoundaries->faces->datatarray().begin();
			auto end   = mesh._processBoundaries->faces->datatarray().end();
			bnodes[i].insert(bnodes[i].end(), begin, end);
			Esutils::sortAndRemoveDuplicity(bnodes[i]);
		} else {
			auto begin = (mesh._processBoundaries->faces->cbegin() + mesh._processBoundaries->facesIntervals[i].begin)->begin();
			auto end   = (mesh._processBoundaries->faces->cbegin() + mesh._processBoundaries->facesIntervals[i].end)->begin();
			bnodes[i].insert(bnodes[i].end(), begin, end);
			Esutils::sortAndRemoveDuplicity(bnodes[i]);
		}
	}

	std::vector<std::vector<eslocal> > nodeNeighDistribution(threads), nodeNeighData(threads);
	std::vector<size_t> ndistribution = tarray<eslocal>::distribute(threads, bnodes.back().size());

	nodeNeighDistribution.front().push_back(0);
	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		size_t offset = 0;
		for (size_t n = ndistribution[t]; n < ndistribution[t + 1]; ++n) {
			for (size_t i = 0; i < mesh._processBoundaries->facesIntervals.size(); ++i) {
				if (std::binary_search(bnodes[i].begin(), bnodes[i].end(), bnodes.back()[n])) {
					++offset;
					nodeNeighData[t].push_back(mesh._processBoundaries->facesIntervals[i].neighbors.front());
				}
			}
			nodeNeighDistribution[t].push_back(offset);
		}
	}

	Esutils::threadDistributionToFullDistribution(nodeNeighDistribution);
	serializededata<eslocal, eslocal> nodesNeigh(nodeNeighDistribution, nodeNeighData);

	permutation.resize(nodesNeigh.structures());
	std::iota(permutation.begin(), permutation.end(), 0);
	std::sort(permutation.begin(), permutation.end(), [&] (eslocal i, eslocal j) {
		auto ni = nodesNeigh.cbegin() + i;
		auto nj = nodesNeigh.cbegin() + j;
		for (size_t n = 0; n < ni->size() && n < nj->size(); ++n) {
			if ((*ni)[n] != (*nj)[n]) {
				return (*ni)[n] < (*nj)[n];
			}
		}
		if (ni->size() != nj->size()) {
			return ni->size() < nj->size();
		}
		return bnodes.back()[i] < bnodes.back()[j];
	});

	std::vector<std::vector<eslocal> > nodesData(threads);
	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		for (size_t i = ndistribution[t]; i < ndistribution[t + 1]; ++i) {
			nodesData[t].push_back(bnodes.back()[permutation[i]]);
		}
	}

	mesh._processBoundaries->nodes = new serializededata<eslocal, eslocal>(1, nodesData);

	std::vector<eslocal> backpermutation(permutation.size());
	std::iota(backpermutation.begin(), backpermutation.end(), 0);
	std::sort(backpermutation.begin(), backpermutation.end(), [&] (eslocal i, eslocal j) { return permutation[i] < permutation[j]; });

	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		for (auto n = mesh._processBoundaries->faces->begin(t)->begin(); n != mesh._processBoundaries->faces->end(t)->begin(); ++n) {
			*n = backpermutation[std::lower_bound(bnodes.back().begin(), bnodes.back().end(), *n) - bnodes.back().begin()];
		}
	}

	auto equalNeighs = [] (serializededata<eslocal, eslocal>::const_iterator &i, serializededata<eslocal, eslocal>::const_iterator j) {
		if (i->size() != j->size()) {
			return false;
		}
		for (size_t n = 0; n < i->size(); ++n) {
			if ((*i)[n] != (*j)[n]) {
				return false;
			}
		}
		return true;
	};

	std::vector<std::vector<BoundaryInterval> > nintervals(threads);
	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		for (size_t i = ndistribution[t]; i < ndistribution[t + 1]; ++i) {
			auto neighs = nodesNeigh.cbegin() + permutation[i];
			if (i > ndistribution[t] && equalNeighs(neighs, nodesNeigh.cbegin() + permutation[i - 1])) {
				++nintervals[t].back().end;
			} else {
				nintervals[t].push_back(BoundaryInterval(i, i + 1, std::vector<int>(neighs->begin(), neighs->end())));
			}
		}
	}

	for (size_t t = 1; t < threads; t++) {
		if (nintervals[t].size() && nintervals[0].back().neighbors == nintervals[t].front().neighbors) {
			nintervals[0].back().end = nintervals[t].front().end;
			nintervals[0].insert(nintervals[0].end(), nintervals[t].begin() + 1, nintervals[t].end());
		} else {
			nintervals[0].insert(nintervals[0].end(), nintervals[t].begin(), nintervals[t].end());
		}
	}
	mesh._processBoundaries->nodesIntervals = nintervals[0];

	ESINFO(TVERBOSITY) << std::string(--level * 2, ' ') << "MESH::computation of process boundaries finished.";
}

void Transformation::computeDomainsBoundaries(NewMesh &mesh)
{
	if (mesh._domains == NULL) {
		ESINFO(TVERBOSITY) << std::string(2 * (level + 1), ' ') << "MESH::computation of domains boundaries skipped. There are no domains.";
		return;
	}

	ESINFO(TVERBOSITY) << std::string(2 * level++, ' ') << "MESH::computation of domains boundaries started.";

	if (mesh._nodes->elems == NULL) {
		Transformation::addLinkFromTo(mesh, TFlags::ELEVEL::NODE, TFlags::ELEVEL::ELEMENT);
	}

	if (mesh._elems->decomposedDual == NULL) {
		Transformation::computeDecomposedDual(mesh, TFlags::SEPARATE::ETYPES);
	}

	std::vector<esglobal> IDBoundaries = mesh._elems->gatherSizes();

	size_t threads = environment->OMP_NUM_THREADS;
	std::vector<std::vector<eslocal> > faceDistribution(threads), faceData(threads), faceNodes(threads);
	std::vector<std::vector<NewElement*> > faceCodes(threads);

	faceDistribution.front().push_back(0);
	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		std::vector<esglobal> common;
		size_t ncommons, counter;
		bool isAdept;

		for (size_t d = mesh._domains->domainDistribution[t]; d < mesh._domains->domainDistribution[t + 1]; ++d) {
			esglobal begine = IDBoundaries[environment->MPIrank] + mesh._domains->domainElementBoundaries[d];
			esglobal ende   = IDBoundaries[environment->MPIrank] + mesh._domains->domainElementBoundaries[d + 1];
			auto dual = mesh._elems->decomposedDual->cbegin() + mesh._domains->domainElementBoundaries[d];
			auto epointer = mesh._elems->epointers->cbegin() + mesh._domains->domainElementBoundaries[d];

			for (
					auto e = mesh._elems->nodes->cbegin() + mesh._domains->domainElementBoundaries[d];
					e != mesh._elems->nodes->cbegin() + mesh._domains->domainElementBoundaries[d + 1];
					++e, ++dual, ++epointer) {

				isAdept = false;
				if (dual->size() < epointer->front()->faces->structures()) {
					isAdept = true;
				} else {
					for (auto ne = dual->begin(); ne != dual->end(); ++ne) {
						if (*ne < begine || ende <= *ne) {
							isAdept = true;
							break;
						}
					}
				}

				if (isAdept) {
					auto facepointer = epointer->front()->facepointers->cbegin(t);
					for (auto face = epointer->front()->faces->cbegin(t); face != epointer->front()->faces->cend(t); ++face, ++facepointer) {

						common.clear();
						for (auto n = face->begin(); n != face->end(); ++n) {
							auto nelements = mesh._nodes->elems->cbegin() + (*e)[*n];
							for (auto ne = nelements->begin(); ne != nelements->end(); ++ne) {
								common.push_back(*ne);
							}

						}
						std::sort(common.begin(), common.end());

						ncommons = counter = 0;
						for (size_t i = 1; i < common.size(); i++) {
							if (common[i - 1] == common[i]) {
								++counter;
							} else {
								if (face->size() == counter + 1) {
									if (begine <= common[i - 1] && common[i - 1] < ende) {
										++ncommons;
									}
								}
								counter = 0;
							}
						}
						if (face->size() == counter + 1) {
							if (begine <= common.back() && common.back() < ende) {
								++ncommons;
							}
						}

						if (ncommons == 1) {
							faceDistribution[t].push_back(face->size());
							faceCodes[t].push_back(facepointer->front());
							if (faceDistribution[t].size() > 1) {
								faceDistribution[t].back() += *(faceDistribution[t].end() - 2);
							}
							for (auto n = face->begin(); n != face->end(); ++n) {
								faceData[t].push_back((*e)[*n]);
							}
						}
					}
				}
			}
		}
		faceNodes[t] = faceData[t];
		Esutils::sortAndRemoveDuplicity(faceNodes[t]);
	}
	Esutils::mergeThreadedUniqueData(faceNodes);
	faceNodes.resize(1);
	faceNodes.resize(threads);

	Esutils::threadDistributionToFullDistribution(faceDistribution);

	mesh._domainsBoundaries->faces = new serializededata<eslocal, eslocal>(faceDistribution, faceData);
	mesh._domainsBoundaries->facepointers = new serializededata<eslocal, NewElement*>(1, faceCodes);
	mesh._domainsBoundaries->nodes = new serializededata<eslocal, eslocal>(1, faceNodes);

	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		for (auto n = mesh._domainsBoundaries->faces->begin(t)->begin(); n != mesh._domainsBoundaries->faces->end(t)->begin(); ++n) {
			*n = std::lower_bound(faceNodes[0].begin(), faceNodes[0].end(), *n) - faceNodes[0].begin();
		}
	}

	ESINFO(TVERBOSITY) << std::string(--level * 2, ' ') << "MESH::computation of domains boundaries finished.";
}

