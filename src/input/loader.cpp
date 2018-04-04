
#include "loader.h"
#include "workbench/workbench.h"

#include "../basis/containers/point.h"
#include "../basis/containers/serializededata.h"
#include "../basis/utilities/utils.h"
#include "../basis/utilities/communication.h"
#include "../basis/logging/timeeval.h"

#include "../config/ecf/root.h"

#include "../mesh/mesh.h"
#include "../mesh/preprocessing/meshpreprocessing.h"
#include "../mesh/elements/element.h"
#include "../mesh/store/nodestore.h"
#include "../mesh/store/elementstore.h"
#include "../mesh/store/elementsregionstore.h"
#include "../mesh/store/boundaryregionstore.h"
#include "../old/input/loader.h"

#include <numeric>
#include <algorithm>
#include <fstream>

using namespace espreso;

void Loader::load(const ECFRoot &configuration, Mesh &mesh, int MPIrank, int MPIsize)
{
	switch (configuration.input) {
	case INPUT_FORMAT::WORKBENCH:
		WorkbenchLoader::load(configuration, mesh);
		mesh.update();
		break;
	default:
		input::OldLoader::load(configuration, *mesh.mesh, MPIrank, MPIsize);
		mesh.load();
		break;
	}
}

void Loader::loadDistributedMesh(const ECFRoot &configuration, DistributedMesh &dMesh, Mesh &mesh)
{
	Loader(configuration, dMesh, mesh);
}

Loader::Loader(const ECFRoot &configuration, DistributedMesh &dMesh, Mesh &mesh)
: _configuration(configuration), _dMesh(dMesh), _mesh(mesh)
{
	ESINFO(OVERVIEW) << "Balance distributed mesh.";
	TimeEval timing("Load distributed mesh");
	timing.totalTime.startWithBarrier();

	TimeEvent tdistribution("distribute mesh across processes"); tdistribution.start();
	distributeMesh();
	checkERegions();
	tdistribution.end(); timing.addEvent(tdistribution);
	ESINFO(PROGRESS2) << "Distributed loader:: data balanced.";

	TimeEvent telements("fill elements"); telements.start();
	fillElements();
	telements.end(); timing.addEvent(telements);
	ESINFO(PROGRESS2) << "Distributed loader:: elements filled.";

	TimeEvent tcoordinates("fill coordinates"); tcoordinates.start();
	fillCoordinates();
	tcoordinates.end(); timing.addEvent(tcoordinates);
	ESINFO(PROGRESS2) << "Distributed loader:: coordinates filled.";

	TimeEvent tnregions("fill node regions"); tnregions.start();
	addNodeRegions();
	tnregions.end(); timing.addEvent(tnregions);
	ESINFO(PROGRESS2) << "Distributed loader:: node regions filled.";

	TimeEvent tbregions("fill boundary regions"); tbregions.start();
	addBoundaryRegions();
	tbregions.end(); timing.addEvent(tbregions);
	ESINFO(PROGRESS2) << "Distributed loader:: boundary regions filled.";

	TimeEvent teregions("fill element regions"); teregions.start();
	addElementRegions();
	teregions.end(); timing.addEvent(teregions);
	ESINFO(PROGRESS2) << "Distributed loader:: elements regions filled.";

	timing.totalTime.endWithBarrier();
	timing.printStatsMPI();
//	exit(0);
}

