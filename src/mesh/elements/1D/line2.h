#ifndef LINE2_H_
#define LINE2_H_


#include "../element.h"

#define Line2NodesCount 2
#define Line2FacesCount 0
#define Line2GPCount 2
#define Line2VTKCode 3

namespace espreso {

class Line2: public Element
{

public:
	static bool match(const eslocal *indices, eslocal n);

	Line2(const eslocal *indices, const eslocal *params);

	Element* copy() const
	{
		return new Line2(*this);
	}

	eslocal vtkCode() const
	{
		return Line2VTKCode;
	}

	const eslocal* indices() const
	{
		return _indices;
	}

	size_t size() const
	{
		return Line2NodesCount;
	}

	size_t coarseSize() const
	{
		return Line2NodesCount;
	}

	size_t gpSize() const
	{
		return Line2GPCount;
	}

	size_t faces() const
	{
		return Line2FacesCount;
	}

	const std::vector<DenseMatrix>& dN() const
	{
		return Line2::_dN;
	}

	const std::vector<DenseMatrix>&  N() const
	{
		return Line2::_N;
	}

	const std::vector<double>& weighFactor() const
	{
		return Line2::_weighFactor;
	}

	eslocal nCommon() const
	{
		return 1;
	}

	std::vector<eslocal> getNeighbours(size_t nodeIndex) const;
	std::vector<eslocal> getFace(size_t face) const;
	Element* getFullFace(size_t face) const;
	Element* getCoarseFace(size_t face) const;

protected:

	eslocal* indices()
	{
		return _indices;
	}

private:
	eslocal _indices[Line2NodesCount];

	static std::vector<DenseMatrix> _dN;
	static std::vector<DenseMatrix> _N;
	static std::vector<double> _weighFactor;
};

}


#endif /* LINE2_H_ */