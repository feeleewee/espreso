#include "pyramid5.h"

using namespace espreso;

size_t Pyramid5::_counter = 0;

std::vector<Property> Pyramid5::_DOFElement;
std::vector<Property> Pyramid5::_DOFFace;
std::vector<Property> Pyramid5::_DOFEdge;
std::vector<Property> Pyramid5::_DOFPoint;
std::vector<Property> Pyramid5::_DOFMidPoint;

static std::vector< std::vector< double> > Pyramid5_rst()
{
	std::vector< std::vector<double> > rst(3, std::vector<double>(Pyramid5GPCount));

	switch (Pyramid5GPCount) {
	case 8:
		double v = 0.577350269189625953;
		rst[0] = {  v,  v,  v,  v, -v, -v, -v, -v };
		rst[1] = { -v, -v,  v,  v, -v, -v,  v,  v };
		rst[2] = { -v,  v, -v,  v, -v,  v, -v,  v };
		return rst;
	default:
		ESINFO(ERROR) << "Unknown number of Pyramid5 GP count.";
		exit(EXIT_FAILURE);
	}
}

static std::vector<DenseMatrix> Pyramid5_dN() {
	std::vector<DenseMatrix> dN(
		Pyramid5GPCount,
		DenseMatrix(Point::size(), Pyramid5NodesCount)
	);

	std::vector< std::vector< double> > _pyramid5_rst = Pyramid5_rst();

	for (unsigned int i = 0; i < Pyramid5GPCount; i++) {
		///dN contains [dNr, dNs, dNt]
		DenseMatrix &m = dN[i];

		double r = _pyramid5_rst[0][i];
		double s = _pyramid5_rst[1][i];
		double t = _pyramid5_rst[2][i];

		// dNr - derivation of basis function
		m(0, 0) = 0.125 * (-(1. - s) * (1. - t));
		m(0, 1) = 0.125 * ( (1. - s) * (1. - t));
		m(0, 2) = 0.125 * ( (1. + s) * (1. - t));
		m(0, 3) = 0.125 * (-(1. + s) * (1. - t));
		m(0, 4) = 0;

		// dNs - derivation of basis function
		m(1, 0) = 0.125 * (-(1. - r) * (1. - t));
		m(1, 1) = 0.125 * (-(1. + r) * (1. - t));
		m(1, 2) = 0.125 * ( (1. + r) * (1. - t));
		m(1, 3) = 0.125 * ( (1. - r) * (1. - t));
		m(1, 4) = 0;

		// dNt - derivation of basis function
		m(2, 0) = 0.125 * (-(1. - r) * (1. - s));
		m(2, 1) = 0.125 * (-(1. + r) * (1. - s));
		m(2, 2) = 0.125 * (-(1. + r) * (1. + s));
		m(2, 3) = 0.125 * (-(1. - r) * (1. + s));
		m(2, 4) = 0.125 * (4.0);
	}

	return dN;
}

static std::vector<DenseMatrix> Pyramid5_N() {
	std::vector<DenseMatrix> N(
		Pyramid5GPCount,
		DenseMatrix(1, Pyramid5NodesCount)
	);

	std::vector< std::vector< double> > _pyramid5_rst = Pyramid5_rst();

	for (unsigned int i = 0; i < Pyramid5GPCount; i++) {
		DenseMatrix &m = N[i];

		double r = _pyramid5_rst[0][i];
		double s = _pyramid5_rst[1][i];
		double t = _pyramid5_rst[2][i];

		// basis function
		m(0, 0) = 0.125 * ((1 - r) * (1 - s) * (1 - t));
		m(0, 1) = 0.125 * ((1 + r) * (1 - s) * (1 - t));
		m(0, 2) = 0.125 * ((1 + r) * (1 + s) * (1 - t));
		m(0, 3) = 0.125 * ((1 - r) * (1 + s) * (1 - t));
		m(0, 4) = 0.125 * ( 4 * (1 + t));
	}

	return N;
}