void Loader::distributeMesh()
{
	TimeEval timing("MESH DISTRIBUTION");
	timing.totalTime.startWithBarrier();

	// DISTRIBUTE NODES
	eslocal myMaxID = 0, maxID;
	int sorted = std::is_sorted(_dMesh.nIDs.begin(), _dMesh.nIDs.end()), allSorted;
	std::vector<eslocal> permutation;
	if (_dMesh.nIDs.size()) {
		if (sorted) {
			myMaxID = _dMesh.nIDs.back();
		} else {
			permutation.resize(_dMesh.nIDs.size());
			std::iota(permutation.begin(), permutation.end(), 0);
			std::sort(permutation.begin(), permutation.end(), [&] (eslocal i, eslocal j) { return _dMesh.nIDs[i] < _dMesh.nIDs[j]; });
			myMaxID = _dMesh.nIDs[permutation.back()];
		}
	}

	MPI_Allreduce(&myMaxID, &maxID, sizeof(eslocal), MPI_BYTE, MPITools::operations().max, environment->MPICommunicator);
	MPI_Allreduce(&sorted, &allSorted, 1, MPI_INT, MPI_MIN, environment->MPICommunicator);

	if (allSorted) {
		if (environment->MPIsize == 1) {
			_nDistribution = { 0, _dMesh.nIDs.size() };
		} else {
			std::vector<size_t> cCurrent = Communication::getDistribution(_dMesh.nIDs.size(), MPITools::operations().sizeToOffsetsSize_t);
			_nDistribution = tarray<eslocal>::distribute(environment->MPIsize, cCurrent.back());

			TimeEvent e1("BALANCE IDS"); e1.start();
			if (!Communication::balance(_dMesh.nIDs, cCurrent, _nDistribution)) {
				ESINFO(ERROR) << "ESPRESO internal error: balance node IDs.";
			}
			e1.end(); timing.addEvent(e1);

			TimeEvent e2("BALANCE COORDINATES"); e2.start();
			if (!Communication::balance(_dMesh.coordinates, cCurrent, _nDistribution)) {
				ESINFO(ERROR) << "ESPRESO internal error: balance coordinates.";
			}
			e2.end(); timing.addEvent(e2);
		}
	} else {
		if (environment->MPIsize == 1) {
			_nDistribution = { 0, _dMesh.nIDs.size() };
			std::vector<eslocal> nIDs;
			std::vector<Point> nPoints;
			nIDs.reserve(_dMesh.nIDs.size());
			nPoints.reserve(_dMesh.nIDs.size());
			for (size_t i = 0; i < permutation.size(); i++) {
				nIDs.push_back(_dMesh.nIDs[permutation[i]]);
				nPoints.push_back(_dMesh.coordinates[permutation[i]]);
			}
			_dMesh.nIDs.swap(nIDs);
			_dMesh.coordinates.swap(nPoints);
		} else {
			_nDistribution = tarray<eslocal>::distribute(environment->MPIsize, maxID + 1);
			std::vector<std::vector<eslocal> > sIDs, rIDs;
			std::vector<std::vector<Point> > sCoordinates, rCoordinates;
			std::vector<int> targets;
			for (int r = 0; r < environment->MPIsize; r++) {
				auto begin = std::lower_bound(permutation.begin(), permutation.end(), _nDistribution[r], [&] (eslocal i, const size_t &ID) { return _dMesh.nIDs[i] < ID; });
				auto end = std::lower_bound(permutation.begin(), permutation.end(), _nDistribution[r + 1], [&] (eslocal i, const size_t &ID) { return _dMesh.nIDs[i] < ID; });
				if (begin != end) {
					sIDs.push_back({});
					sCoordinates.push_back({});
					targets.push_back(r);
				}
				for (size_t n = begin - permutation.begin(); n < end - permutation.begin(); ++n) {
					sIDs.back().push_back(_dMesh.nIDs[permutation[n]]);
					sCoordinates.back().push_back(_dMesh.coordinates[permutation[n]]);
				}
			}

			TimeEvent e2("VARIOUS IDS"); e2.start();
			if (!Communication::sendVariousTargets(sIDs, rIDs, targets)) {
				ESINFO(ERROR) << "ESPRESO internal error: distribute not sorted node IDs.";
			}
			e2.end(); timing.addEvent(e2);

			TimeEvent e3("VARIOUS COORDINATES"); e3.start();
			if (!Communication::sendVariousTargets(sCoordinates, rCoordinates, targets)) {
				ESINFO(ERROR) << "ESPRESO internal error: distribute not sorted node coordinates.";
			}
			e3.end(); timing.addEvent(e3);

			for (size_t r = 1; r < rIDs.size(); r++) {
				rIDs[0].insert(rIDs[0].end(), rIDs[r].begin(), rIDs[r].end());
				rCoordinates[0].insert(rCoordinates[0].end(), rCoordinates[r].begin(), rCoordinates[r].end());
			}

			permutation.resize(rIDs[0].size());
			std::iota(permutation.begin(), permutation.end(), 0);
			std::sort(permutation.begin(), permutation.end(), [&] (eslocal i, eslocal j) { return rIDs[0][i] < rIDs[0][j]; });
			_dMesh.nIDs.clear();
			_dMesh.coordinates.clear();
			_dMesh.nIDs.reserve(permutation.size());
			_dMesh.coordinates.reserve(permutation.size());
			for (size_t n = 0; n < permutation.size(); n++) {
				_dMesh.nIDs.push_back(rIDs[0][permutation[n]]);
				_dMesh.coordinates.push_back(rCoordinates[0][permutation[n]]);
			}
		}
	}

	// DISTRIBUTE ELEMENTS

	myMaxID = 0;
	sorted = std::is_sorted(_dMesh.edata.begin(), _dMesh.edata.end(), [] (const EData &e1, const EData &e2) { return e1.id < e2.id; }), allSorted;
	if (_dMesh.edata.size()) {
		if (sorted) {
			myMaxID = _dMesh.edata.back().id;
		} else {
			permutation.resize(_dMesh.edata.size());
			std::iota(permutation.begin(), permutation.end(), 0);
			std::sort(permutation.begin(), permutation.end(), [&] (eslocal i, eslocal j) { return _dMesh.edata[i].id < _dMesh.edata[j].id; });
			myMaxID = _dMesh.edata[permutation.back()].id;
		}
	}

	MPI_Allreduce(&myMaxID, &maxID, sizeof(eslocal), MPI_BYTE, MPITools::operations().max, environment->MPICommunicator);
	MPI_Allreduce(&sorted, &allSorted, 1, MPI_INT, MPI_MIN, environment->MPICommunicator);

	if (allSorted) {
		std::vector<size_t> eCurrent = Communication::getDistribution(_dMesh.esize.size(), MPITools::operations().sizeToOffsetsSize_t);
		std::vector<size_t> eTarget = tarray<eslocal>::distribute(environment->MPIsize, eCurrent.back());

		std::vector<size_t> nCurrent = Communication::getDistribution(_dMesh.enodes.size(), MPITools::operations().sizeToOffsetsSize_t);
		std::vector<size_t> nTarget;

		if (environment->MPIrank == 0) {
			nTarget.push_back(0);
		}

		size_t nodeOffset = nCurrent[environment->MPIrank];
		size_t eTargetIndex = std::lower_bound(eTarget.begin(), eTarget.end(), eCurrent[environment->MPIrank] + 1) - eTarget.begin();
		for (size_t n = 0; n < _dMesh.esize.size(); ++n) {
			nodeOffset += _dMesh.esize[n];
			if (eCurrent[environment->MPIrank] + n + 1 == eTarget[eTargetIndex]) {
				nTarget.push_back(nodeOffset);
				++eTargetIndex;
			}
		}
		Communication::allGatherUnknownSize(nTarget);

		TimeEvent e3("BALANCE ESIZE"); e3.start();
		if (!Communication::balance(_dMesh.esize, eCurrent, eTarget)) {
			ESINFO(ERROR) << "ESPRESO internal error: balance element sizes.";
		}
		e3.end(); timing.addEvent(e3);

		TimeEvent e4("BALANCE EDATA"); e4.start();
		if (!Communication::balance(_dMesh.edata, eCurrent, eTarget)) {
			ESINFO(ERROR) << "ESPRESO internal error: balance element data.";
		}
		e4.end(); timing.addEvent(e4);

		TimeEvent e5("BALANCE ENODES"); e5.start();
		if (!Communication::balance(_dMesh.enodes, nCurrent, nTarget)) {
			ESINFO(ERROR) << "ESPRESO internal error: balance element nodes.";
		}
		e5.end(); timing.addEvent(e5);

		_eDistribution = eTarget;
	} else {
		if (environment->MPIsize == 1) {
			_eDistribution = { 0, _dMesh.esize.size() };
			std::vector<eslocal> eSize, eNodes;
			std::vector<EData> eData;
			eSize.reserve(_dMesh.esize.size());
			eData.reserve(_dMesh.esize.size());
			eNodes.reserve(_dMesh.enodes.size());
			std::vector<eslocal> edist = { 0 };
			edist.reserve(_dMesh.esize.size() + 1);
			for (size_t e = 0; e < _dMesh.esize.size(); e++) {
				edist.push_back(edist.back() + _dMesh.esize[e]);
			}

			for (size_t i = 0; i < permutation.size(); i++) {
				eData.push_back(_dMesh.edata[permutation[i]]);
				eSize.push_back(_dMesh.esize[permutation[i]]);
				eNodes.insert(eNodes.end(), _dMesh.enodes.begin() + edist[permutation[i]], _dMesh.enodes.begin() + edist[permutation[i] + 1]);
			}

			_dMesh.esize.swap(eSize);
			_dMesh.edata.swap(eData);
			_dMesh.enodes.swap(eNodes);
		} else {
			_eDistribution = tarray<eslocal>::distribute(environment->MPIsize, maxID + 1);
			std::vector<std::vector<eslocal> > sSize, sNodes, rSize, rNodes;
			std::vector<std::vector<EData> > sEData, rEData;
			std::vector<int> targets;
			std::vector<eslocal> edist = { 0 };
			edist.reserve(_dMesh.esize.size() + 1);
			for (size_t e = 0; e < _dMesh.esize.size(); e++) {
				edist.push_back(edist.back() + _dMesh.esize[e]);
			}

			if (sorted) {
				for (int r = 0; r < environment->MPIsize; r++) {
					auto begin = std::lower_bound(_dMesh.edata.begin(), _dMesh.edata.end(), _eDistribution[r], [&] (EData &edata, const size_t &ID) { return edata.id < ID; });
					auto end = std::lower_bound(_dMesh.edata.begin(), _dMesh.edata.end(), _eDistribution[r + 1], [&] (EData &edata, const size_t &ID) { return edata.id < ID; });
					if (begin != end) {
						sSize.push_back({});
						sNodes.push_back({});
						sEData.push_back({});
						targets.push_back(r);
						size_t b = begin - _dMesh.edata.begin();
						size_t e = end - _dMesh.edata.begin();
						sSize.back().insert(sSize.back().end(), _dMesh.esize.begin() + b, _dMesh.esize.begin() + e);
						sEData.back().insert(sEData.back().end(), _dMesh.edata.begin() + b, _dMesh.edata.begin() + e);
						sNodes.back().insert(sNodes.back().end(), _dMesh.enodes.begin() + edist[b], _dMesh.enodes.begin() + edist[e]);
					}
				}
			} else {
				for (int r = 0; r < environment->MPIsize; r++) {
					auto begin = std::lower_bound(permutation.begin(), permutation.end(), _eDistribution[r], [&] (eslocal i, const size_t &ID) { return _dMesh.edata[i].id < ID; });
					auto end = std::lower_bound(permutation.begin(), permutation.end(), _eDistribution[r + 1], [&] (eslocal i, const size_t &ID) { return _dMesh.edata[i].id < ID; });
					if (begin != end) {
						sSize.push_back({});
						sNodes.push_back({});
						sEData.push_back({});
						targets.push_back(r);
					}
					for (size_t n = begin - permutation.begin(); n < end - permutation.begin(); ++n) {
						sSize.back().push_back(_dMesh.esize[permutation[n]]);
						sEData.back().push_back(_dMesh.edata[permutation[n]]);
						sNodes.back().insert(sNodes.back().end(), _dMesh.enodes.begin() + edist[permutation[n]], _dMesh.enodes.begin() + edist[permutation[n] + 1]);
					}
				}
			}

			TimeEvent e3("VARIOUS ESIZE"); e3.start();
			if (!Communication::sendVariousTargets(sSize, rSize, targets)) {
				ESINFO(ERROR) << "ESPRESO internal error: distribute not sorted elements sizes.";
			}
			e3.end(); timing.addEvent(e3);

			TimeEvent e4("VARIOUS EDATA"); e4.start();
			if (!Communication::sendVariousTargets(sEData, rEData, targets)) {
				ESINFO(ERROR) << "ESPRESO internal error: distribute not sorted element data.";
			}
			e4.end(); timing.addEvent(e4);

			TimeEvent e5("VARIOUS NODES"); e5.start();
			if (!Communication::sendVariousTargets(sNodes, rNodes, targets)) {
				ESINFO(ERROR) << "ESPRESO internal error: distribute not sorted element nodes.";
			}
			e5.end(); timing.addEvent(e5);

			for (size_t r = 1; r < rSize.size(); r++) {
				rSize[0].insert(rSize[0].end(), rSize[r].begin(), rSize[r].end());
				rEData[0].insert(rEData[0].end(), rEData[r].begin(), rEData[r].end());
				rNodes[0].insert(rNodes[0].end(), rNodes[r].begin(), rNodes[r].end());
			}

			permutation.resize(rSize[0].size());
			std::iota(permutation.begin(), permutation.end(), 0);
			std::sort(permutation.begin(), permutation.end(), [&] (eslocal i, eslocal j) { return rEData[0][i].id < rEData[0][j].id; });

			edist = std::vector<eslocal>({ 0 });
			edist.reserve(rSize[0].size() + 1);
			for (size_t e = 0; e < rSize[0].size(); e++) {
				edist.push_back(edist.back() + rSize[0][e]);
			}

			_dMesh.esize.clear();
			_dMesh.enodes.clear();
			_dMesh.edata.clear();
			_dMesh.esize.reserve(permutation.size());
			_dMesh.edata.reserve(permutation.size());
			_dMesh.enodes.reserve(rNodes[0].size());
			for (size_t n = 0; n < permutation.size(); n++) {
				_dMesh.esize.push_back(rSize[0][permutation[n]]);
				_dMesh.edata.push_back(rEData[0][permutation[n]]);
				_dMesh.enodes.insert(_dMesh.enodes.end(), rNodes[0].begin() + edist[permutation[n]], rNodes[0].begin() + edist[permutation[n] + 1]);
			}
		}
	}

	timing.totalTime.endWithBarrier();
	timing.printStatsMPI();
//	exit(0);
}

