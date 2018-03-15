
#ifndef SRC_MESH_STORE_CONTACTSTORE_H_
#define SRC_MESH_STORE_CONTACTSTORE_H_

#include <cstddef>
#include <vector>

#include "../../basis/containers/point.h"

namespace espreso {

template <typename TEBoundaries, typename TEData> class serializededata;
struct SurfaceStore;

struct ContactStore {

	double eps;
	size_t groupsize;

	SurfaceStore *surface;
	serializededata<eslocal, Point>* elements;

	Point boundingBox[2], globalBox[2];
	size_t xsize, ysize, zsize;
	size_t xbegin, xend, ybegin, yend, zbegin, zend;

	std::vector<eslocal> filledCells;
	serializededata<eslocal, eslocal>* grid;

	std::vector<int> neighbors;

	ContactStore(SurfaceStore *surface);
	~ContactStore();
};

}


#endif /* SRC_MESH_STORE_CONTACTSTORE_H_ */
