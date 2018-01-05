
#include "collectedensight.h"

#include "../../../basis/containers/point.h"
#include "../../../basis/containers/serializededata.h"
#include "../../../basis/logging/logging.h"
#include "../../../basis/utilities/communication.h"
#include "../../../basis/utilities/utils.h"
#include "../../../basis/utilities/parser.h"

#include "../../../config/ecf/environment.h"

#include "../../../assembler/step.h"

#include "../../../mesh/elements/element.h"
#include "../../../mesh/mesh.h"
#include "../../../mesh/store/nodestore.h"
#include "../../../mesh/store/elementstore.h"
#include "../../../mesh/store/elementsregionstore.h"
#include "../../../mesh/store/boundaryregionstore.h"

#include <fstream>
#include <iomanip>
#include <algorithm>
#include <functional>

using namespace espreso;

CollectedEnSight::CollectedEnSight(const std::string &name, const Mesh &mesh)
: CollectedVisualization(mesh), _path(Logging::outputRoot() + "/"), _name(name), _variableCounter(0)
{
	_caseheader << "#\n";
	_caseheader << "# ESPRESO solution\n";
	_caseheader << "#\n";

	_caseheader << "\nFORMAT\n";
	_caseheader << "type: \tensight gold\n\n";

	_casegeometry << "GEOMETRY\n\n";

	_casevariables << "VARIABLE\n\n";
}

CollectedEnSight::~CollectedEnSight()
{

}

void CollectedEnSight::storecasefile()
{
	if (environment->MPIrank == 0) {
		std::ofstream os(_path + _name + ".case");
		os << _caseheader.str() << "\n";
		os << _casegeometry.str() << "\n";
		os << _casevariables.str() << "\n";

		if (_variableCounter) {
			os << "TIME\n\n";
			os << "time set:              1\n";
			os << "number of steps:       " << _variableCounter << "\n";
			os << "filename start number: 1\n";
			os << "filename increment:    1\n";
			os << "time values:           " << _casetime.str() << "\n";
		}
	}
}

std::string CollectedEnSight::codetotype(int code)
{
	switch (static_cast<Element::CODE>(code)) {

	case Element::CODE::POINT1: return "point";

	case Element::CODE::LINE2: return "bar2";

	case Element::CODE::TRIANGLE3: return "tria3";
	case Element::CODE::SQUARE4: return "quad4";

	case Element::CODE::TETRA4: return "tetra4";
	case Element::CODE::PYRAMID5: return "pyramid5";
	case Element::CODE::PRISMA6: return "penta6";
	case Element::CODE::HEXA8: return "hexa8";

	case Element::CODE::LINE3: return "bar3";

	case Element::CODE::TRIANGLE6: return "tria6";
	case Element::CODE::SQUARE8: return "quad8";

	case Element::CODE::TETRA10: return "tetra10";
	case Element::CODE::PYRAMID13: return "pyramid13";
	case Element::CODE::PRISMA15: return "penta15";
	case Element::CODE::HEXA20: return "hexa20";

	default:
		ESINFO(ERROR) << "ESPRESO internal error: unknown element code.";
		return "";
	}
}

