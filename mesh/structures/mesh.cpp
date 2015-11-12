#include "mesh.h"

#include "esinput.h"
#include "esoutput.h"

using namespace mesh;

Mesh::Mesh(int rank, int size):_elements(0), _fixPoints(0), _rank(rank), _size(size)
{
	_partPtrs.resize(2);
	_partPtrs[0] = 0;
	_partPtrs[1] = 0;
}

void Mesh::load(Input input, int argc, char** argv)
{
	switch (input) {
	case ANSYS: {
		esinput::Loader<esinput::Ansys> loader(argc, argv, _rank, _size);
		loader.load(*this);
		break;
	}
	case ESPRESO_INPUT: {
		esinput::Loader<esinput::Esdata> loader(argc, argv, _rank, _size);
		loader.load(*this);
		break;
	}
	case MESH_GENERATOR: {
		esinput::Loader<esinput::MeshGenerator> loader(argc, argv, _rank, _size);
		loader.load(*this);
		break;
	}
	case OPENFOAM: {
		esinput::Loader<esinput::OpenFOAM> loader(argc, argv, _rank, _size);
		loader.load(*this);
		break;
	}
	default: {
		std::cerr << "Unknown input type.\n";
		exit(EXIT_FAILURE);
	}
	}
}

void Mesh::store(Output output, const std::string &path, std::vector<std::vector<double> > &displacement, double shrinkSubdomain, double shringCluster) const
{
	switch (output) {
	case ESPRESO_OUTPUT: {
		esoutput::Store<esoutput::Esdata> s(*this, path);
		s.store(shrinkSubdomain, shringCluster);
		std::cout << "Warning: ESPRESO output format does not support the displacement.\n";
		break;
	}
	case VTK_FULL: {
		esoutput::Store<esoutput::VTK_Full> s(*this, path);
		s.store(displacement, shrinkSubdomain, shringCluster);
		break;
	}
	case VTK_SURFACE: {
		esoutput::Store<esoutput::VTK_Surface> s(*this, path);
		s.store(displacement, shrinkSubdomain, shringCluster);
		break;
	}
	default: {
		std::cerr << "Unknown output type.\n";
		exit(EXIT_FAILURE);
	}
	}
}

void Mesh::store(Output output, const std::string &path, double shrinkSubdomain, double shringCluster) const
{
	switch (output) {
	case ESPRESO_OUTPUT: {
		esoutput::Store<esoutput::Esdata> s(*this, path);
		s.store(shrinkSubdomain, shringCluster);
		break;
	}
	case VTK_FULL: {
		esoutput::Store<esoutput::VTK_Full> s(*this, path);
		s.store(shrinkSubdomain, shringCluster);
		break;
	}
	case VTK_SURFACE: {
		esoutput::Store<esoutput::VTK_Surface> s(*this, path);
		s.store(shrinkSubdomain, shringCluster);
		break;
	}
	default: {
		std::cerr << "Unknown output type.\n";
		exit(EXIT_FAILURE);
	}
	}
}

void Mesh::partitiate(eslocal parts, eslocal fixPoints)
{
	_partPtrs.resize(parts + 1);
	_coordinates.localClear();
	_coordinates.localResize(parts);

	// Call METIS to get partition of a whole mesh
	eslocal *elementPartition = getPartition(0, _elements.size(), parts);

	// Rearrange mesh's elements
	partitiate(elementPartition);

	delete[] elementPartition;

	if (fixPoints > 0) {
		computeFixPoints(fixPoints);
	}

	computeBoundaries();
}

void Mesh::computeBoundaries()
{
	_subdomainBoundaries.clear();
	_subdomainBoundaries.resize(_coordinates.size());

	for (size_t p = 0; p < parts(); p++) {
		for (eslocal e = _partPtrs[p]; e < _partPtrs[p + 1]; e++) {
			for (size_t n = 0; n < _elements[e]->size(); n++) {
				_subdomainBoundaries[_coordinates.clusterIndex(_elements[e]->node(n), p)].insert(p);
			}
		}
	}
}