void Loader::checkERegions()
{
	std::vector<MeshERegion> bregions;

	for (size_t r = 0; r < _dMesh.eregions.size(); r++) {
		if (_dMesh.eregions[r].min < _eDistribution.back() && _eDistribution.back() < _dMesh.eregions[r].max) {
			ESINFO(ERROR) << "ESPRESO Workbench parser error: weird element region.";
		}
		if (_dMesh.eregions[r].min >= _eDistribution.back()) {
			bregions.push_back(MeshERegion(std::move(_dMesh.eregions[r])));
			_dMesh.eregions.erase(_dMesh.eregions.begin() + r--);
		}
	}

	size_t bsize = 0;
	std::vector<size_t> rsize = { 0 };
	for (size_t i = 0; i < _dMesh.bregions.size(); i++) {
		bsize += _dMesh.bregions[i].esize.size();
		rsize.push_back(bsize);
	}

	std::vector<size_t> fdistribution = Communication::getDistribution(bsize, MPITools::operations().sizeToOffsetsSize_t);

	size_t origBSize = _dMesh.bregions.size();

	for (size_t r = 0; r < bregions.size(); r++) {
		std::vector<size_t> borders;
		for (int t = 0; t < environment->MPIsize; t++) {
			auto begin = std::lower_bound(bregions[r].elements.begin(), bregions[r].elements.end(), fdistribution[t] + _eDistribution.back());
			auto end = std::lower_bound(bregions[r].elements.begin(), bregions[r].elements.end(), fdistribution[t + 1] + _eDistribution.back());
			if (begin != end) {
				borders.push_back(*begin);
				borders.push_back(borders.back() + end - begin);
			}
		}

		if (!Communication::allGatherUnknownSize(borders)) {
			ESINFO(ERROR) << "ESPRESO internal error: gather bregion borders.";
		}

		bool onlyRename = false;
		for (size_t br = 0; br < origBSize; br++) {
			if (_dMesh.bregions[br].min == borders.front() && _dMesh.bregions[br].max == borders.back() - 1) {
				_dMesh.bregions[br].name = bregions[r].name;
				onlyRename = true;
				break;
			}
		}
		if (onlyRename) {
			continue;
		}

		std::vector<int> tRanks;
		std::vector<std::vector<eslocal> > sBuffer, rBuffer;

		for (int t = 0; t < environment->MPIsize; t++) {
			auto begin = std::lower_bound(bregions[r].elements.begin(), bregions[r].elements.end(), fdistribution[t] + _eDistribution.back());
			auto end = std::lower_bound(bregions[r].elements.begin(), bregions[r].elements.end(), fdistribution[t + 1] + _eDistribution.back());
			if (begin != end) {
				tRanks.push_back(t);
				sBuffer.push_back(std::vector<eslocal>(begin, end));
			}
		}

		if (!Communication::sendVariousTargets(sBuffer, rBuffer, tRanks)) {
			ESINFO(ERROR) << "ESPRESO internal error: send boundary region indices.";
		}

		for (size_t i = 1; i < rBuffer.size(); i++) {
			rBuffer[0].insert(rBuffer[0].end(), rBuffer[i].begin(), rBuffer[i].end());
		}

		auto cmp = [] (EData &edata, eslocal id) {
			return edata.id < id;
		};

		_dMesh.bregions.push_back(MeshBRegion());
		_dMesh.bregions.back().name = bregions[r].name;
		if (rBuffer.size() && rBuffer.front().size()) {
			for (size_t nr = 0; nr < origBSize; nr++) {
				if (_dMesh.bregions[nr].esize.size()) {
					auto begin = std::lower_bound(_dMesh.bregions[nr].edata.begin(), _dMesh.bregions[nr].edata.end(), rBuffer[0].front(), cmp);
					auto end = std::lower_bound(_dMesh.bregions[nr].edata.begin(), _dMesh.bregions[nr].edata.end(), rBuffer[0].back() + 1, cmp);
					for (size_t i = begin - _dMesh.bregions[nr].edata.begin(), nodes = 0; i < end - _dMesh.bregions[nr].edata.begin(); nodes += _dMesh.bregions[nr].esize[i++]) {
						_dMesh.bregions.back().edata.push_back(_dMesh.bregions[nr].edata[i]);
						_dMesh.bregions.back().enodes.insert(_dMesh.bregions.back().enodes.end(), _dMesh.bregions[nr].enodes.begin() + nodes, _dMesh.bregions[nr].enodes.begin() + nodes + _dMesh.bregions[nr].esize[i]);
						_dMesh.bregions.back().esize.push_back(_dMesh.bregions[nr].esize[i]);
					}
				}
			}
		}
	}
}

