#include "square.h"

using namespace mesh;

// TODO: Implement base functions
std::vector<DenseMatrix> Square::_dN;
std::vector<DenseMatrix> Square::_N;
std::vector<double> Square::_weighFactor;

bool Square::match(eslocal *indices, eslocal n)
{
	if (n != 4) {
		return false;
	}

	for (eslocal i = 0; i < SquareNodesCount - 1; i++) {
		for (eslocal j = i + 1; j < SquareNodesCount; j++) {
			if (Element::match(indices, i, j)) {
				return false;
			}
		}
	}

	return true;
}

std::vector<eslocal> Square::getNeighbours(size_t nodeIndex) const
{
	std::vector<eslocal> result(2);

	result[0] = _indices[(nodeIndex + 1) % 4];
	result[1] = _indices[(nodeIndex + 3) % 4];

	return result;
}

std::vector<eslocal> Square::getFace(size_t face) const
{
	return std::vector<eslocal> (_indices, _indices + 4);
}

Square::Square(eslocal *indices)
{
	memcpy(_indices, indices, SquareNodesCount * sizeof(eslocal));
}