void CollectedEnSight::updateMesh()
{
	_casegeometry << "model:\t" << _name << ".geo\n\n";

	std::string name = _path + _name + ".geo";
	int part = 1;

	std::stringstream os;
	if (environment->MPIrank == 0) {
		os << "EnSight Gold geometry format\n";
		os << "----------------------------\n";

		os << "node id off\n";
		os << "element id off\n";
	}

	auto storePartHeader = [&] (const std::string &name, eslocal nodes) {
		if (environment->MPIrank == 0) {
			os << "part\n";
			os << std::setw(10) << part++ << "\n";
			os << name << "\n";

			os << "coordinates\n";
			os << std::setw(10) << nodes << "\n";
		}
	};

	auto storeRegionNodes = [&] (const std::vector<ProcessInterval> &intervals, const serializededata<eslocal, eslocal> *nodes, std::function<double(const Point *p)> getCoordinate) {
		os << std::showpos << std::scientific << std::setprecision(5);
		for (size_t i = 0; i < intervals.size(); i++) {
			if (intervals[i].sourceProcess == environment->MPIrank) {
				for (auto n = nodes->datatarray().cbegin() + intervals[i].begin; n != nodes->datatarray().cbegin() + intervals[i].end; ++n) {
					os << getCoordinate(_mesh.nodes->coordinates->datatarray().cbegin() + *n) << "\n";
				}
			}
		}
		os << std::noshowpos;
		pushInterval(os.str().size());
	};

	auto storeNodeRegionIndex = [&] (eslocal n, const std::vector<ProcessInterval> &nintervals, const tarray<eslocal> &nodes) {
		auto iit = std::lower_bound(_mesh.nodes->pintervals.begin(), _mesh.nodes->pintervals.end(), n, [] (const ProcessInterval &interval, eslocal node) { return interval.end <= node; });
		size_t iindex = iit - _mesh.nodes->pintervals.begin();
		eslocal offset = std::lower_bound(nodes.begin() + nintervals[iindex].begin, nodes.begin() + nintervals[iindex].end, n) - nodes.begin();
		return nintervals[iindex].globalOffset + offset - nintervals[iindex].begin;
	};

	auto storeElements = [&] (
			const std::vector<eslocal> &ecounters,
			const tarray<eslocal> &elements, const std::vector<ElementsInterval> &eintervals,
			const tarray<eslocal> &nodes, const std::vector<ProcessInterval> &nintervals) {

		for (int etype = 0; etype < static_cast<int>(Element::CODE::SIZE); etype++) {
			if (ecounters[etype]) {
				if (environment->MPIrank == 0) {
					os << codetotype(etype) << "\n";
					os << std::setw(10) << ecounters[etype] << "\n";
				}

				for (size_t i = 0; i < eintervals.size(); i++) {
					if (eintervals[i].code == etype) {
						auto enodes = _mesh.elements->nodes->cbegin() + eintervals[i].begin;
						eslocal prev = eintervals[i].begin;
						for (eslocal e = eintervals[i].begin; e < eintervals[i].end; prev = elements[e++]) {
							enodes += elements[e] - prev;
							for (auto n = enodes->begin(); n != enodes->end(); ++n) {
								os << std::setw(10) << storeNodeRegionIndex(*n, nintervals, nodes) + 1;
							}
							os << "\n";
						}
					}
				}
				pushInterval(os.str().size());
			}
		}
	};

	auto storeBoundaryElements = [&] (const BoundaryRegionStore *region) {
		for (int etype = 0; etype < static_cast<int>(Element::CODE::SIZE); etype++) {
			if (region->ecounters[etype]) {
				if (environment->MPIrank == 0) {
					os << codetotype(etype) << "\n";
					os << std::setw(10) << region->ecounters[etype] << "\n";
				}

				for (size_t i = 0; i < region->eintervals.size(); i++) {
					if (region->eintervals[i].code == etype) {
						auto enodes = region->elements->cbegin() + region->eintervals[i].begin;
						for (eslocal e = region->eintervals[i].begin; e < region->eintervals[i].end; ++e, ++enodes) {
							for (auto n = enodes->begin(); n != enodes->end(); ++n) {
								os << std::setw(10) << storeNodeRegionIndex(*n, region->nintervals, region->nodes->datatarray()) + 1;
							}
							os << "\n";
						}
					}
				}
				pushInterval(os.str().size());
			}
		}
	};

	auto storeERegion = [&] (const ElementsRegionStore *region) {
		storeRegionNodes(region->nintervals, region->nodes, [] (const Point *p) { return p->x; });
		storeRegionNodes(region->nintervals, region->nodes, [] (const Point *p) { return p->y; });
		storeRegionNodes(region->nintervals, region->nodes, [] (const Point *p) { return p->z; });

		if (StringCompare::caseInsensitiveEq(region->name, "ALL_ELEMENTS")) {
			storeElements(region->ecounters, region->uniqueElements->datatarray(), region->ueintervals, region->nodes->datatarray(), region->nintervals);
		} else {
			storeElements(region->ecounters, region->elements->datatarray(), region->eintervals, region->nodes->datatarray(), region->nintervals);
		}
	};

	auto storeBRegion = [&] (const BoundaryRegionStore *region) {
		storeRegionNodes(region->nintervals, region->nodes, [] (const Point *p) { return p->x; });
		storeRegionNodes(region->nintervals, region->nodes, [] (const Point *p) { return p->y; });
		storeRegionNodes(region->nintervals, region->nodes, [] (const Point *p) { return p->z; });

		if (region->dimension) {
			storeBoundaryElements(region);
		} else {
			if (environment->MPIrank == 0) {
				os << codetotype(static_cast<int>(Element::CODE::POINT1)) << "\n";
				os << std::setw(10) << region->uniqueTotalSize << "\n";
			}
			for (eslocal i = 0; i < region->uniqueSize; ++i) {
				os << std::setw(10) << region->uniqueOffset + i + 1 << "\n";
			}
			pushInterval(os.str().size());
		}
	};

	for (size_t r = 0; r < _mesh.elementsRegions.size(); r++) {
		storePartHeader(_mesh.elementsRegions[r]->name, _mesh.elementsRegions[r]->uniqueTotalSize);
		storeERegion(_mesh.elementsRegions[r]);
	}

	for (size_t r = 0; r < _mesh.boundaryRegions.size(); r++) {
		if (!StringCompare::caseInsensitiveEq(_mesh.boundaryRegions[r]->name, "ALL_NODES")) {
			storePartHeader(_mesh.boundaryRegions[r]->name, _mesh.boundaryRegions[r]->uniqueTotalSize);
			storeBRegion(_mesh.boundaryRegions[r]);
		}
	}

	storeIntervals(name, os.str(), commitIntervals());

	storecasefile();

	auto tabs = [] (size_t size) {
		return size < 8 ? 2 : 1;
	};

	auto format = [] (size_t size) {
		if (size == 1) return "scalar";
		if (size == 4 || size == 3) return "vector";
		return "";
	};

	for (size_t i = 0; i < _mesh.nodes->data.size(); i++) {
		if (_mesh.nodes->data[i]->names.size()) {
			std::string filename = _name + "." + _mesh.nodes->data[i]->names.front().substr(0, 4);
			_casevariables << format(_mesh.nodes->data[i]->names.size());
			_casevariables << " per node:\t1 ";
			_casevariables << _mesh.nodes->data[i]->names.front();
			_casevariables << std::string(tabs(_mesh.nodes->data[i]->names.front().size()), '\t');
			_casevariables << filename << "_***\n";
		}
	}
	for (size_t i = 0; i < _mesh.elements->data.size(); i++) {
		if (_mesh.elements->data[i]->names.size()) {
			std::string filename = _name + "." + _mesh.elements->data[i]->names.front().substr(0, 4);
			_casevariables << format(_mesh.elements->data[i]->names.size());
			_casevariables << " per element:\t1 ";
			_casevariables << _mesh.elements->data[i]->names.front();
			_casevariables << std::string(tabs(_mesh.elements->data[i]->names.front().size()), '\t');
			_casevariables << filename << "_***\n";
		}
	}
}