void Loader::fillElements()
{
	size_t threads = environment->OMP_NUM_THREADS;

	std::vector<std::vector<eslocal> > tedist(threads);
	std::vector<std::vector<eslocal> > tnodes(threads);
	std::vector<std::vector<eslocal> > eIDs(threads), rData(threads);
	std::vector<std::vector<int> > eMat(threads), eBody(threads);
	std::vector<std::vector<Element*> > epointers(threads);

	std::vector<eslocal> edist = { 0 };
	edist.reserve(_dMesh.esize.size() + 1);
	for (size_t e = 0; e < _dMesh.esize.size(); e++) {
		edist.push_back(edist.back() + _dMesh.esize[e]);
	}

	std::vector<size_t> edistribution = tarray<Point>::distribute(threads, _dMesh.esize.size());
	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		if(t == 0) {
			tedist[t].insert(tedist[t].end(), edist.begin() + edistribution[t], edist.begin() + edistribution[t + 1] + 1);
		} else {
			tedist[t].insert(tedist[t].end(), edist.begin() + edistribution[t] + 1, edist.begin() + edistribution[t + 1] + 1);
		}
		tnodes[t].insert(tnodes[t].end(), _dMesh.enodes.begin() + edist[edistribution[t]], _dMesh.enodes.begin() + edist[edistribution[t + 1]]);
		eIDs[t].resize(edistribution[t + 1] - edistribution[t]);
		std::iota(eIDs[t].begin(), eIDs[t].end(), _eDistribution[environment->MPIrank] + edistribution[t]);
		epointers[t].reserve(edistribution[t + 1] - edistribution[t]);
		eBody[t].reserve(edistribution[t + 1] - edistribution[t]);

		for (size_t e = edistribution[t]; e < edistribution[t + 1]; ++e) {
			epointers[t].push_back(&_mesh._eclasses[t][_dMesh.edata[e].etype]);
			eBody[t].push_back(_dMesh.edata[e].body);
			if (eIDs[t][e - edistribution[t]] != _dMesh.edata[e].id) {
				ESINFO(ERROR) << "ESPRESO Workbench parser: not implemented ordering of EBLOCK elements IDs.";
			}
		}


		if (_configuration.input == INPUT_FORMAT::WORKBENCH && _configuration.workbench.keep_material_sets) {
			eMat[t].reserve(edistribution[t + 1] - edistribution[t]);
			for (size_t e = edistribution[t]; e < edistribution[t + 1]; ++e) {
				eMat[t].push_back(_dMesh.edata[e].material);
			}
		} else {
			eMat[t].resize(edistribution[t + 1] - edistribution[t]);
		}
	}

	_mesh.elements->size = _dMesh.esize.size();
	_mesh.elements->distribution = edistribution;
	_mesh.elements->IDs = new serializededata<eslocal, eslocal>(1, eIDs);
	_mesh.elements->nodes = new serializededata<eslocal, eslocal>(tedist, tnodes);
	_mesh.elements->epointers = new serializededata<eslocal, Element*>(1, epointers);
	_mesh.elements->material = new serializededata<eslocal, int>(1, eMat);
	_mesh.elements->body = new serializededata<eslocal, int>(1, eBody);

	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		rData[t].resize(edistribution[t + 1] - edistribution[t]);
		std::iota(rData[t].begin(), rData[t].end(), edistribution[t]);
	}
	_mesh.elementsRegions.push_back(new ElementsRegionStore("ALL_ELEMENTS"));
	_mesh.elementsRegions.back()->elements = new serializededata<eslocal, eslocal>(1, rData);
}

