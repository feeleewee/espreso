
#include "vtklegacy.h"

#include "../vtkwritter.h"

#include "../../../../basis/containers/point.h"
#include "../../../../basis/containers/serializededata.h"
#include "../../../../basis/utilities/utils.h"
#include "../../../../basis/utilities/communication.h"

#include "../../../../config/ecf/environment.h"
#include "../../../../config/ecf/output.h"

#include "../../../../assembler/instance.h"

#include "../../../../mesh/mesh.h"
#include "../../../../mesh/elements/element.h"
#include "../../../../mesh/store/nodestore.h"
#include "../../../../mesh/store/elementstore.h"
#include "../../../../mesh/store/fetidatastore.h"
#include "../../../../mesh/store/surfacestore.h"
#include "../../../../mesh/store/contactstore.h"
#include "../../../../mesh/store/elementsregionstore.h"

#include "../../../../input/sfc/spacefillingcurve.h"

#include "../../../../solver/generic/SparseMatrix.h"

#include <fstream>
#include <algorithm>
#include <numeric>

using namespace espreso;

double VTKLegacy::clusterShrinkRatio = 0.95;
double VTKLegacy::domainShrinkRatio = 0.9;

VTKLegacyDebugInfo::VTKLegacyDebugInfo(const Mesh &mesh, const OutputConfiguration &configuration)
: VTKLegacy(mesh, configuration)
{
	_path = Esutils::createDirectory({ Logging::outputRoot(), "VTKLEGACY_DEBUG_OUTPUT" });
}

VTKLegacy::VTKLegacy(const Mesh &mesh, const OutputConfiguration &configuration)
: SeparatedVisualization(mesh, configuration)
{

}