static std::vector<double> Pyramid5_weight()
{
	switch (Pyramid5GPCount) {
	case 8: {
		return std::vector<double> (8, 1.0);
	}
	default:
		ESINFO(ERROR) << "Unknown number of Tatrahedron10 GP count.";
		exit(EXIT_FAILURE);
	}
}

std::vector<DenseMatrix> Pyramid5::_dN = Pyramid5_dN();
std::vector<DenseMatrix> Pyramid5::_N = Pyramid5_N();
std::vector<double> Pyramid5::_weighFactor = Pyramid5_weight();

bool Pyramid5::match(const eslocal *indices, eslocal n) {

#if ESPRESO_POINT_DIMENSION == 2
	// Hexahedron8 is 3D element
	return false;
#endif

	switch (n) {
	case 5:
		for (eslocal i = 0; i < 4; i++) {
			for (eslocal j = i + 1; j < 5; j++) {
				if (Element::match(indices, i, j)) {
					return false;
				}
			}
		}
		return true;
	case 8:
		if (!Element::match(indices, 4, 5)) {
			return false;
		}
		if (!Element::match(indices, 5, 6)) {
			return false;
		}
		if (!Element::match(indices, 6, 7)) {
			return false;
		}

		eslocal various[5] = { 0, 1, 2, 3, 4};
		for (eslocal i = 0; i < 4; i++) {
			for (eslocal j = i + 1; j < 5; j++) {
				if (Element::match(indices, various[i], various[j])) {
					return false;
				}
			}
		}
		return true;
	default:
		return false;
	}
}

std::vector<eslocal> Pyramid5::getNeighbours(size_t nodeIndex) const
{
	if (nodeIndex < 4) {
	  std::vector<eslocal> result(3);
		result[0] = _indices[4] ;
		result[1] = _indices[(nodeIndex + 1) % 4];
		result[2] = _indices[(nodeIndex + 3) % 4];
	  return result;
	} else {
	  std::vector<eslocal> result(_indices,_indices + 4);
	  return result;
	}
}

std::vector<eslocal> Pyramid5::getFace(size_t face) const
{
	// bottom
	if (face == 0) {
		std::vector<eslocal> result(4);
		result[0] = _indices[0];
		result[1] = _indices[3];
		result[2] = _indices[2];
		result[3] = _indices[1];
		return result;
	}
	////sides
	std::vector<eslocal> result(3);
	result[0] = _indices[ face -1   ];
	result[1] = _indices[(face) % 3 ];
	result[2] = _indices[4];
	return result;
}

Element* Pyramid5::getF(const eslocal *indices, const eslocal *params, size_t face)
{
	// bottom
	if (face == 0) {
		std::vector<eslocal> result(4);
		result[0] = indices[0];
		result[1] = indices[3];
		result[2] = indices[2];
		result[3] = indices[1];
		return new Square4(result.data(), params);
	}

	////sides
	std::vector<eslocal> result(3);
	result[0] = indices[face - 1];
	result[1] = indices[face % 4];
	result[2] = indices[4];
	return new Triangle3(result.data(), params);
}

Pyramid5::Pyramid5(const eslocal *indices, eslocal n, const eslocal *params): Element(params)
{
	switch (n) {
	case 8:
		_indices[0] = indices[0];
		_indices[1] = indices[1];
		_indices[2] = indices[2];
		_indices[3] = indices[3];
		_indices[4] = indices[4];
		break;
	case 5:
		memcpy(_indices, indices, 5 * sizeof(eslocal));
		break;
	default:
		ESINFO(ERROR) << "It is not possible to create Tetrahedron5 from " << n << " elements.";
	}
}

Pyramid5::Pyramid5(std::ifstream &is): Element(is)
{
	is.read(reinterpret_cast<char *>(_indices), sizeof(eslocal) * size());
}