void Loader::fillCoordinates()
{
	size_t threads = environment->OMP_NUM_THREADS;

	if (environment->MPIsize == 1) {
		std::vector<std::vector<Point> > tcoordinates(threads);
		std::vector<std::vector<eslocal> > nIDs(threads), rData(threads);

		std::vector<size_t> cdistribution = tarray<Point>::distribute(threads, _dMesh.coordinates.size());
		#pragma omp parallel for
		for (size_t t = 0; t < threads; t++) {
			tcoordinates[t].insert(tcoordinates[t].end(), _dMesh.coordinates.begin() + cdistribution[t], _dMesh.coordinates.begin() + cdistribution[t + 1]);
			nIDs[t].insert(nIDs[t].end(), _dMesh.nIDs.begin() + cdistribution[t], _dMesh.nIDs.begin() + cdistribution[t + 1]);
		}

		_mesh.nodes->size = _dMesh.coordinates.size();
		_mesh.nodes->distribution = cdistribution;
		_mesh.nodes->IDs = new serializededata<eslocal, eslocal>(1, nIDs);
		_mesh.nodes->coordinates = new serializededata<eslocal, Point>(1, tcoordinates);
		_mesh.nodes->ranks = new serializededata<eslocal, int>(1, tarray<int>(threads, _nDistribution.back()));

		#pragma omp parallel for
		for (size_t t = 0; t < threads; t++) {
			rData[t].resize(cdistribution[t + 1] - cdistribution[t]);
			std::iota(rData[t].begin(), rData[t].end(), cdistribution[t]);
		}
		_mesh.boundaryRegions.push_back(new BoundaryRegionStore("ALL_NODES", _mesh._eclasses));
		_mesh.boundaryRegions.back()->nodes = new serializededata<eslocal, eslocal>(1, rData);

		_mesh.neighboursWithMe.push_back(environment->MPIrank);
		return;
	}

	TimeEval timing("FILL COORDINATES");
	timing.totalTime.startWithBarrier();

	TimeEvent e1("SORT ENODES"); e1.start();
	std::vector<std::vector<eslocal> > nodes(threads);
	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		std::vector<eslocal> tnodes(_mesh.elements->nodes->datatarray().begin(t), _mesh.elements->nodes->datatarray().end(t));
		Esutils::sortAndRemoveDuplicity(tnodes);
		nodes[t].swap(tnodes);
	}
	Esutils::inplaceMerge(nodes);
	Esutils::removeDuplicity(nodes[0]);

	e1.end(); timing.addEvent(e1);

	TimeEvent e2("SBUFFER"); e2.start();

	std::vector<std::vector<eslocal> > sBuffer;
	std::vector<int> sRanks;

	for (int t = 0; t < environment->MPIsize; t++) {
		auto begin = std::lower_bound(nodes[0].begin(), nodes[0].end(), _nDistribution[t]);
		auto end = std::lower_bound(nodes[0].begin(), nodes[0].end(), _nDistribution[t + 1]);
		if (end - begin) {
			sBuffer.push_back(std::vector<eslocal>(begin, end));
			sRanks.push_back(t);
		}
	}

	e2.end(); timing.addEvent(e2);

	TimeEvent e3("EXCHANGE"); e3.start();

	if (!Communication::sendVariousTargets(sBuffer, _rankNodeMap, sRanks, _targetRanks)) {
		ESINFO(ERROR) << "ESPRESO internal error: exchange neighbors.";
	}

	e3.end(); timing.addEvent(e3);

	TimeEvent e4("COMPUTE BACKED"); e4.start();

	std::vector<size_t> ndistribution = tarray<Point>::distribute(threads, _dMesh.coordinates.size());
	std::vector<std::vector<std::vector<eslocal> > > backedData(threads, std::vector<std::vector<eslocal> >(_targetRanks.size()));

	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		std::vector<eslocal> ranks, ranksOffset;
		std::vector<std::vector<eslocal> > tbackedData(_targetRanks.size());
		std::vector<std::vector<eslocal>::const_iterator> rPointer(_targetRanks.size());

		for (size_t r = 0; r < _targetRanks.size(); r++) {
			rPointer[r] = std::lower_bound(_rankNodeMap[r].begin(), _rankNodeMap[r].end(), _nDistribution[environment->MPIrank] + ndistribution[t]);
		}
		for (size_t n = ndistribution[t]; n < ndistribution[t + 1]; ++n) {
			ranks.clear();
			ranksOffset.clear();
			for (size_t r = 0; r < _targetRanks.size(); r++) {
				if (rPointer[r] != _rankNodeMap[r].end() && *rPointer[r] == _nDistribution[environment->MPIrank] + n) {
					ranksOffset.push_back(r);
					ranks.push_back(_targetRanks[r]);
					++rPointer[r];
				}
			}
			for (size_t r = 0; r < ranks.size(); r++) {
				tbackedData[ranksOffset[r]].push_back(ranksOffset.size());
				tbackedData[ranksOffset[r]].insert(tbackedData[ranksOffset[r]].end(), ranks.begin(), ranks.end());
			}
		}

		backedData[t].swap(tbackedData);
	}

	e4.end(); timing.addEvent(e4);

	#pragma omp parallel for
	for (size_t r = 0; r < _targetRanks.size(); r++) {
		for (size_t t = 1; t < threads; t++) {
			backedData[0][r].insert(backedData[0][r].end(), backedData[t][r].begin(), backedData[t][r].end());
		}
	}

	TimeEvent e5("COMPUTE BACKED COORDINATES"); e5.start();

	std::vector<std::vector<Point> > backedCoordinates(_targetRanks.size());
	#pragma omp parallel for
	for (size_t r = 0; r < _targetRanks.size(); r++) {
		backedCoordinates[r].resize(_rankNodeMap[r].size());
	}

	for (size_t r = 0; r < _targetRanks.size(); r++) {
		std::vector<size_t> rdistribution = tarray<eslocal>::distribute(threads, _rankNodeMap[r].size());
		#pragma omp parallel for
		for (size_t t = 0; t < threads; t++) {
			for (size_t n = rdistribution[t]; n < rdistribution[t + 1]; ++n) {
				backedCoordinates[r][n] = _dMesh.coordinates[_rankNodeMap[r][n] - _nDistribution[environment->MPIrank]];
			}
		}
	}

	e5.end(); timing.addEvent(e5);

	TimeEvent e6("RETURN BACKED"); e6.start();

	std::vector<std::vector<eslocal> > nodeRanks(sRanks.size()), allnodes(threads);
	std::vector<std::vector<Point> > coordinates(sRanks.size());

	if (!Communication::sendVariousTargets(backedData[0], nodeRanks, _targetRanks)) {
		ESINFO(ERROR) << "ESPRESO internal error: return node ranks.";
	}
	if (!Communication::sendVariousTargets(backedCoordinates, coordinates, _targetRanks)) {
		ESINFO(ERROR) << "ESPRESO internal error: return coordinates.";
	}

	e6.end(); timing.addEvent(e6);

	TimeEvent e7("RANK DATA"); e7.start();

	size_t csize = 0;
	for (size_t i = 0; i < coordinates.size(); i++) {
		csize += coordinates[i].size();
	}

	std::vector<size_t> distribution = tarray<Point>::distribute(threads, csize);
	std::vector<std::vector<eslocal> > rankDistribution(sRanks.size());
	std::vector<std::vector<int> > rankData(sRanks.size());

	#pragma omp parallel for
	for (size_t r = 0; r < sRanks.size(); r++) {
		std::vector<eslocal> trankDistribution;
		std::vector<int> trankData;
		if (r == 0) {
			trankDistribution.push_back(0);
		}

		for (size_t n = 0; n < nodeRanks[r].size(); n += nodeRanks[r][n] + 1) {
			trankData.insert(trankData.end(), nodeRanks[r].begin() + n + 1, nodeRanks[r].begin() + n + 1 + nodeRanks[r][n]);
			trankDistribution.push_back(trankData.size());
		}

		rankDistribution[r].swap(trankDistribution);
		rankData[r].swap(trankData);
	}

	Esutils::threadDistributionToFullDistribution(rankDistribution);

	for (size_t i = threads; i < sRanks.size(); i++) {
		coordinates[threads - 1].insert(coordinates[threads - 1].end(), coordinates[i].begin(), coordinates[i].end());
		rankData[threads - 1].insert(rankData[threads - 1].end(), rankData[i].begin(), rankData[i].end());
		rankDistribution[threads - 1].insert(rankDistribution[threads - 1].end(), rankDistribution[i].begin(), rankDistribution[i].end());
	}
	for (size_t i = threads; i < sRanks.size(); i++) {
		sBuffer[threads - 1].insert(sBuffer[threads - 1].end(), sBuffer[i].begin(), sBuffer[i].end());
	}
	coordinates.resize(threads);
	sBuffer.resize(threads);
	rankData.resize(threads);
	rankDistribution.resize(threads);

	serializededata<eslocal, Point>::balance(1, coordinates, &distribution);
	serializededata<eslocal, eslocal>::balance(1, sBuffer, &distribution);
	serializededata<eslocal, int>::balance(rankDistribution, rankData, &distribution);


	e7.end(); timing.addEvent(e7);

	TimeEvent e8("BUILD TARRRAY"); e8.start();

	_mesh.nodes->size = distribution.back();
	_mesh.nodes->distribution = distribution;
	_mesh.nodes->IDs = new serializededata<eslocal, eslocal>(1, sBuffer);
	_mesh.nodes->coordinates = new serializededata<eslocal, Point>(1, coordinates);
	_mesh.nodes->ranks = new serializededata<eslocal, int>(rankDistribution, rankData);

	e8.end(); timing.addEvent(e8);

	TimeEvent e9("NEIGHBORS"); e9.start();

	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		allnodes[t].resize(distribution[t + 1] - distribution[t]);
		std::iota(allnodes[t].begin(), allnodes[t].end(), distribution[t]);
		Esutils::sortAndRemoveDuplicity(rankData[t]);
	}

	_mesh.boundaryRegions.push_back(new BoundaryRegionStore("ALL_NODES", _mesh._eclasses));
	_mesh.boundaryRegions.back()->nodes = new serializededata<eslocal, eslocal>(1, allnodes);

	for (size_t t = 0; t < threads; t++) {
		_mesh.neighboursWithMe.insert(_mesh.neighboursWithMe.end(), rankData[t].begin(), rankData[t].end());
	}
	Esutils::sortAndRemoveDuplicity(_mesh.neighboursWithMe);

	for (size_t n = 0; n < _mesh.neighboursWithMe.size(); n++) {
		if (_mesh.neighboursWithMe[n] != environment->MPIrank) {
			_mesh.neighbours.push_back(_mesh.neighboursWithMe[n]);
		}
	}

	e9.end(); timing.addEvent(e9);

	TimeEvent e10("REINDEX"); e10.start();

	#pragma omp parallel for
	for (size_t t = 0; t < threads; t++) {
		for (auto n = _mesh.elements->nodes->begin(t)->begin(); n != _mesh.elements->nodes->end(t)->begin(); ++n) {
			*n = std::lower_bound(_mesh.nodes->IDs->datatarray().begin(), _mesh.nodes->IDs->datatarray().end(), *n) - _mesh.nodes->IDs->datatarray().begin();
		}
	}

	e10.end(); timing.addEvent(e10);

	timing.totalTime.endWithBarrier();
	timing.printStatsMPI();
}

