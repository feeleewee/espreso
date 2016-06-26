
#include "generator.h"

namespace espreso {
namespace input {


static void setCluster(size_t cluster[], const PlaneSettings &settings)
{
	if (settings.clusters[0] * settings.clusters[1] != settings.size) {
		ESINFO(GLOBAL_ERROR) << "The number of clusters(" << settings.clusters[0] * settings.clusters[1]
							<< ") does not accord the number of MPI processes(" << settings.size << ").";
	}
	eslocal index = 0, i = 0;
	for (size_t y = 0; y < settings.clusters[1]; y++) {
		for (size_t x = 0; x < settings.clusters[0]; x++) {
			if (settings.index == index++) {
				cluster[0] = x;
				cluster[1] = y;
				return;
			}
		}
	}
}

template<class TElement>
PlaneGenerator<TElement>::PlaneGenerator(Mesh &mesh, const PlaneSettings &settings)
	: UniformGenerator<TElement>(mesh, settings), _settings(settings)
{
	switch (settings.eType) {
	case ElementType::SQUARE4:
	case ElementType::SQUARE8:
	case ElementType::TRIANGLE3:
	case ElementType::TRIANGLE6:
		setCluster(_cluster, _settings);
		break;
	default:
		ESINFO(GLOBAL_ERROR) << "Plane does not support chosen element type";
	}

}


template<class TElement>
void PlaneGenerator<TElement>::points(Coordinates &coordinates, size_t &DOFs)
{
	DOFs = this->_DOFs;

	eslocal cNodes[3];
	esglobal gNodes[3];

	UniformUtils<TElement>::clusterNodesCount(_settings, cNodes);
	CubeUtils<TElement>::globalNodesCount(_settings, gNodes);

	coordinates.clear();
	coordinates.reserve(cNodes[0] * cNodes[1]);

	esglobal cs[2], ce[2];
	double step[2];
	for (eslocal i = 0; i < 2; i++) {
		cs[i] = (cNodes[i] - 1) * _cluster[i];
		ce[i] = (cNodes[i] - 1) * (_cluster[i] + 1);
	}
	for (eslocal i = 0; i < 2; i++) {
		step[i] = _settings.problemLength[i] / ((cNodes[i] - 1) * _settings.clusters[i]);
	}

	for (esglobal y = cs[1]; y <= ce[1]; y++) {
		for (esglobal x = cs[0]; x <= ce[0]; x++) {
			coordinates.add(
				Point(x * step[0], y * step[1], 0),
				(y - cs[1]) * cNodes[0] + (x - cs[0]),
				y * gNodes[0] + x
			);
		}
	}
}

template<class TElement>
void PlaneGenerator<TElement>::elementsMaterials(std::vector<Element*> &elements)
{
	esglobal cubeElements[3], partSize[3], cOffset[3], offset[3];
	eslocal subdomain[3], element[3], material, counter;

	for (size_t i = 0; i < 3; i++) {
		cubeElements[i] = _settings.clusters[i] * _settings.subdomainsInCluster[i] * _settings.elementsInSubdomain[i];
		cOffset[i] = _cluster[i] * _settings.subdomainsInCluster[i] * _settings.elementsInSubdomain[i];
		partSize[i] = std::ceil(cubeElements[i] / (double)_settings.materialsLayers[i]);
	}

	counter = 0;
	for (subdomain[2] = 0; subdomain[2] < _settings.subdomainsInCluster[2]; subdomain[2]++) {
			for (subdomain[1] = 0; subdomain[1] < _settings.subdomainsInCluster[1]; subdomain[1]++) {
				for (subdomain[0] = 0; subdomain[0] < _settings.subdomainsInCluster[0]; subdomain[0]++) {

					for (element[2] = 0; element[2] < _settings.elementsInSubdomain[2]; element[2]++) {
						for (element[1] = 0; element[1] < _settings.elementsInSubdomain[1]; element[1]++) {
							for (element[0] = 0; element[0] < _settings.elementsInSubdomain[0]; element[0]++) {

								material = 0;
								for (eslocal i = 0; i < 3; i++) {
									offset[i] = cOffset[i] + subdomain[i] * _settings.elementsInSubdomain[i] + element[i];
									if (offset[i] / partSize[i] % 2 == 1) {
										material = (material + 1) % 2;
									}
								}
								for (size_t e = 0; e < TElement::subelements; e++) {
									elements[counter++]->setParam(Element::MATERIAL, material);
								}
							}
						}
					}

				}
			}
	}

}

template<class TElement>
void PlaneGenerator<TElement>::boundaryConditions(Coordinates &coordinates, std::vector<BoundaryCondition*> &conditions)
{
	CoordinatesProperty &dirichlet_x = coordinates.property(DIRICHLET_X);
	CoordinatesProperty &dirichlet_y = coordinates.property(DIRICHLET_Y);
	CoordinatesProperty &dirichlet_z = coordinates.property(DIRICHLET_Z);
	CoordinatesProperty &forces_x = coordinates.property(FORCES_X);
	CoordinatesProperty &forces_y = coordinates.property(FORCES_Y);
	CoordinatesProperty &forces_z = coordinates.property(FORCES_Z);

	eslocal nodes[3];
	UniformUtils<TElement>::clusterNodesCount(_settings, nodes);

	if (_cluster[0] == 0) {
		eslocal index = 0;
		for (eslocal y = 0; y < nodes[1]; y++) {
			if (_settings.boundaryCondition[CubeSettings::TOP * 6 + DIRICHLET_X] != std::numeric_limits<double>::infinity()) {
				dirichlet_x[index] = _settings.boundaryCondition[CubeSettings::TOP * 6 + DIRICHLET_X];
			}
			if (_settings.boundaryCondition[CubeSettings::TOP * 6 + FORCES_X] != std::numeric_limits<double>::infinity()) {
				forces_x[index] = _settings.boundaryCondition[CubeSettings::TOP * 6 + FORCES_X];
			}
			if (_settings.boundaryCondition[CubeSettings::TOP * 6 + DIRICHLET_Y] != std::numeric_limits<double>::infinity()) {
				dirichlet_y[index] = _settings.boundaryCondition[CubeSettings::TOP * 6 + DIRICHLET_Y];
			}
			if (_settings.boundaryCondition[CubeSettings::TOP * 6 + FORCES_Y] != std::numeric_limits<double>::infinity()) {
				forces_y[index] = _settings.boundaryCondition[CubeSettings::TOP * 6 + FORCES_Y];
			}
			if (_settings.boundaryCondition[CubeSettings::TOP * 6 + DIRICHLET_Z] != std::numeric_limits<double>::infinity()) {
				dirichlet_z[index] = _settings.boundaryCondition[CubeSettings::TOP * 6 + DIRICHLET_Z];
			}
			if (_settings.boundaryCondition[CubeSettings::TOP * 6 + FORCES_Z] != std::numeric_limits<double>::infinity()) {
				forces_z[index] = _settings.boundaryCondition[CubeSettings::TOP * 6 + FORCES_Z];
			}
			index += nodes[0];
		}
	}

	if (_cluster[0] == _settings.clusters[0] - 1) {
		eslocal index = nodes[0] - 1;
		for (eslocal y = 0; y < nodes[1]; y++) {
			if (_settings.boundaryCondition[CubeSettings::BOTTOM * 6 + DIRICHLET_X] != std::numeric_limits<double>::infinity()) {
				dirichlet_x[index] = _settings.boundaryCondition[CubeSettings::BOTTOM * 6 + DIRICHLET_X];
			}
			if (_settings.boundaryCondition[CubeSettings::BOTTOM * 6 + FORCES_X] != std::numeric_limits<double>::infinity()) {
				forces_x[index] = _settings.boundaryCondition[CubeSettings::BOTTOM * 6 + FORCES_X];
			}
			if (_settings.boundaryCondition[CubeSettings::BOTTOM * 6 + DIRICHLET_Y] != std::numeric_limits<double>::infinity()) {
				dirichlet_y[index] = _settings.boundaryCondition[CubeSettings::BOTTOM * 6 + DIRICHLET_Y];
			}
			if (_settings.boundaryCondition[CubeSettings::BOTTOM * 6 + FORCES_Y] != std::numeric_limits<double>::infinity()) {
				forces_y[index] = _settings.boundaryCondition[CubeSettings::BOTTOM * 6 + FORCES_Y];
			}
			if (_settings.boundaryCondition[CubeSettings::BOTTOM * 6 + DIRICHLET_Z] != std::numeric_limits<double>::infinity()) {
				dirichlet_z[index] = _settings.boundaryCondition[CubeSettings::BOTTOM * 6 + DIRICHLET_Z];
			}
			if (_settings.boundaryCondition[CubeSettings::BOTTOM * 6 + FORCES_Z] != std::numeric_limits<double>::infinity()) {
				forces_z[index] = _settings.boundaryCondition[CubeSettings::BOTTOM * 6 + FORCES_Z];
			}
			index += nodes[0];
		}
	}

	if (_cluster[1] == 0) {
		eslocal index = 0;
		for (eslocal x = 0; x < nodes[0]; x++) {
			if (_settings.boundaryCondition[CubeSettings::LEFT * 6 + DIRICHLET_X] != std::numeric_limits<double>::infinity()) {
				dirichlet_x[index] = _settings.boundaryCondition[CubeSettings::LEFT * 6 + DIRICHLET_X];
			}
			if (_settings.boundaryCondition[CubeSettings::LEFT * 6 + FORCES_X] != std::numeric_limits<double>::infinity()) {
				forces_x[index] = _settings.boundaryCondition[CubeSettings::LEFT * 6 + FORCES_X];
			}
			if (_settings.boundaryCondition[CubeSettings::LEFT * 6 + DIRICHLET_Y] != std::numeric_limits<double>::infinity()) {
				dirichlet_y[index] = _settings.boundaryCondition[CubeSettings::LEFT * 6 + DIRICHLET_Y];
			}
			if (_settings.boundaryCondition[CubeSettings::LEFT * 6 + FORCES_Y] != std::numeric_limits<double>::infinity()) {
				forces_y[index] = _settings.boundaryCondition[CubeSettings::LEFT * 6 + FORCES_Y];
			}
			if (_settings.boundaryCondition[CubeSettings::LEFT * 6 + DIRICHLET_Z] != std::numeric_limits<double>::infinity()) {
				dirichlet_z[index] = _settings.boundaryCondition[CubeSettings::LEFT * 6 + DIRICHLET_Z];
			}
			if (_settings.boundaryCondition[CubeSettings::LEFT * 6 + FORCES_Z] != std::numeric_limits<double>::infinity()) {
				forces_z[index] = _settings.boundaryCondition[CubeSettings::LEFT * 6 + FORCES_Z];
			}
			index++;
		}
	}

	if (_cluster[1] == _settings.clusters[1] - 1) {
		eslocal index = nodes[1] * nodes[0] - 1;
		for (eslocal x = 0; x < nodes[0]; x++) {
			if (_settings.boundaryCondition[CubeSettings::RIGHT * 6 + DIRICHLET_X] != std::numeric_limits<double>::infinity()) {
				dirichlet_x[index] = _settings.boundaryCondition[CubeSettings::RIGHT * 6 + DIRICHLET_X];
			}
			if (_settings.boundaryCondition[CubeSettings::RIGHT * 6 + FORCES_X] != std::numeric_limits<double>::infinity()) {
				forces_x[index] = _settings.boundaryCondition[CubeSettings::RIGHT * 6 + FORCES_X];
			}
			if (_settings.boundaryCondition[CubeSettings::RIGHT * 6 + DIRICHLET_Y] != std::numeric_limits<double>::infinity()) {
				dirichlet_y[index] = _settings.boundaryCondition[CubeSettings::RIGHT * 6 + DIRICHLET_Y];
			}
			if (_settings.boundaryCondition[CubeSettings::RIGHT * 6 + FORCES_Y] != std::numeric_limits<double>::infinity()) {
				forces_y[index] = _settings.boundaryCondition[CubeSettings::RIGHT * 6 + FORCES_Y];
			}
			if (_settings.boundaryCondition[CubeSettings::RIGHT * 6 + DIRICHLET_Z] != std::numeric_limits<double>::infinity()) {
				dirichlet_z[index] = _settings.boundaryCondition[CubeSettings::RIGHT * 6 + DIRICHLET_Z];
			}
			if (_settings.boundaryCondition[CubeSettings::RIGHT * 6 + FORCES_Z] != std::numeric_limits<double>::infinity()) {
				forces_z[index] = _settings.boundaryCondition[CubeSettings::RIGHT * 6 + FORCES_Z];
			}
			index++;
		}
	}
}

template<class TElement>
void PlaneGenerator<TElement>::fixPoints(std::vector<std::vector<eslocal> > &fixPoints)
{
	if (_settings.useMetis) {
		Loader::fixPoints(fixPoints);
		return;
	}

	fixPoints.reserve(_settings.subdomainsInCluster[0] * _settings.subdomainsInCluster[1]);
	eslocal SHIFT = 1;
	eslocal shift_offset[2] = {SHIFT, SHIFT};

	eslocal nodes[2];
	eslocal cNodes[2];
	UniformUtils<TElement>::clusterNodesCount(_settings, cNodes);
	for (int i = 0; i < 2; i++) {
		nodes[i] = (TElement::subnodes[i] + 1) * _settings.elementsInSubdomain[i];
		if (2 * (shift_offset[i] + 1) > nodes[i] + 1) { // not enough nodes
			shift_offset[i] = (nodes[i] + 1) / 2 - 1;
		}
		if (2 * shift_offset[i] == nodes[i]) { // offset to the same node
			shift_offset[i]--;
		}
	}

	eslocal offset[2];
	eslocal shift[2];
	for (eslocal sy = 0; sy < _settings.subdomainsInCluster[1]; sy++) {
		for (eslocal sx = 0; sx < _settings.subdomainsInCluster[0]; sx++) {
			fixPoints.push_back(std::vector<eslocal>());
			fixPoints.back().reserve(8);
			for (int i = 0; i < 8; i++) {
				offset[0] = (i & 1) ? 1 : 0;
				offset[1] = (i & 2) ? 1 : 0;
				shift[0] = (i & 1) ? -shift_offset[0] : shift_offset[0];
				shift[1] = (i & 2) ? -shift_offset[1] : shift_offset[1];
				fixPoints.back().push_back(
						((sy + offset[1]) * nodes[1] + shift[1]) * cNodes[0] +
						((sx + offset[0]) * nodes[0] + shift[0]));
			}
		}
	}

	for (size_t p = 0; p < fixPoints.size(); p++) {
		for (size_t i = 0; i < fixPoints[p].size(); i++) {
			fixPoints[p][i] = this->mesh.coordinates().localIndex(fixPoints[p][i], p);
		}
		std::sort(fixPoints[p].begin(), fixPoints[p].end());

		// Remove the same points
		auto it = std::unique(fixPoints[p].begin(), fixPoints[p].end());
		fixPoints[p].resize(it - fixPoints[p].begin());
	}
}

template <class TElement>
void PlaneGenerator<TElement>::clusterBoundaries(Boundaries &boundaries, std::vector<int> &neighbours)
{
	esglobal gNodes[3];
	CubeUtils<TElement>::globalNodesCount(_settings, gNodes);
	eslocal cNodes[3];
	UniformUtils<TElement>::clusterNodesCount(_settings, cNodes);
	boundaries.resize(cNodes[0] * cNodes[1]);

	bool border[2];
	eslocal cIndex = _cluster[0] + _cluster[1] * _settings.clusters[0];
	esglobal index = 0;

	esglobal cs[2], ce[2];
	for (eslocal i = 0; i < 2; i++) {
		cs[i] = (cNodes[i] - 1) * _cluster[i];
		ce[i] = (cNodes[i] - 1) * (_cluster[i] + 1);
	}

	// TODO: optimize this
	std::set<int> neighs;

	for (esglobal y = cs[1]; y <= ce[1]; y++) {
		border[1] = (y == 0 || y == gNodes[1] - 1) ? false : y % ( cNodes[1] - 1) == 0;
		for (esglobal x = cs[0]; x <= ce[0]; x++) {
			border[0] = (x == 0 || x == gNodes[0] - 1) ? false : x % ( cNodes[0] - 1) == 0;
			for (int i = 0; i < 4; i++) {
				eslocal tmp = cIndex;
				if (border[0] && (i & 1)) {
					tmp += (x == cs[0]) ? -1 : 1;
				}
				if (border[1] && (i & 2)) {
					tmp += ((y == cs[1]) ? -1 : 1) * _settings.clusters[0];
				}
				boundaries[index].push_back(tmp);
				neighs.insert(tmp);
			}
			std::sort(boundaries[index].begin(), boundaries[index].end());
			auto end = std::unique(boundaries[index].begin(), boundaries[index].end());
			boundaries[index].resize(end - boundaries[index].begin());
			index++;
		}
	}

	neighs.erase(config::env::MPIrank);
	neighbours.insert(neighbours.end(), neighs.begin(), neighs.end());
}

template <class TElement>
void PlaneGenerator<TElement>::corners(Boundaries &boundaries)
{
	if (config::solver::FETI_METHOD == config::solver::FETI_METHODalternative::TOTAL_FETI) {
		// corners are not used in the case of TOTAL FETI
		return;
	}

	if (_settings.useMetis) {
		Loader::corners(boundaries);
		return;
	}

	if (_settings.corners) {
		ESINFO(DETAILS) << "Set corners to vertices";
	}
	ESINFO(DETAILS) << "Number of corners on each edge is " << (_settings.edges ? _settings.cornerCount : 0) << ".";

	eslocal nodes[2];
	eslocal cNodes[2];
	for (int i = 0; i < 2; i++) {
		nodes[i] = (TElement::subnodes[i] + 1) * _settings.elementsInSubdomain[i];
	}
	UniformUtils<TElement>::clusterNodesCount(_settings, cNodes);

	eslocal step[2];
	for (int i = 0; i < 2; i++) {
		step[i] = _settings.elementsInSubdomain[i] / (_settings.cornerCount + 1);
		step[i] *= TElement::subnodes[i] + 1;
	}
	std::vector<std::vector<size_t> > offsets(2);
	std::vector<size_t> mul(2);

	for (int i = 0; i < 2; i++) {
		for (size_t j = 0; j < _settings.subdomainsInCluster[i]; j++) {
			for (size_t k = 0; k <= _settings.cornerCount / 2; k++) {
				offsets[i].push_back(j * nodes[i] + k * step[i]);
				offsets[i].push_back(j * nodes[i] + nodes[i] - k * step[i]);
			}
			if (_settings.cornerCount % 2 == 1) {
				eslocal mid = (_settings.elementsInSubdomain[i] / 2) * (TElement::subnodes[i] + 1);
				offsets[i].push_back(j * nodes[i] + mid);
			}
		}
	}
	mul[0] = 1;
	mul[1] = cNodes[0];

	eslocal index;
	for (size_t d = 0; d < 2; d++) {
		for (eslocal i = 1; i < _settings.subdomainsInCluster[d]; i++) {
			for (size_t j = 0; j < offsets[(d + 1) % 2].size(); j++) {
				if (!_settings.corners && offsets[(d + 1) % 2][j] % nodes[(d + 1) % 2] == 0)
				{
					continue;
				}
				if (!_settings.edges && offsets[(d + 1) % 2][j] % nodes[(d + 1) % 2] != 0)
				{
					continue;
				}

				index = i * nodes[d] * mul[d];
				index += offsets[(d + 1) % 2][j] * mul[(d + 1) % 2];
				boundaries.setCorner(index);
			}
		}
	}

	if (config::mesh::AVERAGE_EDGES || config::mesh::AVERAGE_FACES) {
		// TODO: check correctness
		this->mesh.computeCorners(0, true, false, false, config::mesh::AVERAGE_EDGES, config::mesh::AVERAGE_FACES);
	}

}

}
}