void CollectedEnSight::storeDecomposition()
{
	_casevariables << "scalar per element:\tDOMAINS\t\t" << _name << ".DOMAINS" << "\n";
	_casevariables << "scalar per element:\tCLUSTERS\t" << _name << ".CLUSTERS" << "\n";

	auto iterateElements = [&] (std::stringstream &os, const std::vector<ElementsInterval> &intervals, const std::vector<eslocal> &ecounters, std::function<double(eslocal domain)> fnc) {
		os << std::scientific << std::setprecision(5);
		for (int etype = 0; etype < static_cast<int>(Element::CODE::SIZE); etype++) {
			if (ecounters[etype]) {
				if (environment->MPIrank == 0) {
					os << codetotype(etype) << "\n";
				}

				for (size_t i = 0; i < intervals.size(); i++) {
					if (intervals[i].code == etype) {
						for (eslocal e = intervals[i].begin; e < intervals[i].end; ++e) {
							os << " " << fnc(intervals[i].domain) << "\n";
						}
					}
				}

				pushInterval(os.str().size());
			}
		}
	};

	eslocal part = 1;

	auto storePartHeader = [&] (std::stringstream &os) {
		if (environment->MPIrank == 0) {
			os << "part\n";
			os << std::setw(10) << part++ << "\n";
		}
	};


	{ // DOMAINS
		std::string filename = _name + ".DOMAINS";
		std::string name = _path + filename;

		std::stringstream os;
		if (environment->MPIrank == 0) {
			os << "DOMAINS\n";
		}

		part = 1;
		clearIntervals();
		for (size_t r = 0; r < _mesh.elementsRegions.size(); r++) {
			storePartHeader(os);
			if (StringCompare::caseInsensitiveEq(_mesh.elementsRegions[r]->name, "ALL_ELEMENTS")) {
				iterateElements(os, _mesh.elementsRegions[r]->ueintervals, _mesh.elementsRegions[r]->ecounters, [&] (eslocal domain)->double { return domain; });
			} else {
				iterateElements(os, _mesh.elementsRegions[r]->eintervals, _mesh.elementsRegions[r]->ecounters, [&] (eslocal domain)->double { return domain; });
			}
		}

		storeIntervals(name, os.str(), commitIntervals());
	}

	{ // CLUSTERS
		std::string filename = _name + ".CLUSTERS";
		std::string name = _path + filename;

		std::stringstream os;
		if (environment->MPIrank == 0) {
			os << "CLUSTERS\n";
		}

		part = 1;
		clearIntervals();
		eslocal cluster = _mesh.elements->gatherClustersDistribution()[environment->MPIrank];
		for (size_t r = 0; r < _mesh.elementsRegions.size(); r++) {
			storePartHeader(os);
			if (StringCompare::caseInsensitiveEq(_mesh.elementsRegions[r]->name, "ALL_ELEMENTS")) {
				iterateElements(os, _mesh.elementsRegions[r]->ueintervals, _mesh.elementsRegions[r]->ecounters, [&] (eslocal domain)->double { return _mesh.elements->clusters[domain - _mesh.elements->firstDomain] + cluster; });
			} else {
				iterateElements(os, _mesh.elementsRegions[r]->eintervals, _mesh.elementsRegions[r]->ecounters, [&] (eslocal domain)->double { return _mesh.elements->clusters[domain - _mesh.elements->firstDomain] + cluster; });
			}
		}

		storeIntervals(name, os.str(), commitIntervals());
	}

	storecasefile();
}