void Loader::addNodeRegions()
{
	// assume sorted nodes !!
	size_t threads = environment->OMP_NUM_THREADS;

	if (environment->MPIsize == 1) {
		for (size_t i = 0; i < _dMesh.nregions.size(); i++) {
			_mesh.boundaryRegions.push_back(new BoundaryRegionStore(_dMesh.nregions[i].name, _mesh._eclasses));

			std::vector<size_t> distribution = tarray<eslocal>::distribute(threads, _dMesh.nregions[i].nodes.size());
			std::vector<std::vector<eslocal> > tnodes(threads);
			#pragma omp parallel for
			for (size_t t = 0; t < threads; t++) {
				tnodes[t].insert(tnodes[t].end(), _dMesh.nregions[i].nodes.begin() + distribution[t], _dMesh.nregions[i].nodes.begin() + distribution[t + 1]);
			}
			_mesh.boundaryRegions.back()->nodes = new serializededata<eslocal, eslocal>(1, tnodes);

			#pragma omp parallel for
			for (size_t t = 0; t < threads; t++) {
				for (auto n = _mesh.boundaryRegions.back()->nodes->begin(t)->begin(); n != _mesh.boundaryRegions.back()->nodes->end(t)->begin(); ++n) {
					*n = std::lower_bound(_mesh.nodes->IDs->datatarray().begin(), _mesh.nodes->IDs->datatarray().end(), *n) - _mesh.nodes->IDs->datatarray().begin();
				}
			}
		}
		return;
	}

	for (size_t i = 0; i < _dMesh.nregions.size(); i++) {
		std::sort(_dMesh.nregions[i].nodes.begin(), _dMesh.nregions[i].nodes.end());

		std::vector<std::vector<eslocal> > sBuffer, rBuffer;
		std::vector<int> sRanks, tRanks;

		for (int t = 0; t < environment->MPIsize; t++) {
			auto begin = std::lower_bound(_dMesh.nregions[i].nodes.begin(), _dMesh.nregions[i].nodes.end(), _nDistribution[t]);
			auto end = std::lower_bound(_dMesh.nregions[i].nodes.begin(), _dMesh.nregions[i].nodes.end(), _nDistribution[t + 1]);
			if (end - begin) {
				sBuffer.push_back(std::vector<eslocal>(begin, end));
				sRanks.push_back(t);
			}
		}

		if (!Communication::sendVariousTargets(sBuffer, rBuffer, sRanks)) {
			ESINFO(ERROR) << "ESPRESO internal error: exchange node region.";
		}

		sBuffer.clear();
		sBuffer.resize(_targetRanks.size());
		for (size_t r = 1; r < rBuffer.size(); r++) {
			rBuffer[0].insert(rBuffer[0].end(), rBuffer[r].begin(), rBuffer[r].end());
		}

		if (rBuffer.size()) {
			#pragma omp parallel for
			for (size_t t = 0; t < _targetRanks.size(); t++) {
				sBuffer[t].resize(rBuffer[0].size());
				sBuffer[t].resize(std::set_intersection(_rankNodeMap[t].begin(), _rankNodeMap[t].end(), rBuffer[0].begin(), rBuffer[0].end(), sBuffer[t].begin()) - sBuffer[t].begin());
			}
		}

		for (size_t t = 0; t < _targetRanks.size(); t++) {
			if (sBuffer[t].size()) {
				tRanks.push_back(t);
			}
		}
		for (size_t t = 0; t < tRanks.size(); t++) {
			sBuffer[t].swap(sBuffer[tRanks[t]]);
			tRanks[t] = _targetRanks[tRanks[t]];
		}
		sBuffer.resize(tRanks.size());

		rBuffer.clear();
		if (!Communication::sendVariousTargets(sBuffer, rBuffer, tRanks)) {
			ESINFO(ERROR) << "ESPRESO internal error: exchange node region to targets.";
		}

		for (size_t t = threads; t < rBuffer.size(); t++) {
			rBuffer[threads - 1].insert(rBuffer[threads - 1].end(), rBuffer[t].begin(), rBuffer[t].end());
		}
		rBuffer.resize(threads);
		serializededata<eslocal, eslocal>::balance(1, rBuffer);

		_mesh.boundaryRegions.push_back(new BoundaryRegionStore(_dMesh.nregions[i].name, _mesh._eclasses));
		_mesh.boundaryRegions.back()->nodes = new serializededata<eslocal, eslocal>(1, rBuffer);

		#pragma omp parallel for
		for (size_t t = 0; t < threads; t++) {
			for (auto n = _mesh.boundaryRegions.back()->nodes->begin(t)->begin(); n != _mesh.boundaryRegions.back()->nodes->end(t)->begin(); ++n) {
				*n = std::lower_bound(_mesh.nodes->IDs->datatarray().begin(), _mesh.nodes->IDs->datatarray().end(), *n) - _mesh.nodes->IDs->datatarray().begin();
			}
		}
	}
}