void Mesh::computeFixPoints(eslocal fixPoints)
{
	_fixPoints.resize(parts(), std::vector<eslocal>(fixPoints));

#ifndef DEBUG
	cilk_for (eslocal i = 0; i < parts(); i++) {
#else
	for (eslocal i = 0; i < parts(); i++) {
#endif
		eslocal *eSubPartition = getPartition(_partPtrs[i], _partPtrs[i + 1], fixPoints);

		for (eslocal j = 0; j < fixPoints; j++) {
			_fixPoints[i][j] = getCentralNode(_partPtrs[i], _partPtrs[i + 1], eSubPartition, i, j);
		}
		std::sort(_fixPoints[i].begin(), _fixPoints[i].end());

		delete[] eSubPartition;
	}
}

eslocal* Mesh::getPartition(eslocal first, eslocal last, eslocal parts) const
{
	if (parts == 1) {
		eslocal *ePartition = new eslocal[last - first];
		for (eslocal i = first; i < last; i++) {
			ePartition[i] = 0;
		}
		return ePartition;
	}
	// INPUTS
	eslocal ncommon, eSize, nSize, *e, *n, options[METIS_NOPTIONS];

	// OUTPUTS
	eslocal objval, *ePartition, *nPartition;

	// FILL INPUT VARIABLES
	////////////////////////////////////////////////////////////////////////////

	// number of common nodes to be neighbor
	ncommon = 3;

	// There is probably BUG in METIS numbering or I do not understand documentation.
	// The solution is increase the size of 'nodesCount' and keep the default numbering
	//options[METIS_OPTION_NUMBERING] = coordinates.getNumbering();
	METIS_SetDefaultOptions(options);
	//TODO:
	options[METIS_OPTION_CONTIG]  = 1;
//	options[METIS_OPTION_MINCONN] = 1;
//	options[METIS_OPTION_NITER]   = 20;
//	options[METIS_PTYPE_KWAY]     = 1;

	eSize = last - first;
	nSize = _coordinates.clusterSize();

	// create array storing pointers to elements' nodes
	e = new eslocal[eSize + 1];
	e[0] = 0;
	for (eslocal i = first, index = 0; i < last; i++, index++) {
		e[index + 1] = e[index] + _elements[i]->size();
	}

	// create array of nodes
	n = new eslocal[e[eSize]];
	for (eslocal i = first, index = 0; i < last; i++, index++) {
		_elements[i]->fillNodes(n + e[index]);
	}

	// PREPARE OUTPUT VARIABLES
	////////////////////////////////////////////////////////////////////////////
	ePartition = new eslocal[eSize];
	nPartition = new eslocal[nSize];

	eslocal result = METIS_PartMeshDual(
			&eSize,
			&nSize,
			e,
			n,
			NULL,		// weights of nodes
			NULL,		// size of nodes
			&ncommon,
			&parts,
			NULL,		// weights of parts
			options,
			&objval,
			ePartition,
			nPartition);
	checkMETISResult(result);

	delete[] e;
	delete[] n;
	delete[] nPartition;

	return ePartition;
}

void Mesh::partitiate(eslocal *ePartition)
{
	_partPtrs[0] = 0;

	Element *e;
	eslocal p;
	for (size_t part = 0; part < _partPtrs.size() - 1; part++) {
		eslocal index = _partPtrs[part];	// index of last ordered element
		for (size_t i = _partPtrs[part]; i < _elements.size(); i++) {
			if (ePartition[i] == part) {
				if (i == index) {
					index++;
				} else {
					e = _elements[i];
					_elements[i] = _elements[index];
					_elements[index] = e;
					p = ePartition[i];
					ePartition[i] = ePartition[index];
					ePartition[index] = p;
					index++;
				}
			}
		}
		_partPtrs[part + 1] = index;
		computeLocalIndices(part);
	}
}

void Mesh::computeLocalIndices(size_t part)
{
	std::vector<eslocal> nodeMap(_coordinates.clusterSize(), -1);

	// Compute mask of nodes
	for (eslocal e = _partPtrs[part]; e < _partPtrs[part + 1]; e++) {
		for (size_t n = 0; n < _elements[e]->size(); n++) {
			nodeMap[_elements[e]->node(n)] = 1;
		}
	}

	// re-index nodes
	eslocal nSize = 0;
	for (eslocal k = 0; k < _coordinates.clusterSize(); k++) {
		if (nodeMap[k] == 1) {
			nodeMap[k] = nSize++;
		}
	}

	for (eslocal e = _partPtrs[part]; e < _partPtrs[part + 1]; e++) {
		_elements[e]->setLocalIndices(nodeMap);
	}

	_coordinates.computeLocal(part, nodeMap, nSize);
}

eslocal Mesh::getCentralNode(
		eslocal first,
		eslocal last,
		eslocal *ePartition,
		eslocal part,
		eslocal subpart) const
{
	// Compute CSR format of symmetric adjacency matrix
	////////////////////////////////////////////////////////////////////////////
	std::vector<std::set<eslocal> > neighbours(_coordinates.localSize(part));
	for (eslocal i = first, index = 0; i < last; i++, index++) {
		if (ePartition[index] == subpart) {
			for (size_t j = 0; j < _elements[i]->size(); j++) {
				std::vector<eslocal> neigh = _elements[i]->getNeighbours(j);
				for (size_t k = 0; k < neigh.size(); k++) {
					if (_elements[i]->node(j) < neigh[k]) {
						neighbours[_elements[i]->node(j)].insert(neigh[k]);
					}
				}
			}
		}
	}
	eslocal nonZeroValues = 0;
	for (size_t i = 0; i < neighbours.size(); i++) {
		nonZeroValues += neighbours[i].size();
	}

	float *a;
	eslocal *ia, *ja;
	a = new float[nonZeroValues];
	ia = new eslocal[_coordinates.localSize(part) + 1];
	ja = new eslocal[nonZeroValues];

	eslocal i = 0;
	std::set<eslocal>::const_iterator it;
	for (size_t n = 0; n < neighbours.size(); n++) {
		std::set<eslocal> &values = neighbours[n];
		ia[n] = i;
		for (it = values.begin(); it != values.end(); ++it, i++) {
			a[i] = 1;
			ja[i] = *it;
		}
	}
	ia[neighbours.size()] = i;

	eslocal nSize = _coordinates.localSize(part);
	float *x, *y, *swap;
	x = new float[nSize];
	y = new float[nSize];

	// Initial vector
	for (eslocal xi = 0; xi < nSize; xi++) {
		x[xi] = 1. / nSize;
	}
	float last_l = nSize, l = 1;
	eslocal incr = 1;

	while (fabs((l - last_l) / l) > 1e-6) {
		mkl_cspblas_scsrsymv("U", &nSize, a, ia, ja, x, y);
		last_l = l;
		l = snrm2(&nSize, y, &incr);
		cblas_sscal(nSize, 1 / l, y, incr);
		swap = x;
		x = y;
		y = swap;
	}
	eslocal result = cblas_isamax(nSize, x, incr);

	delete[] a;
	delete[] ia;
	delete[] ja;
	delete[] x;
	delete[] y;

	return result;
}

void Mesh::checkMETISResult(eslocal result) const
{
	switch (result) {
	case METIS_ERROR_INPUT:
		fprintf(stderr, "An input for METIS procedure is incorrect.\n");
		exit(EXIT_FAILURE);
	case METIS_ERROR_MEMORY:
		fprintf(stderr, "There is not enough memory for compute a partition.\n");
		exit(EXIT_FAILURE);
	case METIS_ERROR:
		fprintf(stderr, "METIS fail computation.\n");
		exit(EXIT_FAILURE);
	}
}

void Mesh::checkMKLResult(eslocal result) const
{
	switch (result) {
	case 0:
		return;
	default:
		std::cerr << "MKL error: " << result << ".\n";
		exit(EXIT_FAILURE);
	}
}

Mesh::~Mesh()
{
	for (size_t i = 0; i < _elements.size(); i++) {
		delete _elements[i];
	}
}

void Mesh::saveNodeArray(eslocal *nodeArray, size_t part)
{
	size_t p = 0;
	for (eslocal i = _partPtrs[part]; i < _partPtrs[part + 1]; i++) {
		_elements[i]->fillNodes(nodeArray + p);
		p += _elements[i]->size();
	}
}

bool isOuterFace(
		std::vector<std::vector<eslocal> > &nodesElements,
		std::vector<eslocal> &face)
{
	std::vector<eslocal> result(nodesElements[face[0]]);
	std::vector<eslocal>::iterator it = result.end();

	for (size_t i = 1; i < face.size(); i++) {
		std::vector<eslocal> tmp(result.begin(), it);
		it = std::set_intersection(tmp.begin(), tmp.end(),
				nodesElements[face[i]].begin(), nodesElements[face[i]].end(),
				result.begin());
		if (it - result.begin() == 1) {
			return true;
		}
	}
	return false;
}

eslocal isOnBoundary(
		std::vector<std::vector<eslocal> > &nodesElements,
		std::vector<eslocal> &face,
		const std::vector<eslocal> &partPtrs,
		eslocal differentParts)
{
	eslocal NOT_ON_BOUNDARY = -1;
	std::vector<eslocal> result(nodesElements[face[0]]);
	std::vector<eslocal>::iterator it = result.end();

	for (size_t i = 1; i < face.size(); i++) {
		std::vector<eslocal> tmp(result.begin(), it);
		it = std::set_intersection(tmp.begin(), tmp.end(),
				nodesElements[face[i]].begin(), nodesElements[face[i]].end(),
				result.begin());
		if (it - result.begin() == 1) {
			return NOT_ON_BOUNDARY;
		}
	}

	eslocal counter = 0;
	eslocal minPart = result[0];
	for (size_t r = 1; r < it - result.begin(); r++) {
		for (size_t i = 1; i < partPtrs.size() - 1; i++) {
			if (result[r - 1] < partPtrs[i] && partPtrs[i] <= result[r]) {
				if (minPart > result[r]) {
					minPart = result[r];
				}
				counter++;
				break;
			}
		}
	}
	return (counter >= differentParts) ? minPart : NOT_ON_BOUNDARY;
}

void Mesh::getSurface(SurfaceMesh &surface) const
{
	// vector of faces in all parts
	std::vector<std::vector<std::vector<eslocal> > > faces(parts());
	// number of elements in all parts
	std::vector<size_t> elementsCount(parts(), 0);

	if (parts() < 1) {
		std::cerr << "Internal error: _partPtrs.size()\n";
		exit(EXIT_FAILURE);
	}
#ifndef DEBUG
	cilk_for (size_t i = 0; i < parts(); i++) {
#else
	for (size_t i = 0; i < parts(); i++) {
#endif
		// Compute nodes' adjacent elements
		std::vector<std::vector<eslocal> > nodesElements(_coordinates.localSize(i));
		for (eslocal j = _partPtrs[i]; j < _partPtrs[i + 1]; j++) {
			for (size_t k = 0; k < _elements[j]->size(); k++) {
				nodesElements[_elements[j]->node(k)].push_back(j);
			}
		}

		// compute number of elements and fill used nodes
		for (eslocal j = _partPtrs[i]; j < _partPtrs[i + 1]; j++) {
			for (size_t k = 0; k < _elements[j]->faces(); k++) {
				std::vector<eslocal> face = _elements[j]->getFace(k);
				if (isOuterFace(nodesElements, face)) {
					for (size_t f = 0; f < face.size(); f++) {
						face[f] = _coordinates.clusterIndex(face[f], i);
					}
					faces[i].push_back(face);
					if (face.size() == 3) {
						elementsCount[i] += 1;
					}
					if (face.size() == 4) {
						elementsCount[i] += 2;
					}
				}
			}
		}
	}

	surface.coordinates() = _coordinates;

	size_t count = 0;
	for (size_t i = 0; i + 1 < _partPtrs.size(); i++) {
		count += elementsCount[i];
	}

	surface._elements.reserve(count);
	surface._partPtrs.clear();
	surface._partPtrs.reserve(_partPtrs.size());

	// create surface mesh
	surface._partPtrs.push_back(0); //(surface._elements.size());
	for (size_t i = 0; i + 1 < _partPtrs.size(); i++) {
		for (size_t j = 0; j < faces[i].size(); j++) {
			std::vector<eslocal> &face = faces[i][j];
			if (face.size() == 3) {
				surface._elements.push_back(new Triangle(&face[0]));
			}
			// divide square to triangles
			if (face.size() == 4) {
				size_t min = 0;
				for (size_t p = 1; p < 4; p++) {
					if (_coordinates[face[p]] < _coordinates[face[min]]) {
						min = p;
					}
				}
				if (min % 2 == 0) {
					surface._elements.push_back(new Triangle(&face[0]));
					face[1] = face[0];
					surface._elements.push_back(new Triangle(&face[1]));
				} else {
					surface._elements.push_back(new Triangle(&face[1]));
					face[2] = face[3];
					surface._elements.push_back(new Triangle(&face[0]));
				}
			}
		}
		surface._partPtrs.push_back(surface._elements.size());
		surface.computeLocalIndices(surface._partPtrs.size() - 2);

	}

	surface.computeBoundaries();

}

void Mesh::getCommonFaces(CommonFacesMesh &commonFaces) const
{
	// vector of faces in all parts
	std::vector<std::vector<std::vector<eslocal> > > faces(parts());
	// number of elements in all parts
	std::vector<size_t> elementsCount(parts(), 0);

	if (parts() < 1) {
		std::cerr << "Internal error: _partPtrs.size()\n";
		exit(EXIT_FAILURE);
	}

	std::vector<std::vector<eslocal> > nodesElements(_coordinates.size());
	for (size_t i = 0; i < parts(); i++) {
		// Compute nodes' adjacent elements
		for (eslocal j = _partPtrs[i]; j < _partPtrs[i + 1]; j++) {
			for (size_t k = 0; k < _elements[j]->size(); k++) {
				nodesElements[_coordinates.clusterIndex(_elements[j]->node(k), i)].push_back(j);
			}
		}
	}

#ifndef DEBUG
	cilk_for (size_t i = 0; i < parts(); i++) {
#else
	for (size_t i = 0; i < parts(); i++) {
#endif
		// compute number of elements and fill used nodes
		for (eslocal j = _partPtrs[i]; j < _partPtrs[i + 1]; j++) {
			for (size_t k = 0; k < _elements[j]->faces(); k++) {
				std::vector<eslocal> face = _elements[j]->getFace(k);
				for (size_t f = 0; f < face.size(); f++) {
					face[f] = _coordinates.clusterIndex(face[f], i);
				}
				if (isOnBoundary(nodesElements, face, _partPtrs, 1) >= 0) {
					faces[i].push_back(face);
					elementsCount[i] += 1;
				}
			}
		}
	}

	commonFaces.coordinates() = _coordinates;

	size_t count = 0;
	for (size_t i = 0; i < parts(); i++) {
		count += elementsCount[i];
	}
	commonFaces._elements.reserve(count);
	commonFaces._partPtrs.clear();
	commonFaces._partPtrs.reserve(_partPtrs.size());

	// create surface mesh
	commonFaces._partPtrs.push_back(commonFaces._elements.size());
	for (size_t i = 0; i + 1 < _partPtrs.size(); i++) {
		for (size_t j = 0; j < faces[i].size(); j++) {
			std::vector<eslocal> &face = faces[i][j];
			if (faces[i][j].size() == 3) {
				commonFaces._elements.push_back(new Triangle(&face[0]));
			}
			if (faces[i][j].size() == 4) {
				commonFaces._elements.push_back(new Square(&face[0]));
			}
		}
		commonFaces._partPtrs.push_back(commonFaces._elements.size());
		commonFaces.computeLocalIndices(commonFaces._partPtrs.size() - 1);
	}
}

void Mesh::getCornerLines(CornerLinesMesh &cornerLines) const
{
	if (parts() < 1) {
		std::cerr << "Internal error: _partPtrs.size()\n";
		exit(EXIT_FAILURE);
	}

	std::vector<std::vector<eslocal> > nodesElements(_coordinates.size());
	for (size_t i = 0; i < parts(); i++) {
		// Compute nodes' adjacent elements
		for (eslocal j = _partPtrs[i]; j < _partPtrs[i + 1]; j++) {
			for (size_t k = 0; k < _elements[j]->size(); k++) {
				nodesElements[_coordinates.clusterIndex(_elements[j]->node(k), i)].push_back(j);
			}
		}
	}

	std::vector<std::set<eslocal> > nodeParts(_coordinates.size());

	for (size_t i = 0; i < parts(); i++) {
		for (size_t j = 0; j < _coordinates.localSize(i); j++) {
			nodeParts[_coordinates.clusterIndex(j, i)].insert(i);
		}
	}

	for (size_t i = 0; i < parts(); i++) {
		for (eslocal j = _partPtrs[i]; j < _partPtrs[i + 1]; j++) {
			for (size_t k = 0; k < _elements[j]->faces(); k++) {
				std::vector<eslocal> face = _elements[j]->getFace(k);
				for (size_t f = 0; f < face.size(); f++) {
					face[f] = _coordinates.clusterIndex(face[f], i);
				}
				if (isOuterFace(nodesElements, face)) {
					for (size_t f = 0; f < face.size(); f++) {
						nodeParts[face[f]].insert(parts());
					}
				}
			}
		}
	}

	std::vector<std::set<eslocal> > neighbours(_coordinates.size());
	for (size_t p = 0; p < parts(); p++) {
		for (eslocal j = _partPtrs[p]; j < _partPtrs[p + 1]; j++) {
			for (size_t k = 0; k < _elements[j]->size(); k++) {
				std::vector<eslocal> n = _elements[j]->getNeighbours(k);
				eslocal c1 = _coordinates.clusterIndex(_elements[j]->node(k), p);
				for (size_t i = 0; i < n.size(); i++) {
					eslocal c2 = _coordinates.clusterIndex(n[i], p);
					if (c1 < c2) {
						neighbours[c1].insert(c2);
					}
				}
			}
		}
	}

	cornerLines.coordinates() = _coordinates;

	std::vector<eslocal> result;
	std::vector<eslocal>::iterator end;
	std::vector<eslocal>::iterator pit;
	std::vector<std::vector<eslocal> > pairs(parts());

	std::set<eslocal>::iterator it;
	for (size_t i = 0; i < neighbours.size(); i++) {
		for (it = neighbours[i].begin(); it != neighbours[i].end(); ++it) {
			result.resize(nodeParts[i].size());
			end = std::set_intersection(
				nodeParts[i].begin(), nodeParts[i].end(),
				nodeParts[*it].begin(), nodeParts[*it].end(),
				result.begin());
			if (end - result.begin() >= 3) {
				for (pit = result.begin(); pit != end && *pit < parts(); ++pit) {
					pairs[*pit].push_back(i);
					pairs[*pit].push_back(*it);
				}
			}
		}
	}

	eslocal size = 0;
	for (size_t i = 0; i < parts(); i++) {
		size += pairs[i].size() / 2;
	}

	cornerLines._elements.reserve(size);
	cornerLines._partPtrs.clear();
	cornerLines._partPtrs.reserve(_partPtrs.size());

	cornerLines._partPtrs.push_back(cornerLines._elements.size());
	for (size_t i = 0; i < parts(); i++) {
		for (size_t j = 0; j < pairs[i].size(); j += 2) {
			cornerLines._elements.push_back(new Line(&pairs[i][j]));
		}
		cornerLines._partPtrs.push_back(cornerLines._elements.size());
		cornerLines.computeLocalIndices(cornerLines._partPtrs.size() - 1);
	}

}

void Mesh::computeCorners(Boundaries &boundaries, eslocal number, bool corners, bool edges, bool faces) const
{
	if (parts() < 1) {
		std::cerr << "Internal error: _partPtrs.size()\n";
		exit(EXIT_FAILURE);
	}
	if (!corners && !edges && !faces) {
		return;
	}

	// node to element
	std::vector<std::vector<eslocal> > nodesElements(_coordinates.size());
	// node to neighbors nodes
	std::vector<std::set<eslocal> > neighbours(_coordinates.size());

	for (size_t i = 0; i < parts(); i++) {
		for (eslocal j = _partPtrs[i]; j < _partPtrs[i + 1]; j++) {
			for (size_t k = 0; k < _elements[j]->size(); k++) {
				// add node's adjacent element
				nodesElements[_coordinates.clusterIndex(_elements[j]->node(k), i)].push_back(j);
			}
		}
	}

	// vector of faces
	std::vector<std::vector<eslocal> > commonFaces;

#ifndef DEBUG
	cilk_for (eslocal i = 0; i < parts(); i++) {
#else
	for (eslocal i = 0; i < parts(); i++) {
#endif
		// compute number of elements and fill used nodes
		for (eslocal j = _partPtrs[i]; j < _partPtrs[i + 1]; j++) {
			for (size_t k = 0; k < _elements[j]->faces(); k++) {
				std::vector<eslocal> face = _elements[j]->getFace(k);
				for (size_t f = 0; f < face.size(); f++) {
					face[f] = _coordinates.clusterIndex(face[f], i);
				}
				if (isOnBoundary(nodesElements, face, _partPtrs, 1) == j) {
					commonFaces.push_back(face);
				}
			}
		}
	}

	// create mesh from common faces
	Mesh cfm(_rank, _size);
	Element *e;
	if (faces) {
		cfm.coordinates() = _coordinates;
		cfm._elements.reserve(commonFaces.size());
	}
	std::vector<std::vector<eslocal> > nodesFaces(_coordinates.size());
	for (size_t i = 0; i < commonFaces.size(); i++) {
		if (commonFaces[i].size() == 3) {
			e = new Triangle(&commonFaces[i][0]);
		}
		if (commonFaces[i].size() == 4) {
			e = new Square(&commonFaces[i][0]);
		}
		if (faces) {
			cfm._elements.push_back(e);
		}
		// preparation for corners on edges
		for (size_t j = 0; j < e->size(); j++) {
			std::vector<eslocal> neigh = e->getNeighbours(j);
			eslocal index = e->node(j);
			for (size_t n = 0; n < neigh.size(); n++) {
				if (index < neigh[n]) {
					neighbours[index].insert(neigh[n]);
				}
			}
			nodesFaces[index].push_back(i);
		}
	}
	if (faces) {
		cfm._partPtrs.back() = cfm._elements.size();
		cfm.computeLocalIndices(cfm._partPtrs.size() - 1);
		cfm.computeFixPoints(number);
		for (size_t p = 0; p < cfm.parts(); p++) {
			for (size_t i = 0; i < cfm.getFixPoints()[p].size(); i++) {
				boundaries.setCorner(cfm.coordinates().clusterIndex(cfm.getFixPoints()[p][i], 0));
			}
		}
	}

	Mesh clm(_rank, _size);
	if (edges) {
		clm.coordinates() = _coordinates;
	}

	std::vector<eslocal> result;
	std::vector<eslocal>::iterator end;
	std::vector<eslocal> pairs;
	std::vector<eslocal> pair(2);
	std::vector<eslocal> nodeCounter(_coordinates.size(), 0);

	std::set<eslocal>::iterator it;
	for (size_t i = 0; i < neighbours.size(); i++) {
		for (it = neighbours[i].begin(); it != neighbours[i].end(); ++it) {
			pair[0] = i;
			pair[1] = *it;
			result.resize(boundaries[i].size());
			end = std::set_intersection(
				boundaries[i].begin(), boundaries[i].end(),
				boundaries[*it].begin(), boundaries[*it].end(),
				result.begin());
			if (end - result.begin() >= 3 && isOnBoundary(nodesElements, pair, _partPtrs, 2)) {
				pairs.push_back(pair[0]);
				pairs.push_back(pair[1]);
				nodeCounter[pair[0]]++;
				nodeCounter[pair[1]]++;
				continue;
			}
			if (isOuterFace(nodesFaces, pair)) {
				pairs.push_back(pair[0]);
				pairs.push_back(pair[1]);
				nodeCounter[pair[0]]++;
				nodeCounter[pair[1]]++;
			}
		}
	}

	if (edges) {
		clm._elements.reserve(pairs.size() / 2);
	}

	for (size_t j = 0; j < pairs.size(); j += 2) {
		if (corners) {
			if (nodeCounter[pairs[j]] > 3 || nodeCounter[pairs[j + 1]] > 3) {
				if (nodeCounter[pairs[j]] > 3) {
					boundaries.setCorner(pairs[j]);
				}
				if (nodeCounter[pairs[j + 1]] > 3) {
					boundaries.setCorner(pairs[j + 1]);
				}
				continue;
			}
		}
		if (edges) {
			clm._elements.push_back(new Line(&pairs[j]));
		}
	}
	if (edges) {
		clm._partPtrs.back() = clm._elements.size();
		clm.computeLocalIndices(clm._partPtrs.size() - 1);
		clm.computeFixPoints(number);
		for (size_t p = 0; p < clm.parts(); p++) {
			for (size_t i = 0; i < clm.getFixPoints()[p].size(); i++) {
				boundaries.setCorner(clm.coordinates().clusterIndex(clm.getFixPoints()[p][i], 0));
			}
		}
	}
}

void SurfaceMesh::elasticity(DenseMatrix &K, size_t part) const
{
	eslocal nK = Point::size() * _coordinates.localSize(part);
	eslocal eSize = _partPtrs[part + 1] - _partPtrs[part];
	K.resize(nK, nK);
	std::vector<double> nodes(nK);
	std::vector<eslocal> elems(3 * eSize);

	for (size_t i = 0; i < _coordinates.localSize(part); i++) {
		&nodes[i * Point::size()] << _coordinates.get(i, part);
	}
	for (size_t i = _partPtrs[part], index = 0; i < _partPtrs[part + 1];
			i++, index++) {
		// TODO: various data types int32_t and int64_t
		// _elements[i]->fillNodes(&elems[3 * i]); CANNOT be used
		for (size_t j = 0; j < _elements[i]->size(); j++) {
			elems[3 * index + j] = _elements[i]->node(j);
		}
	}

	bem4i::getLameSteklovPoincare(
			K.values(),
			_coordinates.localSize(part),
			&nodes[0],
			eSize,
			&elems[0],
			0.3,			// nu
			2.1e5,			// E
			3,				// order near
			4,				// order far
			false			// verbose
			);
}

void SurfaceMesh::integrateUpperFaces(std::vector<double> &f, size_t part) const
{
	double hight_z = 29.99999999;
	Point p0, p1, p2, v10, v20;
	double Area_h;

	for (size_t i = _partPtrs[part]; i < _partPtrs[part + 1]; i++) {
		bool flag_edgeOnTop = true;
		for (size_t j = 0; j < _elements[i]->size(); j++) {
			if (_coordinates.get(_elements[i]->node(j), part).z < hight_z) {
				flag_edgeOnTop = false;
				continue;
			}
		}
		if (flag_edgeOnTop) {
			p0 = _coordinates.get(_elements[i]->node(0), part);
			p1 = _coordinates.get(_elements[i]->node(1), part);
			p2 = _coordinates.get(_elements[i]->node(2), part);
			v10 = p1 - p0;
			v20 = p2 - p0;

			Area_h = 0.5
					* (v10.y * v20.z - v20.y * v10.z + v20.x * v10.z
					-  v10.x * v20.z + v10.x * v20.y - v20.x * v10.y);

			for (size_t k = 0; k < 3; k++) {
				f[3 * _elements[i]->node(k) + 2] += (1. / 3.) * Area_h;
			}
		}
	}
}

std::ostream& mesh::operator<<(std::ostream& os, const Mesh &m)
{
	for (size_t i = 0; i < m._elements.size(); i++) {
		os << *(m._elements[i]) << "\n";
	}
	return os;
}