void CollectedEnSight::updateSolution(const Step &step)
{
	_casetime << step.currentTime;
	if ((++_variableCounter) % 10 == 0) {
		_casetime << "\n                       ";
	} else {
		_casetime << " ";
	}

	for (size_t di = 0; di < _mesh.nodes->data.size(); di++) {
		if (_mesh.nodes->data[di]->names.size() == 0) {
			continue;
		}
		std::string filename = _name + "." + _mesh.nodes->data[di]->names.front().substr(0, 4);
		std::stringstream name;
		name << _path + filename + "_" << std::setw(3) << std::setfill('0') << _variableCounter;

		std::stringstream os;
		if (environment->MPIrank == 0) {
			os << _mesh.nodes->data[di]->names.front() << "\n";
		}

		eslocal part = 1;

		auto storePartHeader = [&] () {
			if (environment->MPIrank == 0) {
				os << "part\n";
				os << std::setw(10) << part++ << "\n";
				os << "coordinates\n";
			}
		};

		auto iterateNodes = [&] (const std::vector<ProcessInterval> &intervals, const tarray<eslocal> &nodes) {
			os << std::showpos << std::scientific << std::setprecision(5);
			for (size_t i = 0; i < intervals.size(); i++) {
				if (intervals[i].sourceProcess == environment->MPIrank) {
					eslocal offset = _mesh.nodes->pintervals[i].globalOffset - _mesh.nodes->uniqueOffset;
					for (eslocal n = intervals[i].begin; n < intervals[i].end; ++n) {
						eslocal index = offset + nodes[n] - _mesh.nodes->pintervals[i].begin;
						os << _mesh.nodes->data[di]->gatheredData[index] << "\n";
					}
				}
			}
			os << std::noshowpos;
			pushInterval(os.str().size());
		};

		clearIntervals();
		for (size_t r = 0; r < _mesh.elementsRegions.size(); r++) {
			storePartHeader();
			iterateNodes(_mesh.elementsRegions[r]->nintervals, _mesh.elementsRegions[r]->nodes->datatarray());
		}

		for (size_t r = 0; r < _mesh.boundaryRegions.size(); r++) {
			if (!StringCompare::caseInsensitiveEq(_mesh.boundaryRegions[r]->name, "ALL_NODES")) {
				storePartHeader();
				iterateNodes(_mesh.boundaryRegions[r]->nintervals, _mesh.boundaryRegions[r]->nodes->datatarray());
			}
		}

		storeIntervals(name.str(), os.str(), commitIntervals());
	}

	for (size_t di = 0; di < _mesh.elements->data.size(); di++) {
		if (_mesh.elements->data[di]->names.size() == 0) {
			continue;
		}
		std::string filename = _name + "." + _mesh.elements->data[di]->names.front().substr(0, 4);
		std::stringstream name;
		name << _path + filename + "_" << std::setw(3) << std::setfill('0') << _variableCounter;

		std::stringstream os;
		if (environment->MPIrank == 0) {
			os << _mesh.elements->data[di]->names.front() << "\n";
		}

		eslocal part = 1;

		auto storePartHeader = [&] () {
			if (environment->MPIrank == 0) {
				os << "part\n";
				os << std::setw(10) << part++ << "\n";
			}
		};

		auto iterateElements = [&] (
				std::stringstream &os, int etype,
				const tarray<eslocal> &elements, const std::vector<ElementsInterval> &eintervals,
				std::vector<double> &data, eslocal size) {

				for (eslocal s = 0; s < size; s++) {
					for (size_t i = 0; i < eintervals.size(); i++) {
						if (eintervals[i].code == etype) {
							for (eslocal e = eintervals[i].begin; e < eintervals[i].end; ++e) {
								os << std::setw(10) << data[size * elements[e] + s];
								os << "\n";
							}
						}
					}
					pushInterval(os.str().size());
				}
				if (size == 2) {
					for (size_t i = 0; i < eintervals.size(); i++) {
						if (eintervals[i].code == etype) {
							for (eslocal e = eintervals[i].begin; e < eintervals[i].end; ++e) {
								os << std::setw(10) << .0;
								os << "\n";
							}
						}
					}
					pushInterval(os.str().size());
				}
		};

		clearIntervals();
		for (size_t r = 0; r < _mesh.elementsRegions.size(); r++) {
			storePartHeader();
			for (int etype = 0; etype < static_cast<int>(Element::CODE::SIZE); etype++) {
				if (_mesh.elementsRegions[r]->ecounters[etype]) {
					if (environment->MPIrank == 0) {
						os << codetotype(etype) << "\n";
					}

					os << std::showpos << std::scientific << std::setprecision(5);
					eslocal size = _mesh.elements->data[di]->names.size();
					if (size > 1) {
						--size;
					}
					if (StringCompare::caseInsensitiveEq(_mesh.elementsRegions[r]->name, "ALL_ELEMENTS")) {
						iterateElements(os, etype, _mesh.elementsRegions[r]->uniqueElements->datatarray(), _mesh.elementsRegions[r]->ueintervals, *_mesh.elements->data[di]->data, size);
					} else {
						iterateElements(os, etype, _mesh.elementsRegions[r]->elements->datatarray(), _mesh.elementsRegions[r]->eintervals, *_mesh.elements->data[di]->data, size);
					}
					os << std::noshowpos;
				}
			}
		}

		storeIntervals(name.str(), os.str(), commitIntervals());
	}

	storecasefile();
}