void Loader::addBoundaryRegions()
{
	size_t threads = environment->OMP_NUM_THREADS;

	if (environment->MPIsize == 1) {
		for (size_t i = 0; i < _dMesh.bregions.size(); i++) {
			std::vector<eslocal> edist = { 0 };
			edist.reserve(_dMesh.bregions[i].esize.size() + 1);
			for (size_t e = 0; e < _dMesh.bregions[i].esize.size(); e++) {
				edist.push_back(edist.back() + _dMesh.bregions[i].esize[e]);
			}

			std::vector<std::vector<eslocal> > tedist(threads), tnodes(threads);
			std::vector<std::vector<Element*> > epointers(threads);
			std::vector<size_t> edistribution = tarray<Point>::distribute(threads, _dMesh.bregions[i].esize.size());

			tedist.front().push_back(0);
			#pragma omp parallel for
			for (size_t t = 0; t < threads; t++) {
				for (size_t n = edistribution[t]; n < edistribution[t + 1]; ++n) {
					tnodes[t].insert(tnodes[t].end(), _dMesh.bregions[i].enodes.begin() + edist[n], _dMesh.bregions[i].enodes.begin() + edist[n + 1]);
					epointers[t].push_back(&_mesh._eclasses[t][_dMesh.bregions[i].edata[n].etype]);
					tedist[t].push_back(tnodes[t].size());
				}
			}

			Esutils::threadDistributionToFullDistribution(tedist);

			_mesh.boundaryRegions.push_back(new BoundaryRegionStore(_dMesh.bregions[i].name, _mesh._eclasses));
			_mesh.boundaryRegions.back()->distribution = tarray<eslocal>::distribute(threads, epointers.front().size());
			switch (epointers.front().front()->type) {
			case Element::TYPE::PLANE:
				_mesh.boundaryRegions.back()->dimension = 2;
				break;
			case Element::TYPE::LINE:
				_mesh.boundaryRegions.back()->dimension = 1;
				break;
			default:
				ESINFO(ERROR) << "ESPRESO Workbench parser: invalid boundary region type. Have to be 3D plane or 2D line.";
			}
			_mesh.boundaryRegions.back()->elements = new serializededata<eslocal, eslocal>(tedist, tnodes);
			_mesh.boundaryRegions.back()->epointers = new serializededata<eslocal, Element*>(1, epointers);

			#pragma omp parallel for
			for (size_t t = 0; t < threads; t++) {
				for (auto n = _mesh.boundaryRegions.back()->elements->begin(t)->begin(); n != _mesh.boundaryRegions.back()->elements->end(t)->begin(); ++n) {
					*n = std::lower_bound(_mesh.nodes->IDs->datatarray().begin(), _mesh.nodes->IDs->datatarray().end(), *n) - _mesh.nodes->IDs->datatarray().begin();
				}
			}
		}
		return;
	}

	TimeEval timing("BOUNDARY REGIONS");
	timing.totalTime.startWithBarrier();

	TimeEvent e1("LINK NODES AND ELEMENTS"); e1.start();

	if (_dMesh.bregions.size()) {
		_mesh.preprocessing->linkNodesAndElements();
	}

	e1.end(); timing.addEvent(e1);

	std::vector<eslocal> edistribution = _mesh.elements->gatherElementsProcDistribution();

	for (size_t i = 0; i < _dMesh.bregions.size(); i++) {

		TimeEvent e2("PREPARE"); e2.start();

		std::vector<eslocal> edist = { 0 };
		edist.reserve(_dMesh.bregions[i].esize.size() + 1);
		for (size_t e = 0; e < _dMesh.bregions[i].esize.size(); e++) {
			edist.push_back(edist.back() + _dMesh.bregions[i].esize[e]);
		}

		std::vector<eslocal> permutation(edist.size() - 1);
		std::iota(permutation.begin(), permutation.end(), 0);
		std::sort(permutation.begin(), permutation.end(), [&] (eslocal e1, eslocal e2) {
			return _dMesh.bregions[i].enodes[edist[e1]] < _dMesh.bregions[i].enodes[edist[e2]];
		});

		e2.end(); timing.addEvent(e2);

		TimeEvent e3("SRANKS"); e3.start();

		std::vector<std::vector<eslocal> > sBuffer, rBuffer;
		std::vector<int> sRanks, tRanks;

		for (int t = 0; t < environment->MPIsize; t++) {
			auto begin = std::lower_bound(permutation.begin(), permutation.end(), _nDistribution[t], [&] (eslocal e, eslocal n) { return _dMesh.bregions[i].enodes[edist[e]] < n; });
			auto end = std::lower_bound(permutation.begin(), permutation.end(), _nDistribution[t + 1], [&] (eslocal e, eslocal n) { return _dMesh.bregions[i].enodes[edist[e]] < n; });
			if (begin != end) {
				sRanks.push_back(t);
			}
		}
		sBuffer.resize(sRanks.size());

		e3.end(); timing.addEvent(e3);

		TimeEvent e4("SBUFFER"); e4.start();

		for (size_t r = 0; r < sRanks.size(); r++) {
			auto begin = std::lower_bound(permutation.begin(), permutation.end(), _nDistribution[sRanks[r]], [&] (eslocal e, eslocal n) { return _dMesh.bregions[i].enodes[edist[e]] < n; });
			auto end = std::lower_bound(permutation.begin(), permutation.end(), _nDistribution[sRanks[r] + 1], [&] (eslocal e, eslocal n) { return _dMesh.bregions[i].enodes[edist[e]] < n; });
			std::vector<size_t> sdistribution = tarray<eslocal>::distribute(threads, end - begin);
			std::vector<std::vector<eslocal> > tsBuffer(threads);

			#pragma omp parallel for
			for (size_t t = 0; t < threads; t++) {
				std::vector<eslocal> ttsBuffer;

				for (auto e = begin + sdistribution[t]; e != begin + sdistribution[t + 1]; ++e) {
					ttsBuffer.push_back(_dMesh.bregions[i].edata[*e].etype);
					ttsBuffer.push_back(_dMesh.bregions[i].esize[*e]);
					for (eslocal n = 0; n < _dMesh.bregions[i].esize[*e]; ++n) {
						ttsBuffer.push_back(_dMesh.bregions[i].enodes[edist[*e] + n]);
					}
				}

				tsBuffer[t].swap(ttsBuffer);
			}

			sBuffer[r].push_back(0);
			for (size_t t = 0; t < threads; t++) {
				sBuffer[r].push_back(tsBuffer[t].size() + sBuffer[r].back());
			}
			for (size_t t = 0; t < threads; t++) {
				sBuffer[r].insert(sBuffer[r].end(), tsBuffer[t].begin(), tsBuffer[t].end());
			}

		}

		e4.end(); timing.addEvent(e4);

		TimeEvent e5("EXCHANGE SBUFFER"); e5.start();

		if (!Communication::sendVariousTargets(sBuffer, rBuffer, sRanks)) {
			ESINFO(ERROR) << "ESPRESO internal error: exchange node region.";
		}

		e5.end(); timing.addEvent(e5);

		TimeEvent e6("PROCESS RBUFFER"); e6.start();

		sBuffer.clear();
		sBuffer.resize(_targetRanks.size());

		for (size_t r = 0; r < rBuffer.size(); r++) {
			std::vector<std::vector<std::vector<eslocal> > > tsBuffer(threads, std::vector<std::vector<eslocal> >(_targetRanks.size()));

			#pragma omp parallel for
			for (size_t t = 0; t < threads; t++) {
				std::vector<eslocal> nodes;
				std::vector<std::vector<eslocal> > ttsBuffer(_targetRanks.size());

				for (size_t n = rBuffer[r][t] + threads + 1; n < rBuffer[r][t + 1] + threads + 1; n += 2 + rBuffer[r][n + 1]) {
					nodes.clear();
					nodes.insert(nodes.end(), rBuffer[r].begin() + n + 2, rBuffer[r].begin() + n + 2 + rBuffer[r][n + 1]);
					std::sort(nodes.begin(), nodes.end());
					auto nbegin = std::lower_bound(nodes.begin(), nodes.end(), _nDistribution[environment->MPIrank]);
					auto nend = std::lower_bound(nodes.begin(), nodes.end(), _nDistribution[environment->MPIrank + 1]);

					for (size_t tt = 0; tt < _targetRanks.size(); tt++) {
						auto it = _rankNodeMap[tt].begin();
						bool found = true;
						for (auto current = nbegin; found && current != nend; ++current) {
							it = std::lower_bound(it, _rankNodeMap[tt].end(), *current);
							found = it != _rankNodeMap[t].end() && *it == *current;
						}
						if (found) {
							ttsBuffer[tt].insert(ttsBuffer[tt].end(), rBuffer[r].begin() + n, rBuffer[r].begin() + n + 2 + rBuffer[r][n + 1]);
						}
					}
				}

				tsBuffer[t].swap(ttsBuffer);
			}

			for (size_t tt = 0; tt < _targetRanks.size(); tt++) {
				size_t tsize = 0;
				for (size_t t = 0; t < threads; t++) {
					tsize += tsBuffer[t][tt].size();
				}
				if (tsize) {
					sBuffer[tt].push_back(0);
					for (size_t t = 0; t < threads; t++) {
						sBuffer[tt].push_back(sBuffer[tt].back() + tsBuffer[t][tt].size());
					}
				}
				for (size_t t = 0; t < threads; t++) {
					sBuffer[tt].insert(sBuffer[tt].end(), tsBuffer[t][tt].begin(), tsBuffer[t][tt].end());
				}
			}
		}

		e6.end(); timing.addEvent(e6);

		TimeEvent e7("SEND DATA TO POTENTIAL OWNERS"); e7.start();

		for (size_t t = 0; t < _targetRanks.size(); t++) {
			if (sBuffer[t].size()) {
				tRanks.push_back(t);
			}
		}
		for (size_t t = 0; t < tRanks.size(); t++) {
			sBuffer[t].swap(sBuffer[tRanks[t]]);
			tRanks[t] = _targetRanks[tRanks[t]];
		}
		sBuffer.resize(tRanks.size());

		rBuffer.clear();
		if (!Communication::sendVariousTargets(sBuffer, rBuffer, tRanks)) {
			ESINFO(ERROR) << "ESPRESO internal error: exchange node region to targets.";
		}

		e7.end(); timing.addEvent(e7);

		TimeEvent e8("BUILD FACES"); e8.start();

		std::vector<std::vector<eslocal> > tedist(threads), tnodes(threads);
		std::vector<std::vector<Element*> > epointers(threads);

		for (size_t r = 0; r < rBuffer.size(); r++) {

			#pragma omp parallel for
			for (size_t t = 0; t < threads; t++) {
				std::vector<eslocal> ttedist, ttnodes;
				std::vector<Element*> tepointers;
				if (t == 0 && r == 0) {
					ttedist.push_back(0);
				}
				eslocal foffset = 0;
				if (r && tedist[t].size()) {
					foffset = tedist[t].back();
				}

				std::vector<eslocal> nodes;
				std::vector<eslocal> nlinks;
				int counter;
				bool found = true;
				for (eslocal e = rBuffer[r][t] + threads + 1; e < rBuffer[r][t + 1] + threads + 1; e += 2 + rBuffer[r][e + 1]) {
					found = true;
					for (auto n = rBuffer[r].begin() + e + 2; found && n != rBuffer[r].begin() + e + 2 + rBuffer[r][e + 1]; ++n) {
						auto it = std::lower_bound(_mesh.nodes->IDs->datatarray().begin(), _mesh.nodes->IDs->datatarray().end(), *n);
						if (it != _mesh.nodes->IDs->datatarray().end() && *it == *n) {
							*n = it - _mesh.nodes->IDs->datatarray().begin();
						} else {
							found = false;
						}
					}
					if (found) {
						nlinks.clear();
						for (auto n = rBuffer[r].begin() + e + 2; n != rBuffer[r].begin() + e + 2 + rBuffer[r][e + 1]; ++n) {
							auto links = _mesh.nodes->elements->cbegin() + *n;
							nlinks.insert(nlinks.end(), links->begin(), links->end());
						}
						std::sort(nlinks.begin(), nlinks.end());
						counter = 1;
						for (size_t i = 1; i < nlinks.size(); ++i) {
							if (nlinks[i - 1] == nlinks[i]) {
								++counter;
								if (counter == rBuffer[r][e + 1]) {
									if (_eDistribution[environment->MPIrank] <= nlinks[i] && nlinks[i] < _eDistribution[environment->MPIrank + 1]) {
										ttnodes.insert(ttnodes.end(), rBuffer[r].begin() + e + 2, rBuffer[r].begin() + e + 2 + rBuffer[r][e + 1]);
										ttedist.push_back(ttnodes.size() + foffset);
										tepointers.push_back(&_mesh._eclasses[0][rBuffer[r][e]]);
									}
									break;
								}
							} else {
								counter = 1;
							}
						}
					}
				}

				tedist[t].insert(tedist[t].end(), ttedist.begin(), ttedist.end());
				tnodes[t].insert(tnodes[t].end(), ttnodes.begin(), ttnodes.end());
				epointers[t].insert(epointers[t].end(), tepointers.begin(), tepointers.end());
			}
		}

		e8.end(); timing.addEvent(e8);

		TimeEvent e10("CREATE ARRAYS"); e10.start();

		Esutils::threadDistributionToFullDistribution(tedist);

		serializededata<eslocal, eslocal>::balance(tedist, tnodes);
		serializededata<eslocal, Element*>::balance(1, epointers);

		#pragma omp parallel for
		for (size_t t = 1; t < threads; t++) {
			for (size_t e = 0; e < epointers[t].size(); e++) {
				epointers[t][e] = &_mesh._eclasses[t][epointers[t][e] - _mesh._eclasses[0]];
			}
		}

		_mesh.boundaryRegions.push_back(new BoundaryRegionStore(_dMesh.bregions[i].name, _mesh._eclasses));
		_mesh.boundaryRegions.back()->dimension = 2;
		if (epointers.front().size()) {
			switch (epointers.front().front()->type) {
			case Element::TYPE::PLANE:
				_mesh.boundaryRegions.back()->dimension = 2;
				break;
			case Element::TYPE::LINE:
				_mesh.boundaryRegions.back()->dimension = 1;
				break;
			default:
				ESINFO(ERROR) << "ESPRESO Workbench parser: invalid boundary region type. Have to be 3D plane or 2D line.";
			}
		}
		int dim = _mesh.boundaryRegions.back()->dimension;
		MPI_Allreduce(&dim, &_mesh.boundaryRegions.back()->dimension, 1, MPI_INT, MPI_MIN, environment->MPICommunicator);

		_mesh.boundaryRegions.back()->elements = new serializededata<eslocal, eslocal>(tedist, tnodes);
		_mesh.boundaryRegions.back()->epointers = new serializededata<eslocal, Element*>(1, epointers);
		_mesh.boundaryRegions.back()->distribution = _mesh.boundaryRegions.back()->epointers->datatarray().distribution();

		e10.end(); timing.addEvent(e10);

		TimeEvent e11("-------"); e11.start();
		e11.end(); timing.addEvent(e11);
	}

	timing.totalTime.endWithBarrier();
	timing.printStatsMPI();
}