void VTKLegacy::mesh(const std::string &name)
{
	std::ofstream os(name + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	size_t nsize = 0;
	std::vector<eslocal> doffset(_mesh.elements->ndomains);

	for (eslocal d = 0; d < _mesh.elements->ndomains; d++) {
		doffset[d] = nsize;
		nsize += _mesh.nodes->dintervals[d].back().DOFOffset + _mesh.nodes->dintervals[d].back().end - _mesh.nodes->dintervals[d].back().begin;
	}

	os << "POINTS " << nsize << " float\n";
	for (eslocal d = 0; d < _mesh.elements->ndomains; d++) {
		const auto &coordinates = _mesh.nodes->coordinates->datatarray();
		Point p;
		for (size_t i = 0; i < _mesh.nodes->dintervals[d].size(); ++i) {
			for (eslocal n = _mesh.nodes->dintervals[d][i].begin; n < _mesh.nodes->dintervals[d][i].end; ++n) {
				p = shrink(coordinates[n], _mesh.nodes->center, _mesh.nodes->dcenter[d], clusterShrinkRatio, domainShrinkRatio);
				os << p.x << " " << p.y << " " << p.z << "\n";
			}
		}
	}
	os << "\n";

	os << "CELLS " << _mesh.elements->size << " " << _mesh.elements->size + _mesh.elements->nodes->datatarray().size() << "\n";
	auto enodes = _mesh.elements->nodes->cbegin();
	for (eslocal d = 0; d < _mesh.elements->ndomains; d++) {
		for (eslocal e = _mesh.elements->elementsDistribution[d]; e < _mesh.elements->elementsDistribution[d + 1]; ++e, ++enodes) {
			os << enodes->size() << " ";
			for (auto n = enodes->begin(); n != enodes->end(); ++n) {
				auto it = std::lower_bound(_mesh.nodes->dintervals[d].begin(), _mesh.nodes->dintervals[d].end(), *n, [] (const DomainInterval &interval, eslocal node) { return interval.end <= node; });
				os << doffset[d] + it->DOFOffset + *n - it->begin << " ";
			}
			os << "\n";
		}
	}
	os << "\n";

	os << "CELL_TYPES " << _mesh.elements->size << "\n";
	for (auto e = _mesh.elements->epointers->datatarray().begin(); e != _mesh.elements->epointers->datatarray().end(); ++e) {
		os << VTKWritter::ecode((*e)->code) << "\n";
	}
	os << "\n";

	os << "CELL_DATA " << _mesh.elements->size << "\n";
	os << "SCALARS cluster int 1\n";
	os << "LOOKUP_TABLE default\n";
	for (eslocal e = 0; e < _mesh.elements->size; e++) {
		os << environment->MPIrank << "\n";
	}
	os << "\n";
	os << "\n";

	os << "SCALARS domains int 1\n";
	os << "LOOKUP_TABLE default\n";
	for (eslocal d = 0; d < _mesh.elements->ndomains; d++) {
		for (eslocal e = _mesh.elements->elementsDistribution[d]; e < _mesh.elements->elementsDistribution[d + 1]; ++e) {
			os << _mesh.elements->firstDomain + d << "\n";
		}
	}
	os << "\n";
}

void VTKLegacy::solution(const std::string &name)
{
	std::ofstream os(name + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	size_t nsize = 0;
	std::vector<eslocal> doffset(_mesh.elements->ndomains);

	for (eslocal d = 0; d < _mesh.elements->ndomains; d++) {
		doffset[d] = nsize;
		nsize += _mesh.nodes->dintervals[d].back().DOFOffset + _mesh.nodes->dintervals[d].back().end - _mesh.nodes->dintervals[d].back().begin;
	}

	os << "POINTS " << nsize << " float\n";
	for (eslocal d = 0; d < _mesh.elements->ndomains; d++) {
		const auto &coordinates = _mesh.nodes->coordinates->datatarray();
		Point p;
		for (size_t i = 0; i < _mesh.nodes->dintervals[d].size(); ++i) {
			for (eslocal n = _mesh.nodes->dintervals[d][i].begin; n < _mesh.nodes->dintervals[d][i].end; ++n) {
				p = shrink(coordinates[n], _mesh.nodes->center, _mesh.nodes->dcenter[d], clusterShrinkRatio, domainShrinkRatio);
				os << p.x << " " << p.y << " " << p.z << "\n";
			}
		}
	}
	os << "\n";

	os << "CELLS " << _mesh.elements->size << " " << _mesh.elements->size + _mesh.elements->nodes->datatarray().size() << "\n";
	auto enodes = _mesh.elements->nodes->cbegin();
	for (eslocal d = 0; d < _mesh.elements->ndomains; d++) {
		for (eslocal e = _mesh.elements->elementsDistribution[d]; e < _mesh.elements->elementsDistribution[d + 1]; ++e, ++enodes) {
			os << enodes->size() << " ";
			for (auto n = enodes->begin(); n != enodes->end(); ++n) {
				auto it = std::lower_bound(_mesh.nodes->dintervals[d].begin(), _mesh.nodes->dintervals[d].end(), *n, [] (const DomainInterval &interval, eslocal node) { return interval.end <= node; });
				os << doffset[d] + it->DOFOffset + *n - it->begin << " ";
			}
			os << "\n";
		}
	}
	os << "\n";

	os << "CELL_TYPES " << _mesh.elements->size << "\n";
	for (auto e = _mesh.elements->epointers->datatarray().begin(); e != _mesh.elements->epointers->datatarray().end(); ++e) {
		switch ((*e)->code) {
		case Element::CODE::SQUARE4:
			os << "9\n";
			break;
		case Element::CODE::SQUARE8:
			os << "23\n";
			break;
		case Element::CODE::TRIANGLE3:
			os << "5\n";
			break;
		case Element::CODE::TRIANGLE6:
			os << "22\n";
			break;
		case Element::CODE::TETRA4:
			os << "10\n";
			break;
		case Element::CODE::TETRA10:
			os << "24\n";
			break;
		case Element::CODE::PYRAMID5:
			os << "14\n";
			break;
		case Element::CODE::PYRAMID13:
			os << "27\n";
			break;
		case Element::CODE::PRISMA6:
			os << "13\n";
			break;
		case Element::CODE::PRISMA15:
			os << "26\n";
			break;
		case Element::CODE::HEXA8:
			os << "12\n";
			break;
		case Element::CODE::HEXA20:
			os << "25\n";
			break;
		default:
			break;
		}
	}
	os << "\n";

	os << "POINT_DATA " << nsize << "\n";
	os << "SCALARS TEMPERATURE double 1\n";
	os << "LOOKUP_TABLE default\n";
	NodeData *solution;
	for (size_t i = 0; i < _mesh.nodes->data.size(); i++) {
		if (_mesh.nodes->data[i]->decomposedData != NULL) {
			solution = _mesh.nodes->data[i];
			break;
		}
	}

	for (eslocal d = 0; d < _mesh.elements->ndomains; d++) {
		for (size_t i = 0; i < _mesh.nodes->dintervals[d].size(); ++i) {
			for (eslocal n = 0; n < _mesh.nodes->dintervals[d][i].end - _mesh.nodes->dintervals[d][i].begin; n++) {
				os << (*solution->decomposedData)[d][_mesh.nodes->dintervals[d][i].DOFOffset + n] << "\n";
			}
		}
	}
}

void VTKLegacy::nodesIntervals(const std::string &name)
{
	std::ofstream os(name + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	os << "POINTS " << _mesh.nodes->size << " float\n";
	for (auto n = _mesh.nodes->coordinates->datatarray().begin(); n != _mesh.nodes->coordinates->datatarray().end(); ++n) {
		os << n->x << " " << n->y << " " << n->z << "\n";
	}
	os << "\n";

	os << "CELLS " << _mesh.nodes->size << " " << 2 * _mesh.nodes->size << "\n";
	for (eslocal n = 0; n < _mesh.nodes->size; ++n) {
		os << "1 " << n << "\n";
	}
	os << "\n";

	os << "CELL_TYPES " << _mesh.nodes->size << "\n";
	for (eslocal n = 0; n < _mesh.nodes->size; ++n) {
		os << "1\n";
	}
	os << "\n";

	os << "CELL_DATA " << _mesh.nodes->size << "\n";
	os << "SCALARS interval int 1\n";
	os << "LOOKUP_TABLE default\n";
	int interval = 0;
	for (size_t i = 0; i < _mesh.nodes->pintervals.size(); i++) {
		for (eslocal n = _mesh.nodes->pintervals[i].begin; n < _mesh.nodes->pintervals[i].end; n++) {
			os << interval << "\n";
		}
		interval++;
	}
	os << "SCALARS sourceProcess int 1\n";
	os << "LOOKUP_TABLE default\n";
	for (size_t i = 0; i < _mesh.nodes->pintervals.size(); i++) {
		for (eslocal n = _mesh.nodes->pintervals[i].begin; n < _mesh.nodes->pintervals[i].end; n++) {
			os << _mesh.nodes->pintervals[i].sourceProcess << "\n";
		}
	}
	os << "\n";
}

void VTKLegacy::externalIntervals(const std::string &name)
{
	std::ofstream os(name + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	os << "POINTS " << _mesh.nodes->size << " float\n";
	for (auto n = _mesh.nodes->coordinates->datatarray().begin(); n != _mesh.nodes->coordinates->datatarray().end(); ++n) {
		os << n->x << " " << n->y << " " << n->z << "\n";
	}
	os << "\n";

	eslocal csize = 0;
	for (size_t i = 0; i < _mesh.nodes->externalIntervals.size(); i++) {
		eslocal ii = _mesh.nodes->externalIntervals[i];
		csize += _mesh.nodes->pintervals[ii].end - _mesh.nodes->pintervals[ii].begin;
	}

	os << "CELLS " << csize << " " << 2 * csize << "\n";
	for (size_t i = 0; i < _mesh.nodes->externalIntervals.size(); i++) {
		eslocal ii = _mesh.nodes->externalIntervals[i];
		for (eslocal n = _mesh.nodes->pintervals[ii].begin; n < _mesh.nodes->pintervals[ii].end; n++) {
			os << "1 " << n << "\n";
		}
	}
	os << "\n";

	os << "CELL_TYPES " << csize << "\n";
	for (eslocal n = 0; n < csize; ++n) {
		os << "1\n";
	}
	os << "\n";

	os << "CELL_DATA " << csize << "\n";
	os << "SCALARS interval int 1\n";
	os << "LOOKUP_TABLE default\n";
	int interval = 0;
	for (size_t i = 0; i < _mesh.nodes->externalIntervals.size(); i++) {
		eslocal ii = _mesh.nodes->externalIntervals[i];
		for (eslocal n = _mesh.nodes->pintervals[ii].begin; n < _mesh.nodes->pintervals[ii].end; n++) {
			os << interval << "\n";
		}
		interval++;
	}
	os << "\n";
}

void VTKLegacy::sharedInterface(const std::string &name)
{
	if (_mesh.FETIData == NULL || _mesh.FETIData->interfaceNodes == NULL) {
		return;
	}
	std::ofstream os(name + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	size_t points = _mesh.FETIData->interfaceNodes->datatarray().size();

	os << "POINTS " << points << " float\n";
	for (size_t i = 0; i < points; i++) {
		const Point &n = _mesh.nodes->coordinates->datatarray()[_mesh.FETIData->interfaceNodes->datatarray()[i]];
		os << n.x << " " << n.y << " " << n.z << "\n";
	}
	os << "\n";

	os << "CELLS " << points << " " << 2 * points << "\n";
	for (size_t n = 0; n < points; ++n) {
		os << "1 " << n << "\n";
	}
	os << "\n";

	os << "CELL_TYPES " << points << "\n";
	for (size_t n = 0; n < points; ++n) {
		os << "1\n";
	}
	os << "\n";

	os << "CELL_DATA " << points << "\n";
	os << "SCALARS distribution int 1\n";
	os << "LOOKUP_TABLE default\n";
	for (size_t i = 0; i < _mesh.FETIData->inodesDistribution.size() - 1; i++) {
		for (eslocal n = _mesh.FETIData->inodesDistribution[i]; n < _mesh.FETIData->inodesDistribution[i + 1]; n++) {
			os << i << "\n";
		}
	}
	os << "\n";
}

void VTKLegacy::surface(const std::string &name)
{
	auto surface = [&] (const std::string &suffix, serializededata<eslocal, eslocal>* elements, serializededata<eslocal, Element*>* epointers) {
		std::ofstream os(name + "." + suffix + std::to_string(environment->MPIrank) + ".vtk");

		os << "# vtk DataFile Version 2.0\n";
		os << "EXAMPLE\n";
		os << "ASCII\n";
		os << "DATASET UNSTRUCTURED_GRID\n\n";

		std::vector<eslocal> nodes(elements->datatarray().begin(), elements->datatarray().end());
		Esutils::sortAndRemoveDuplicity(nodes);

		os << "POINTS " << nodes.size() << " float\n";
		for (size_t n = 0; n < nodes.size(); ++n) {
			const Point &p = _mesh.nodes->coordinates->datatarray()[nodes[n]];
			os << p.x << " " << p.y << " " << p.z << "\n";
		}
		os << "\n";

		os << "CELLS " << elements->structures() << " " << elements->structures() + elements->datatarray().size() << "\n";
		for (auto element = elements->cbegin(); element != elements->cend(); ++element) {
			os << element->size() << " ";
			for (auto n = element->begin(); n != element->end(); ++n) {
				os << std::lower_bound(nodes.begin(), nodes.end(), *n) - nodes.begin() << " ";
			}
			os << "\n";
		}
		os << "\n";

		if (epointers == NULL) {
			os << "CELL_TYPES " << elements->structures() << "\n";
			for (size_t n = 0; n < elements->structures(); ++n) {
				os << "5\n";
			}
			os << "\n";
		} else {
			os << "CELL_TYPES " << epointers->structures() << "\n";
			for (size_t n = 0; n < epointers->structures(); ++n) {
				os << VTKWritter::ecode(epointers->datatarray()[n]->code) << "\n";
			}
			os << "\n";
		}
	};

	if (_mesh.surface != NULL) {
		surface("elements", _mesh.surface->elements, _mesh.surface->epointers);
		if (_mesh.surface->triangles != NULL) {
			surface("triangles",  _mesh.surface->triangles, NULL);
		}
	}

	for (size_t r = 0; r < _mesh.elementsRegions.size(); r++) {
		if (_mesh.elementsRegions[r]->surface != NULL) {
			surface(_mesh.elementsRegions[r]->name, _mesh.elementsRegions[r]->surface->elements, _mesh.elementsRegions[r]->surface->epointers);
			if (_mesh.elementsRegions[r]->surface->triangles != NULL) {
				surface(_mesh.elementsRegions[r]->name + ".triangles",  _mesh.elementsRegions[r]->surface->triangles, NULL);
			}
		}
	}
}

void VTKLegacy::domainSurface(const std::string &name)
{
	if (_mesh.domainsSurface == NULL) {
		return;
	}

	auto surface = [&] (const std::string &suffix, serializededata<eslocal, eslocal>* elements, std::vector<size_t> &distribution) {
		std::ofstream os(name + "." + suffix + std::to_string(environment->MPIrank) + ".vtk");

		os << "# vtk DataFile Version 2.0\n";
		os << "EXAMPLE\n";
		os << "ASCII\n";
		os << "DATASET UNSTRUCTURED_GRID\n\n";

		os << "POINTS " << _mesh.domainsSurface->coordinates->datatarray().size() << " float\n";
		for (eslocal d = 0; d < _mesh.elements->ndomains; ++d) {
			for (size_t n = _mesh.domainsSurface->cdistribution[d]; n < _mesh.domainsSurface->cdistribution[d + 1]; ++n) {
				Point p = shrink(_mesh.domainsSurface->coordinates->datatarray()[n], _mesh.nodes->center, _mesh.nodes->dcenter[d], clusterShrinkRatio, domainShrinkRatio);
				os << p.x << " " << p.y << " " << p.z << "\n";
			}
		}
		os << "\n";

		os << "CELLS " << elements->structures() << " " << elements->structures() + elements->datatarray().size() << "\n";
		for (eslocal d = 0; d < _mesh.elements->ndomains; ++d) {
			for (auto element = elements->cbegin() + distribution[d]; element != elements->cbegin() + distribution[d + 1]; ++element) {
				os << element->size() << " ";
				for (auto n = element->begin(); n != element->end(); ++n) {
					os << _mesh.domainsSurface->cdistribution[d] + *n << " ";
				}
				os << "\n";
			}
		}
		os << "\n";

		os << "CELL_TYPES " << distribution.back() << "\n";
		for (size_t n = 0; n < distribution.back(); ++n) {
			os << "5\n";
		}
		os << "\n";
	};

	surface("elements",  _mesh.domainsSurface->elements, _mesh.domainsSurface->edistribution);
	if (_mesh.domainsSurface->triangles != NULL) {
		surface("triangles",  _mesh.domainsSurface->triangles, _mesh.domainsSurface->tdistribution);
	}
}

void VTKLegacy::points(const std::string &name, const std::vector<eslocal> &points, const std::vector<eslocal> &distribution)
{
	std::ofstream os(name + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	os << "POINTS " << points.size() << " float\n";
	for (size_t d = 0; d < distribution.size() - 1; d++) {
		for (eslocal i = distribution[d]; i < distribution[d + 1]; i++) {
			Point n = shrink(_mesh.nodes->coordinates->datatarray()[points[i]], _mesh.nodes->center, _mesh.nodes->dcenter[d], clusterShrinkRatio, domainShrinkRatio);
			os << n.x << " " << n.y << " " << n.z << "\n";
		}
	}
	os << "\n";

	os << "CELLS " << points.size() << " " << 2 * points.size() << "\n";
	for (size_t n = 0; n < points.size(); ++n) {
		os << "1 " << n << "\n";
	}
	os << "\n";

	os << "CELL_TYPES " << points.size() << "\n";
	for (size_t n = 0; n < points.size(); ++n) {
		os << "1\n";
	}
	os << "\n";
}

void VTKLegacy::corners(const std::string &name)
{
	if (_mesh.FETIData == NULL || _mesh.FETIData->corners.size() == 0) {
		return;
	}

	std::ofstream os(name + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	eslocal points = 0;
	auto cdomains = _mesh.FETIData->cornerDomains->begin();
	for (size_t i = 0; i < _mesh.FETIData->corners.size(); ++i, ++cdomains) {
		points += cdomains->size();
	}
	os << "POINTS " << points << " float\n";
	cdomains = _mesh.FETIData->cornerDomains->begin();
	for (size_t i = 0; i < _mesh.FETIData->corners.size(); ++i, ++cdomains) {
		for (auto d = cdomains->begin(); d != cdomains->end(); ++d) {
			Point n = shrink(_mesh.nodes->coordinates->datatarray()[_mesh.FETIData->corners[i]], _mesh.nodes->center, _mesh.nodes->dcenter[*d], clusterShrinkRatio, domainShrinkRatio);
			os << n.x << " " << n.y << " " << n.z << "\n";
		}
	}
	os << "\n";

	os << "CELLS " << points << " " << 2 * points << "\n";
	for (eslocal n = 0; n < points; ++n) {
		os << "1 " << n << "\n";
	}
	os << "\n";

	os << "CELL_TYPES " << points << "\n";
	for (eslocal n = 0; n < points; ++n) {
		os << "1\n";
	}
	os << "\n";
}

void VTKLegacy::sFixPoints(const std::string &name)
{
	if (_mesh.FETIData == NULL) {
		return;
	}
	points(name, _mesh.FETIData->surfaceFixPoints, _mesh.FETIData->sFixPointsDistribution);
}

void VTKLegacy::iFixPoints(const std::string &name)
{
	if (_mesh.FETIData == NULL) {
		return;
	}
	points(name, _mesh.FETIData->innerFixPoints, _mesh.FETIData->iFixPointsDistribution);
}

void VTKLegacy::contact(const std::string &name)
{
	if (_mesh.contacts == NULL) {
		return;
	}

	double boxsize = _mesh.contacts->eps * _mesh.contacts->groupsize;
	size_t xsize = std::ceil((_mesh.contacts->boundingBox[1].x - _mesh.contacts->boundingBox[0].x) / boxsize);
	size_t ysize = std::ceil((_mesh.contacts->boundingBox[1].y - _mesh.contacts->boundingBox[0].y) / boxsize);
	size_t zsize = std::ceil((_mesh.contacts->boundingBox[1].z - _mesh.contacts->boundingBox[0].z) / boxsize);

	{ // BOUNDING BOX
		std::ofstream osbb(name + std::to_string(environment->MPIrank) + ".boundingbox.vtk");

		osbb << "# vtk DataFile Version 2.0\n";
		osbb << "EXAMPLE\n";
		osbb << "ASCII\n";
		osbb << "DATASET UNSTRUCTURED_GRID\n\n";

		osbb << "POINTS 8 float\n";
		osbb << _mesh.contacts->boundingBox[0].x << " " << _mesh.contacts->boundingBox[0].y << " " << _mesh.contacts->boundingBox[0].z << "\n";
		osbb << _mesh.contacts->boundingBox[1].x << " " << _mesh.contacts->boundingBox[0].y << " " << _mesh.contacts->boundingBox[0].z << "\n";
		osbb << _mesh.contacts->boundingBox[0].x << " " << _mesh.contacts->boundingBox[1].y << " " << _mesh.contacts->boundingBox[0].z << "\n";
		osbb << _mesh.contacts->boundingBox[1].x << " " << _mesh.contacts->boundingBox[1].y << " " << _mesh.contacts->boundingBox[0].z << "\n";
		osbb << _mesh.contacts->boundingBox[0].x << " " << _mesh.contacts->boundingBox[0].y << " " << _mesh.contacts->boundingBox[1].z << "\n";
		osbb << _mesh.contacts->boundingBox[1].x << " " << _mesh.contacts->boundingBox[0].y << " " << _mesh.contacts->boundingBox[1].z << "\n";
		osbb << _mesh.contacts->boundingBox[0].x << " " << _mesh.contacts->boundingBox[1].y << " " << _mesh.contacts->boundingBox[1].z << "\n";
		osbb << _mesh.contacts->boundingBox[1].x << " " << _mesh.contacts->boundingBox[1].y << " " << _mesh.contacts->boundingBox[1].z << "\n";
		osbb << "\n";

		osbb << "CELLS 1 9 \n";
		osbb << "8 0 1 3 2 4 5 7 6\n";
		osbb << "\n";

		osbb << "CELL_TYPES 1\n";
		Element::CODE code = Element::CODE::HEXA8;
		osbb << VTKWritter::ecode(code) << "\n";
		osbb << "\n";
		osbb.close();
	}

	{ // BOXS
		std::ofstream osb(name + std::to_string(environment->MPIrank) + ".boxs.vtk");

		osb << "# vtk DataFile Version 2.0\n";
		osb << "EXAMPLE\n";
		osb << "ASCII\n";
		osb << "DATASET UNSTRUCTURED_GRID\n\n";

		Point pmin = _mesh.contacts->boundingBox[0], pmax = _mesh.contacts->boundingBox[1];

		size_t points = 4 * (xsize - 1) * (ysize - 1) * (zsize - 1);
		osb << "POINTS " << points << " float\n";

		pmin = _mesh.contacts->boundingBox[0];
		pmax = _mesh.contacts->boundingBox[1];
		for (size_t x = 1; x < xsize; ++x) {
			pmin.x = _mesh.contacts->boundingBox[0].x + x * boxsize;
			osb << pmin.x << " " << pmin.y << " " << pmin.z << "\n";
			osb << pmin.x << " " << pmax.y << " " << pmin.z << "\n";
			osb << pmin.x << " " << pmin.y << " " << pmax.z << "\n";
			osb << pmin.x << " " << pmax.y << " " << pmax.z << "\n";
		}
		pmin = _mesh.contacts->boundingBox[0];
		pmax = _mesh.contacts->boundingBox[1];
		for (size_t y = 1; y < ysize; ++y) {
			pmin.y = _mesh.contacts->boundingBox[0].y + y * boxsize;
			osb << pmin.x << " " << pmin.y << " " << pmin.z << "\n";
			osb << pmax.x << " " << pmin.y << " " << pmin.z << "\n";
			osb << pmin.x << " " << pmin.y << " " << pmax.z << "\n";
			osb << pmax.x << " " << pmin.y << " " << pmax.z << "\n";
		}
		pmin = _mesh.contacts->boundingBox[0];
		pmax = _mesh.contacts->boundingBox[1];
		for (size_t z = 1; z < zsize; ++z) {
			pmin.z = _mesh.contacts->boundingBox[0].z + z * boxsize;
			osb << pmin.x << " " << pmin.y << " " << pmin.z << "\n";
			osb << pmax.x << " " << pmin.y << " " << pmin.z << "\n";
			osb << pmin.x << " " << pmax.y << " " << pmin.z << "\n";
			osb << pmax.x << " " << pmax.y << " " << pmin.z << "\n";
		}
		osb << "\n";

		osb << "CELLS " << points / 4 << " " << points / 4 + points << " \n";
		size_t plane = 0;
		for (size_t x = 1; x < xsize; ++x, ++plane) {
			osb << "4 " << 4 * plane + 0 << " " << 4 * plane + 1 << " " << 4 * plane + 3 << " " << 4 * plane + 2 << "\n";
		}
		for (size_t y = 1; y < ysize; ++y, ++plane) {
			osb << "4 " << 4 * plane + 0 << " " << 4 * plane + 1 << " " << 4 * plane + 3 << " " << 4 * plane + 2 << "\n";
		}
		for (size_t z = 1; z < zsize; ++z, ++plane) {
			osb << "4 " << 4 * plane + 0 << " " << 4 * plane + 1 << " " << 4 * plane + 3 << " " << 4 * plane + 2 << "\n";
		}
		osb << "\n";

		osb << "CELL_TYPES " << points / 4 << "\n";
		Element::CODE code = Element::CODE::SQUARE4;
		for (size_t x = 1; x < xsize; ++x) {
			osb << VTKWritter::ecode(code) << "\n";
		}
		for (size_t y = 1; y < ysize; ++y) {
			osb << VTKWritter::ecode(code) << "\n";
		}
		for (size_t z = 1; z < zsize; ++z) {
			osb << VTKWritter::ecode(code) << "\n";
		}
		osb << "\n";
		osb.close();
	}

	{ // FILLED
		size_t points = 8 * _mesh.contacts->filledCells.size();
		Point begin, end;

		std::ofstream os(name + std::to_string(environment->MPIrank) + ".filled.vtk");

		os << "# vtk DataFile Version 2.0\n";
		os << "EXAMPLE\n";
		os << "ASCII\n";
		os << "DATASET UNSTRUCTURED_GRID\n\n";

		os << "POINTS " << points << " float\n";
		for (size_t i = 0; i < _mesh.contacts->filledCells.size(); ++i) {
			begin = _mesh.contacts->boundingBox[0];
			begin.x += boxsize * (_mesh.contacts->filledCells[i] % xsize);
			begin.y += boxsize * (_mesh.contacts->filledCells[i] % (xsize * ysize) / xsize);
			begin.z += boxsize * (_mesh.contacts->filledCells[i] / (xsize * ysize));
			end.x = begin.x + boxsize;
			end.y = begin.y + boxsize;
			end.z = begin.z + boxsize;

			os << begin.x << " " << begin.y << " " << begin.z << "\n";
			os << end.x << " " << begin.y << " " << begin.z << "\n";
			os << begin.x << " " << end.y << " " << begin.z << "\n";
			os << end.x << " " << end.y << " " << begin.z << "\n";
			os << begin.x << " " << begin.y << " " << end.z << "\n";
			os << end.x << " " << begin.y << " " << end.z << "\n";
			os << begin.x << " " << end.y << " " << end.z << "\n";
			os << end.x << " " << end.y << " " << end.z << "\n";
		}

		os << "CELLS " << points / 8 << " " << points / 8 + points << "\n";
		for (size_t i = 0; i < _mesh.contacts->filledCells.size(); ++i) {
			os << "8 "
					<< 8 * i + 0 << " "
					<< 8 * i + 1 << " "
					<< 8 * i + 3 << " "
					<< 8 * i + 2 << " "
					<< 8 * i + 4 << " "
					<< 8 * i + 5 << " "
					<< 8 * i + 7 << " "
					<< 8 * i + 6 << "\n";
		}
		os << "\n";

		os << "CELL_TYPES " << points / 8 << "\n";
		Element::CODE code = Element::CODE::HEXA8;
		for (size_t i = 0; i < _mesh.contacts->filledCells.size(); ++i) {
			os << VTKWritter::ecode(code) << "\n";
		}
		os << "\n";

		os << "CELL_DATA " << points / 8 << "\n";
		os << "SCALARS faces int 1\n";
		os << "LOOKUP_TABLE default\n";
		for (auto cell = _mesh.contacts->grid->cbegin(); cell != _mesh.contacts->grid->cend(); ++cell) {
			os << cell->size() << "\n";
		}
		os << "\n";

		os.close();
	}

	{ // NEIGHBORS
		Point center(
				(_mesh.contacts->boundingBox[0].x + _mesh.contacts->boundingBox[1].x) / 2,
				(_mesh.contacts->boundingBox[0].y + _mesh.contacts->boundingBox[1].y) / 2,
				(_mesh.contacts->boundingBox[0].z + _mesh.contacts->boundingBox[1].z) / 2);

		std::vector<Point> centers(environment->MPIsize);
		MPI_Allgather(&center, 3, MPI_DOUBLE, centers.data(), 3, MPI_DOUBLE, environment->MPICommunicator);

		std::ofstream os(name + std::to_string(environment->MPIrank) + ".neighbors.vtk");

		os << "# vtk DataFile Version 2.0\n";
		os << "EXAMPLE\n";
		os << "ASCII\n";
		os << "DATASET UNSTRUCTURED_GRID\n\n";

		os << "POINTS " << _mesh.contacts->neighbors.size() + 1 << " float\n";
		os << center.x << " " << center.y << " " << center.z << "\n";
		for (size_t i = 0; i < _mesh.contacts->neighbors.size(); ++i) {
			Point &p = centers[_mesh.contacts->neighbors[i]];
			os << p.x << " " << p.y << " " << p.z << "\n";
		}
		os << "\n";

		os << "CELLS " << _mesh.contacts->neighbors.size() << " " << 3 * _mesh.contacts->neighbors.size() << "\n";
		for (size_t i = 0; i < _mesh.contacts->neighbors.size(); ++i) {
			os << "2 0 " << i + 1 << "\n";
		}
		os << "\n";

		os << "CELL_TYPES " << _mesh.contacts->neighbors.size() << "\n";
		Element::CODE code = Element::CODE::LINE2;
		for (size_t i = 0; i < _mesh.contacts->neighbors.size(); ++i) {
			os << VTKWritter::ecode(code) << "\n";
		}
		os << "\n";

		os.close();
	}
}

void VTKLegacy::closeElements(const std::string &name)
{
	if (_mesh.contacts == NULL || _mesh.contacts->closeElements == NULL) {
		return;
	}

	std::ofstream os(name + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	size_t points = _mesh.contacts->elements->structures() + _mesh.contacts->closeElements->datatarray().size();
	for (size_t n = 0; n < _mesh.contacts->neighbors.size(); n++) {
		points += _mesh.contacts->nsurface[n].size() + _mesh.contacts->ncloseElements[n]->datatarray().size();
	}

	std::vector<Point> centers(environment->MPIsize);
	MPI_Allgather(&_mesh.nodes->center, 3, MPI_DOUBLE, centers.data(), 3, MPI_DOUBLE, environment->MPICommunicator);

	os << "POINTS " << points << " float\n";
	auto closest = _mesh.contacts->closeElements->cbegin();
	for (auto e = _mesh.contacts->elements->cbegin(); e != _mesh.contacts->elements->cend(); ++e, ++closest) {
		Point center;
		for (auto n = e->begin(); n != e->end(); ++n) {
			center += *n;
		}
		center /= e->size();
		center = shrink(center, _mesh.nodes->center, center, clusterShrinkRatio, 1);
		os << center.x << " " << center.y << " " << center.z << "\n";

		for (auto ne = closest->begin(); ne != closest->end(); ++ne) {
			auto ce = _mesh.contacts->elements->cbegin() + *ne;

			Point ncenter;
			for (auto n = ce->begin(); n != ce->end(); ++n) {
				ncenter += *n;
			}
			ncenter /= ce->size();

			ncenter = shrink(ncenter, _mesh.nodes->center, ncenter, clusterShrinkRatio, 1);
			ncenter = center + (ncenter - center) / 2;
			os << ncenter.x << " " << ncenter.y << " " << ncenter.z << "\n";
		}
	}

	for (size_t n = 0; n < _mesh.contacts->neighbors.size(); n++) {
		auto closest = _mesh.contacts->ncloseElements[n]->cbegin();
		for (size_t i = 0; i < _mesh.contacts->nsurface[n].size(); ++i, ++closest) {
			auto e = _mesh.contacts->elements->cbegin() + _mesh.contacts->nsurface[n][i];
			Point center;
			for (auto nn = e->begin(); nn != e->end(); ++nn) {
				center += *nn;
			}
			center /= e->size();
			center = shrink(center, _mesh.nodes->center, center, clusterShrinkRatio, 1);
			os << center.x << " " << center.y << " " << center.z << "\n";

			for (auto ne = closest->begin(); ne != closest->end(); ++ne) {
				auto ce = _mesh.contacts->nelements[n]->cbegin() + *ne;

				Point ncenter;
				for (auto nn = ce->begin(); nn != ce->end(); ++nn) {
					ncenter += *nn;
				}
				ncenter /= ce->size();

				ncenter = shrink(ncenter, centers[_mesh.contacts->neighbors[n]], ncenter, clusterShrinkRatio, 1);
				ncenter = center + (ncenter - center) / 2;
				os << ncenter.x << " " << ncenter.y << " " << ncenter.z << "\n";
			}
		}
	}

	os << "\n";

	size_t cells = _mesh.contacts->closeElements->datatarray().size();
	for (size_t n = 0; n < _mesh.contacts->neighbors.size(); n++) {
		cells += _mesh.contacts->ncloseElements[n]->datatarray().size();
	}

	os << "CELLS " << cells << " " << 3 * cells << "\n";
	eslocal eindex = 0, noffset;
	for (auto e = _mesh.contacts->closeElements->cbegin(); e != _mesh.contacts->closeElements->cend(); ++e) {
		noffset = 1;
		for (auto n = e->begin(); n != e->end(); ++n, ++noffset) {
			os << "2 " << eindex << " " << eindex + noffset << "\n";
		}
		eindex += noffset;
	}
	for (size_t n = 0; n < _mesh.contacts->neighbors.size(); n++) {
		for (auto e = _mesh.contacts->ncloseElements[n]->cbegin(); e != _mesh.contacts->ncloseElements[n]->cend(); ++e) {
			noffset = 1;
			for (auto n = e->begin(); n != e->end(); ++n, ++noffset) {
				os << "2 " << eindex << " " << eindex + noffset << "\n";
			}
			eindex += noffset;
		}
	}
	os << "\n";

	os << "CELL_TYPES " << cells << "\n";
	Element::CODE ecode = Element::CODE::LINE2;
	for (size_t n = 0; n < cells; ++n) {
		os << VTKWritter::ecode(ecode) << "\n";
	}
	os << "\n";

	os << "CELL_DATA " << cells << "\n";
	os << "SCALARS value int 1\n";
	os << "LOOKUP_TABLE default\n";
	eindex = 0;
	for (auto e = _mesh.contacts->closeElements->cbegin(); e != _mesh.contacts->closeElements->cend(); ++e, ++eindex) {
		for (auto n = e->begin(); n != e->end(); ++n) {
			if (*n < eindex) {
				os << "1\n";
			} else {
				os << "-1\n";
			}
		}
	}
	for (size_t n = 0; n < _mesh.contacts->neighbors.size(); n++) {
		for (auto e = _mesh.contacts->ncloseElements[n]->cbegin(); e != _mesh.contacts->ncloseElements[n]->cend(); ++e, ++eindex) {
			if (environment->MPIrank < _mesh.contacts->neighbors[n]) {
				for (auto n = e->begin(); n != e->end(); ++n) {
					os << "1\n";
				}
			} else {
				for (auto n = e->begin(); n != e->end(); ++n) {
					os << "-1\n";
				}
			}
		}
	}
	os << "\n";
}

void VTKLegacy::neighbors(const std::string &name)
{
	if (_mesh.elements->neighbors == NULL) {
		return;
	}

	std::vector<eslocal> pdistribution = _mesh.elements->gatherElementsProcDistribution();
	eslocal ebegin = pdistribution[environment->MPIrank];
	eslocal eend = ebegin + _mesh.elements->size;

	std::vector<std::vector<eslocal> > sIDs(_mesh.neighbours.size()), rIDs(_mesh.neighbours.size());
	std::vector<std::vector<Point> > sCenters(_mesh.neighbours.size()), rCenters(_mesh.neighbours.size());

	auto neighbors = _mesh.elements->neighbors->cbegin();
	auto element = _mesh.elements->nodes->cbegin();
	for (eslocal e = 0; e < _mesh.elements->size; ++e, ++element, ++neighbors) {
		Point center;
		for (auto n = element->begin(); n != element->end(); ++n) {
			center += _mesh.nodes->coordinates->datatarray()[*n];
		}
		center /= element->size();

		for (auto n = neighbors->begin(); n != neighbors->end(); ++n) {
			if (*n != -1 && (*n < ebegin || eend <= *n)) {
				eslocal target = std::lower_bound(pdistribution.begin(), pdistribution.end(), *n + 1) - pdistribution.begin() - 1;
				eslocal tindex = std::lower_bound(_mesh.neighbours.begin(), _mesh.neighbours.end(), target + 1) - _mesh.neighbours.begin() - 1;
				sIDs[tindex].push_back(ebegin + e);
				sCenters[tindex].push_back(center);
			}
		}
	}

	Communication::exchangeUnknownSize(sIDs, rIDs, _mesh.neighbours);
	Communication::exchangeUnknownSize(sCenters, rCenters, _mesh.neighbours);


	std::ofstream os(name + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	size_t points = _mesh.elements->size + _mesh.elements->neighbors->datatarray().size();
	os << "POINTS " << points << " float\n";

	neighbors = _mesh.elements->neighbors->cbegin();
	element = _mesh.elements->nodes->cbegin();
	for (eslocal e = 0; e < _mesh.elements->size; ++e, ++element, ++neighbors) {
		Point center;
		for (auto n = element->begin(); n != element->end(); ++n) {
			center += _mesh.nodes->coordinates->datatarray()[*n];
		}
		center /= element->size();

		os << center << "\n";

		auto face = _mesh.elements->epointers->datatarray()[e]->faces->cbegin();
		for (auto n = neighbors->begin(); n != neighbors->end(); ++n, ++face) {
			if (*n == -1) {
				Point fCenter;
				for (auto f = face->begin(); f != face->end(); ++f) {
					fCenter += _mesh.nodes->coordinates->datatarray()[element->at(*f)];
				}
				fCenter /= face->size();
				os << center + (fCenter - center) / 2.1 << "\n";
			} else if (*n < ebegin || eend <= *n) {
				eslocal target = std::lower_bound(pdistribution.begin(), pdistribution.end(), *n + 1) - pdistribution.begin() - 1;
				eslocal tindex = std::lower_bound(_mesh.neighbours.begin(), _mesh.neighbours.end(), target + 1) - _mesh.neighbours.begin() - 1;
				eslocal offset = std::lower_bound(rIDs[tindex].begin(), rIDs[tindex].end(), *n) - rIDs[tindex].begin();
				os << center + (rCenters[tindex][offset] - center) / 2.1 << "\n";
			} else {
				Point ncenter;
				auto nelement = _mesh.elements->nodes->cbegin() + (*n - ebegin);
				for (auto nn = nelement->begin(); nn != nelement->end(); ++nn) {
					ncenter += _mesh.nodes->coordinates->datatarray()[*nn];
				}
				ncenter /= nelement->size();
				os << center + (ncenter - center) / 2.1 << "\n";
			}
		}
	}
	os << "\n";

	size_t cells = _mesh.elements->neighbors->datatarray().size();

	os << "CELLS " << cells << " " << 3 * cells << "\n";
	size_t nindex = 0, noffset;
	for (auto e = _mesh.elements->neighbors->cbegin(); e != _mesh.elements->neighbors->cend(); ++e, nindex += noffset) {
		noffset = 1;
		for (auto n = e->begin(); n != e->end(); ++n, ++noffset) {
			os << "2 " << nindex << " " << nindex + noffset << "\n";
		}
	}
	os << "\n";

	os << "CELL_TYPES " << cells << "\n";
	Element::CODE ecode = Element::CODE::LINE2;
	for (size_t n = 0; n < cells; ++n) {
		os << VTKWritter::ecode(ecode) << "\n";
	}
	os << "\n";
}

void VTKLegacyDebugInfo::dirichlet(const Mesh &mesh, const Instance &instance)
{
	std::ofstream os(Esutils::createDirectory({ Logging::outputRoot(), "VTKLEGACY_DEBUG_OUTPUT" }) + "DIRICHLET" + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	size_t points = 0;
	for (size_t d = 0; d < instance.domains; d++) {
		points += std::lower_bound(instance.B1[d].I_row_indices.begin(), instance.B1[d].I_row_indices.end(), instance.block[Instance::CONSTRAINT::DIRICHLET] + 1) - instance.B1[d].I_row_indices.begin();
	}
	os << "POINTS " << points << " float\n";

	for (size_t d = 0; d < instance.domains; d++) {
		for (size_t i = 0; i < instance.B1[d].I_row_indices.size() && instance.B1[d].I_row_indices[i] <= (eslocal)instance.block[Instance::CONSTRAINT::DIRICHLET]; i++) {
			auto it = std::lower_bound(mesh.nodes->dintervals[d].begin(), mesh.nodes->dintervals[d].end(), instance.B1[d].J_col_indices[i], [&] (DomainInterval &d, eslocal dof) {
				return d.DOFOffset + d.end - d.begin < dof;
			});
			Point p = shrink(
					mesh.nodes->coordinates->datatarray()[it->begin + instance.B1[d].J_col_indices[i] - 1 - it->DOFOffset],
					mesh.nodes->center, mesh.nodes->dcenter[d], clusterShrinkRatio, domainShrinkRatio);
			os << p << "\n";
		}
	}

	os << "\n";

	size_t cells = points;

	os << "CELLS " << cells << " " << 2 * cells << "\n";
	for (size_t e = 0; e < cells; e++) {
		os << "1 " << e << "\n";
	}
	os << "\n";

	os << "CELL_TYPES " << cells << "\n";
	Element::CODE ecode = Element::CODE::POINT1;
	for (size_t n = 0; n < cells; ++n) {
		os << VTKWritter::ecode(ecode) << "\n";
	}
	os << "\n";

	os << "CELL_DATA " << cells << "\n";
	os << "SCALARS value float 1\n";
	os << "LOOKUP_TABLE default\n";
	for (size_t d = 0; d < instance.domains; d++) {
		for (size_t i = 0; i < instance.B1[d].I_row_indices.size() && instance.B1[d].I_row_indices[i] <= (eslocal)instance.block[Instance::CONSTRAINT::DIRICHLET]; i++) {
			os << instance.B1c[d][i] << "\n";
		}
	}
	os << "\n";
}

void VTKLegacyDebugInfo::gluing(const Mesh &mesh, const Instance &instance)
{
	std::vector<int> neighbours(environment->MPIsize);
	std::iota(neighbours.begin(), neighbours.end(), 0);

	std::vector<std::vector<eslocal> > sLambdas(environment->MPIsize), rLambdas(environment->MPIsize);
	std::vector<std::vector<Point> > sPoints(environment->MPIsize), rPoints(environment->MPIsize);

	size_t points = 0;

	for (size_t d = 0; d < instance.domains; d++) {
		auto start = std::upper_bound(instance.B1[d].I_row_indices.begin(), instance.B1[d].I_row_indices.end(), instance.block[Instance::CONSTRAINT::DIRICHLET]);
		auto end = std::upper_bound(instance.B1[d].I_row_indices.begin(), instance.B1[d].I_row_indices.end(), instance.block[Instance::CONSTRAINT::DIRICHLET] + instance.block[Instance::CONSTRAINT::EQUALITY_CONSTRAINTS]);
		points += end - start;
		for (eslocal i = start - instance.B1[d].I_row_indices.begin(); i < end - instance.B1[d].I_row_indices.begin(); i++) {
			auto cmapit = std::lower_bound(instance.B1clustersMap.begin(), instance.B1clustersMap.end(), instance.B1[d].I_row_indices[i] - 1, [&] (const std::vector<eslocal> &v, eslocal i) {
				return v[0] < i;
			});
			auto dit = std::lower_bound(mesh.nodes->dintervals[d].begin(), mesh.nodes->dintervals[d].end(), instance.B1[d].J_col_indices[i], [&] (DomainInterval &d, eslocal dof) {
				return d.DOFOffset + d.end - d.begin < dof;
			});
			Point p = shrink(
					mesh.nodes->coordinates->datatarray()[dit->begin + instance.B1[d].J_col_indices[i] - 1 - dit->DOFOffset],
					mesh.nodes->center, mesh.nodes->dcenter[d], clusterShrinkRatio, domainShrinkRatio);

			sLambdas[(*cmapit)[1]].push_back(cmapit->front());
			sPoints[(*cmapit)[1]].push_back(p);

			if (cmapit->size() == 3) {
				sLambdas[(*cmapit)[2]].push_back(cmapit->front());
				sPoints[(*cmapit)[2]].push_back(p);
			}
		}
	}

	if (!Communication::exchangeUnknownSize(sLambdas, rLambdas, neighbours)) {
		ESINFO(ERROR) << "problem while exchanging Lambdas in storeLambdas.";
	}
	if (!Communication::exchangeUnknownSize(sPoints, rPoints, neighbours)) {
		ESINFO(ERROR) << "problem while exchanging Points in storeLambdas.";
	}
	for (int i = 0; i < environment->MPIsize; i++) {
		if (sLambdas[i].size() != rLambdas[i].size() || sPoints[i].size() != rPoints[i].size()) {
			ESINFO(ERROR) << "Lambda indices do not match.";
		}
	}

	std::ofstream os(Esutils::createDirectory({ Logging::outputRoot(), "VTKLEGACY_DEBUG_OUTPUT" }) + "GLUING" + std::to_string(environment->MPIrank) + ".vtk");

	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	os << "POINTS " << 2 * points << " float\n";

	for (size_t d = 0, offset = 0; d < instance.domains; d++) {
		auto start = std::upper_bound(instance.B1[d].I_row_indices.begin(), instance.B1[d].I_row_indices.end(), instance.block[Instance::CONSTRAINT::DIRICHLET]);
		auto end = std::upper_bound(instance.B1[d].I_row_indices.begin(), instance.B1[d].I_row_indices.end(), instance.block[Instance::CONSTRAINT::DIRICHLET] + instance.block[Instance::CONSTRAINT::EQUALITY_CONSTRAINTS]);
		for (eslocal i = start - instance.B1[d].I_row_indices.begin(); i < end - instance.B1[d].I_row_indices.begin(); i++, offset++) {
			auto cmapit = std::lower_bound(instance.B1clustersMap.begin(), instance.B1clustersMap.end(), instance.B1[d].I_row_indices[i] - 1, [&] (const std::vector<eslocal> &v, eslocal i) {
				return v[0] < i;
			});
			auto dit = std::lower_bound(mesh.nodes->dintervals[d].begin(), mesh.nodes->dintervals[d].end(), instance.B1[d].J_col_indices[i], [&] (DomainInterval &d, eslocal dof) {
				return d.DOFOffset + d.end - d.begin < dof;
			});
			Point ptarget, psource = shrink(
					mesh.nodes->coordinates->datatarray()[dit->begin + instance.B1[d].J_col_indices[i] - 1 - dit->DOFOffset],
					mesh.nodes->center, mesh.nodes->dcenter[d], clusterShrinkRatio, domainShrinkRatio);

			os << psource << "\n";

			if (cmapit->size() == 3) {
				size_t index = std::find(rLambdas[(*cmapit)[2]].begin(), rLambdas[(*cmapit)[2]].end(), cmapit->front()) - rLambdas[(*cmapit)[2]].begin();
				if (index == rLambdas[(*cmapit)[2]].size() || rLambdas[(*cmapit)[2]][index] != cmapit->front()) {
					ESINFO(ERROR) << "Different Lambdas on neighbour clusters.";
				}
				ptarget = rPoints[(*cmapit)[2]][index];
			} else {
				size_t index = std::find(rLambdas[(*cmapit)[1]].begin(), rLambdas[(*cmapit)[1]].begin() + offset, cmapit->front()) - rLambdas[(*cmapit)[1]].begin();
				if (index == offset) {
					index = std::find(rLambdas[(*cmapit)[1]].begin() + offset + 1, rLambdas[(*cmapit)[1]].end(), cmapit->front()) - rLambdas[(*cmapit)[1]].begin();
				}
				if (index == rLambdas[(*cmapit)[1]].size() || rLambdas[(*cmapit)[1]][index] != cmapit->front()) {
					ESINFO(ERROR) << "Different Lambdas on neighbour clusters.";
				}
				ptarget = rPoints[(*cmapit)[1]][index];
			}

			os << psource + (ptarget - psource) * instance.B1duplicity[d][i] << "\n";
		}
	}
	os << "\n";

	size_t cells = points;

	os << "CELLS " << cells << " " << 3 * cells << "\n";
	for (size_t e = 0; e < cells; e++) {
		os << "2 " << 2 * e << " " << 2 * e + 1 << "\n";
	}
	os << "\n";

	os << "CELL_TYPES " << cells << "\n";
	Element::CODE ecode = Element::CODE::LINE2;
	for (size_t n = 0; n < cells; ++n) {
		os << VTKWritter::ecode(ecode) << "\n";
	}
	os << "\n";

	os << "CELL_DATA " << cells << "\n";
	os << "SCALARS value float 1\n";
	os << "LOOKUP_TABLE default\n";
	for (size_t d = 0, offset = 0; d < instance.domains; d++) {
		auto start = std::upper_bound(instance.B1[d].I_row_indices.begin(), instance.B1[d].I_row_indices.end(), instance.block[Instance::CONSTRAINT::DIRICHLET]);
		auto end = std::upper_bound(instance.B1[d].I_row_indices.begin(), instance.B1[d].I_row_indices.end(), instance.block[Instance::CONSTRAINT::DIRICHLET] + instance.block[Instance::CONSTRAINT::EQUALITY_CONSTRAINTS]);
		for (eslocal i = start - instance.B1[d].I_row_indices.begin(); i < end - instance.B1[d].I_row_indices.begin(); i++, offset++) {
			os << instance.B1[d].V_values[i] << "\n";
		}
	}
	os << "\n";
}

void VTKLegacyDebugInfo::spaceFillingCurve(const SpaceFillingCurve &sfc, const std::vector<uint> &bucketsBorders)
{
	std::ofstream os(Esutils::createDirectory({ Logging::outputRoot(), "VTKLEGACY_DEBUG_OUTPUT" }) + "SFC.vtk");
	os << "# vtk DataFile Version 2.0\n";
	os << "EXAMPLE\n";
	os << "ASCII\n";
	os << "DATASET UNSTRUCTURED_GRID\n\n";

	size_t maxdepth = 0, n;
	while (sfc.hasLevel(maxdepth)) {
		++maxdepth;
	}
	n = 1 << maxdepth;

	size_t points = pow(n + 1, sfc.dimension());
	size_t cells = 0;
	sfc.iterateBuckets(bucketsBorders.front(), bucketsBorders.back(), [&] (size_t depth, size_t index) {
		++cells;
	});

	os << "POINTS " << points + cells << " float\n";

	for (size_t k = 0; k <= n; k++) {
		for (size_t j = 0; j <= n; j++) {
			for (size_t i = 0; i <= n; i++) {
				if (sfc.dimension() == 3) {
					os << sfc.origin().x + i * sfc.size().x / n << " " << sfc.origin().y + j * sfc.size().y / n << " " << sfc.origin().z + k * sfc.size().z / n << " \n";
				}
				if (k == 0 && sfc.dimension() == 2) {
					os << sfc.origin().x + i * sfc.size().x / n << " " << sfc.origin().y + j * sfc.size().y / n << " 0\n";
				}
			}
		}
	}

	Point step;
	step.x = sfc.size().x / n;
	step.y = sfc.size().y / n;
	step.z = sfc.size().z / n;

	size_t bstep;
	if (sfc.dimension() == 2) {
		sfc.iterateBuckets(bucketsBorders.front(), bucketsBorders.back(), [&] (size_t depth, size_t x, size_t y) {
			bstep = 1 << (maxdepth - depth);

			Point center;
			center.x += 2 * (sfc.origin().x + bstep * (x + 0) * step.x);
			center.x += 2 * (sfc.origin().x + bstep * (x + 1) * step.x);
			center.y += 2 * (sfc.origin().y + bstep * (y + 0) * step.y);
			center.y += 2 * (sfc.origin().y + bstep * (y + 1) * step.y);
			center /= 4;
			os << center.x << " " << center.y << " 0\n";
		});
	}

	if (sfc.dimension() == 3) {
		sfc.iterateBuckets(bucketsBorders.front(), bucketsBorders.back(), [&] (size_t depth, size_t x, size_t y, size_t z) {
			bstep = 1 << (maxdepth - depth);

			Point center;
			center.x += 4 * (sfc.origin().x + bstep * (x + 0) * step.x);
			center.x += 4 * (sfc.origin().x + bstep * (x + 1) * step.x);
			center.y += 4 * (sfc.origin().y + bstep * (y + 0) * step.y);
			center.y += 4 * (sfc.origin().y + bstep * (y + 1) * step.y);
			center.z += 4 * (sfc.origin().z + bstep * (z + 0) * step.z);
			center.z += 4 * (sfc.origin().z + bstep * (z + 1) * step.z);
			center /= 8;
			os << center.x << " " << center.y << " " << center.z << "\n";
		});
	}

	os << "\n";

	os << "CELLS " << 2 * cells - 1 << " " << cells + sfc.bucketSize() * cells + 3 * (cells - 1) << "\n";

	if (sfc.dimension() == 2) {
		sfc.iterateBuckets(bucketsBorders.front(), bucketsBorders.back(), [&] (size_t depth, size_t x, size_t y) {
			bstep = 1 << (maxdepth - depth);
			os << "4 ";
			os << (n + 1) * bstep * (y + 0) + bstep * (x + 0) << " ";
			os << (n + 1) * bstep * (y + 0) + bstep * (x + 1) << " ";
			os << (n + 1) * bstep * (y + 1) + bstep * (x + 1) << " ";
			os << (n + 1) * bstep * (y + 1) + bstep * (x + 0) << "\n";
		});
	}

	if (sfc.dimension() == 3) {
		sfc.iterateBuckets(bucketsBorders.front(), bucketsBorders.back(), [&] (size_t depth, size_t x, size_t y, size_t z) {
			bstep = 1 << (maxdepth - depth);
			os << "8 ";
			os << (n + 1) * (n + 1) * bstep * (z + 0) + (n + 1) * bstep * (y + 0) + bstep * (x + 0) << " ";
			os << (n + 1) * (n + 1) * bstep * (z + 0) + (n + 1) * bstep * (y + 0) + bstep * (x + 1) << " ";
			os << (n + 1) * (n + 1) * bstep * (z + 0) + (n + 1) * bstep * (y + 1) + bstep * (x + 1) << " ";
			os << (n + 1) * (n + 1) * bstep * (z + 0) + (n + 1) * bstep * (y + 1) + bstep * (x + 0) << " ";
			os << (n + 1) * (n + 1) * bstep * (z + 1) + (n + 1) * bstep * (y + 0) + bstep * (x + 0) << " ";
			os << (n + 1) * (n + 1) * bstep * (z + 1) + (n + 1) * bstep * (y + 0) + bstep * (x + 1) << " ";
			os << (n + 1) * (n + 1) * bstep * (z + 1) + (n + 1) * bstep * (y + 1) + bstep * (x + 1) << " ";
			os << (n + 1) * (n + 1) * bstep * (z + 1) + (n + 1) * bstep * (y + 1) + bstep * (x + 0) << "\n";
		});
	}

	size_t i = 0;
	sfc.iterateBuckets(bucketsBorders.front(), bucketsBorders.back(), [&] (size_t depth, size_t x, size_t y, size_t z) {
		if (++i < cells) {
			os << "2 " << points + i - 1 << " " << points + i << "\n";
		}
	});

	os << "\n";

	os << "CELL_TYPES " << 2 * cells - 1 << "\n";
	for (size_t i = 0; i < cells; i++) {
		if (sfc.dimension() == 2) {
			os << "9\n";
		}
		if (sfc.dimension() == 3) {
			os << "12\n";
		}
	}
	for (size_t i = 1; i < cells; i++) {
		os << "3\n";
	}
	os << "\n";

	os << "CELL_DATA " << 2 * cells - 1 << "\n";
	os << "SCALARS MPI int 1\n";
	os << "LOOKUP_TABLE default\n";
	for (int r = 0; r < environment->MPIsize; r++) {
		sfc.iterateBuckets(bucketsBorders[r], bucketsBorders[r + 1], [&] (size_t depth, size_t index) {
			os << r << "\n";
		});
	}
	for (size_t i = 1; i < cells; i++) {
		os << environment->MPIsize << "\n";
	}
	os << "\n";

	os.close();
}


