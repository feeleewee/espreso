
#include "generator.h"

namespace esinput {


template<class TElement>
UniformGenerator<TElement>::UniformGenerator(int argc, char** argv, int rank, int size)
	: Generator(rank, size), _settings(argc, argv) { };

template<class TElement>
void UniformGenerator<TElement>::elements(std::vector<mesh::Element*> &elements, std::vector<eslocal> &parts)
{
	eslocal cNodes[3];

	UniformUtils<TElement>::clusterNodesCount(_settings, cNodes);

	std::vector<eslocal> indices((2 + TElement::subnodes[0]) * (2 + TElement::subnodes[1]) * (2 + TElement::subnodes[2]));

	elements.clear();
	elements.reserve(UniformUtils<TElement>::clusterElementsCount(_settings));
	parts.clear();
	parts.reserve(_settings.subdomainsInCluster[0] * _settings.subdomainsInCluster[1] * _settings.subdomainsInCluster[2] + 1);

	eslocal subdomain[3];
	eslocal element[3];

	eslocal subdomainOffset[3];
	eslocal elementOffset[3];

	parts.push_back(elements.size());
	for (subdomain[2] = 0; subdomain[2] < _settings.subdomainsInCluster[2]; subdomain[2]++) {
		for (subdomain[1] = 0; subdomain[1] < _settings.subdomainsInCluster[1]; subdomain[1]++) {
			for (subdomain[0] = 0; subdomain[0] < _settings.subdomainsInCluster[0]; subdomain[0]++) {
				// for each sub-domain

				for (eslocal i = 0; i < 3; i++) {
					subdomainOffset[i] = subdomain[i] * (_settings.elementsInSubdomain[i] * (1 + TElement::subnodes[i]));
				}
				for (element[2] = 0; element[2] < _settings.elementsInSubdomain[2]; element[2]++) {
					for (element[1] = 0; element[1] < _settings.elementsInSubdomain[1]; element[1]++) {
						for (element[0] = 0; element[0] < _settings.elementsInSubdomain[0]; element[0]++) {
							// for each element

							for (eslocal i = 0; i < 3; i++) {
								elementOffset[i] = subdomainOffset[i] + element[i] * (1 + TElement::subnodes[i]);
							}
							eslocal i = 0;
							for (eslocal z = 0; z < 2 + TElement::subnodes[2]; z++) {
								for (eslocal y = 0; y < 2 + TElement::subnodes[1]; y++) {
									for (eslocal x = 0; x < 2 + TElement::subnodes[0]; x++) {
										// fill node indices

										indices[i++] =
												(elementOffset[2] + z) * cNodes[0] * cNodes[1] +
												(elementOffset[1] + y) * cNodes[0] +
												(elementOffset[0] + x);
									}
								}
							}
							_e.addElements(elements, &indices[0]);
						}
					}
				}
				parts.push_back(elements.size());
			}
		}
	}
}

template<class TElement>
void UniformGenerator<TElement>::fixPoints(std::vector<eslocal> &fixPoints)
{
	fixPoints.reserve(8 * _settings.subdomainsInCluster[0] * _settings.subdomainsInCluster[1] * _settings.subdomainsInCluster[2]);

	eslocal nodes[3];
	eslocal cNodes[3];
	for (int i = 0; i < 3; i++) {
		nodes[i] = (TElement::subnodes[i] + 1) * _settings.elementsInSubdomain[i];
	}
	UniformUtils<TElement>::clusterNodesCount(_settings, cNodes);

	eslocal offset[3];
	for (eslocal sz = 0; sz < _settings.subdomainsInCluster[2]; sz++) {
		for (eslocal sy = 0; sy < _settings.subdomainsInCluster[1]; sy++) {
			for (eslocal sx = 0; sx < _settings.subdomainsInCluster[0]; sx++) {
				for (int i = 0; i < 8; i++) {
					offset[0] = (i & 1) ? 1 : 0;
					offset[1] = (i & 2) ? 1 : 0;
					offset[2] = (i & 4) ? 1 : 0;
					fixPoints.push_back(
							(sz + offset[2]) * nodes[2] * cNodes[0] * cNodes[1] +
							(sy + offset[1]) * nodes[1] * cNodes[0] +
							(sx + offset[0]) * nodes[0]);
				}
			}
		}
	}
}

template <class TElement>
void UniformGenerator<TElement>::corners(mesh::Boundaries &boundaries)
{
	eslocal nodes[3];
	eslocal cNodes[3];
	for (int i = 0; i < 3; i++) {
		nodes[i] = (TElement::subnodes[i] + 1) * _settings.elementsInSubdomain[i];
	}
	UniformUtils<TElement>::clusterNodesCount(_settings, cNodes);

	eslocal step[3];
	for (int i = 0; i < 3; i++) {
		step[i] = _settings.elementsInSubdomain[i] / (_settings.cornerCount + 1);
		step[i] *= TElement::subnodes[i] + 1;
	}
	std::vector<std::vector<size_t> > offsets(3);
	std::vector<size_t> mul(3);

	for (int i = 0; i < 3; i++) {
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
	mul[2] = cNodes[0] * cNodes[1];

	eslocal index;
	for (size_t d = 0; d < 3; d++) {
		for (eslocal i = 1; i < _settings.subdomainsInCluster[d]; i++) {
			for (size_t j = 0; j < offsets[(d + 1) % 3].size(); j++) {
				for (size_t k = 0; k < offsets[(d + 2) % 3].size(); k++) {
					if (!_settings.corners
						&& offsets[(d + 1) % 3][j] % nodes[(d + 1) % 3] == 0
						&& offsets[(d + 2) % 3][k] % nodes[(d + 2) % 3] == 0)
					{
						continue;
					}
					if (!_settings.edges
						&& offsets[(d + 1) % 3][j] % nodes[(d + 1) % 3] == 0
						&& offsets[(d + 2) % 3][k] % nodes[(d + 2) % 3] != 0)
					{
						continue;
					}
					if (!_settings.edges
						&& offsets[(d + 1) % 3][j] % nodes[(d + 1) % 3] != 0
						&& offsets[(d + 2) % 3][k] % nodes[(d + 2) % 3] == 0)
					{
						continue;
					}
					if (!_settings.faces
						&& offsets[(d + 1) % 3][j] % nodes[(d + 1) % 3] != 0
						&& offsets[(d + 2) % 3][k] % nodes[(d + 2) % 3] != 0)
					{
						continue;
					}
					index = i * nodes[d] * mul[d];
					index += offsets[(d + 1) % 3][j] * mul[(d + 1) % 3];
					index += offsets[(d + 2) % 3][k] * mul[(d + 2) % 3];
					boundaries.setCorner(index);
				}
			}
		}
	}
}

}