void Loader::addElementRegions()
{
	size_t threads = environment->OMP_NUM_THREADS;

	if (environment->MPIsize == 1) {
		for (size_t i = 0; i < _dMesh.eregions.size(); i++) {
			_mesh.elementsRegions.push_back(new ElementsRegionStore(_dMesh.eregions[i].name));

			std::vector<size_t> distribution = tarray<eslocal>::distribute(threads, _dMesh.eregions[i].elements.size());
			std::vector<std::vector<eslocal> > telements(threads);
			#pragma omp parallel for
			for (size_t t = 0; t < threads; t++) {
				telements[t].insert(telements[t].end(), _dMesh.eregions[i].elements.begin() + distribution[t], _dMesh.eregions[i].elements.begin() + distribution[t + 1]);
			}
			_mesh.elementsRegions.back()->elements = new serializededata<eslocal, eslocal>(1, telements);
		}
		return;
	}

	for (size_t i = 0; i < _dMesh.eregions.size(); i++) {
		std::vector<std::vector<eslocal> > sBuffer, rBuffer;
		std::vector<int> sRanks, tRanks;

		for (int t = 0; t < environment->MPIsize; t++) {
			auto begin = std::lower_bound(_dMesh.eregions[i].elements.begin(), _dMesh.eregions[i].elements.end(), _eDistribution[t]);
			auto end = std::lower_bound(_dMesh.eregions[i].elements.begin(), _dMesh.eregions[i].elements.end(), _eDistribution[t + 1]);
			if (end - begin) {
				sBuffer.push_back(std::vector<eslocal>(begin, end));
				sRanks.push_back(t);
			}
		}

		if (!Communication::sendVariousTargets(sBuffer, rBuffer, sRanks)) {
			ESINFO(ERROR) << "ESPRESO internal error: exchange node region.";
		}

		for (size_t t = threads; t < rBuffer.size(); t++) {
			rBuffer[threads - 1].insert(rBuffer[threads - 1].end(), rBuffer[t].begin(), rBuffer[t].end());
		}
		rBuffer.resize(threads);
		serializededata<eslocal, eslocal>::balance(1, rBuffer);

		_mesh.elementsRegions.push_back(new ElementsRegionStore(_dMesh.eregions[i].name));
		_mesh.elementsRegions.back()->elements = new serializededata<eslocal, eslocal>(1, rBuffer);

		#pragma omp parallel for
		for (size_t t = 0; t < threads; t++) {
			for (auto e = _mesh.elementsRegions.back()->elements->begin(t)->begin(); e != _mesh.elementsRegions.back()->elements->end(t)->begin(); ++e) {
				*e -= _eDistribution[environment->MPIrank];
			}
		}
	}
}
