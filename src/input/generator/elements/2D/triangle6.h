
#ifndef INPUT_MESHGENERATOR_ELEMENTS_2D_TRIANGLE6_H_
#define INPUT_MESHGENERATOR_ELEMENTS_2D_TRIANGLE6_H_

#include "esmesh.h"
#include "../element.h"

namespace espreso {
namespace input {

class Triangle6 {

public:
	static size_t subelements;
	static size_t subnodes[3];

	static void addElements(std::vector<Element*> &elements, const eslocal indices[], const eslocal params[]);
	static void addFaces(std::vector<Element*> &faces, const eslocal indices[], CubeFace face) { ESINFO(GLOBAL_ERROR) << "Generator: plane element has no faces."; }
	static void addEdges(std::vector<Element*> &edges, const eslocal indices[], CubeEdge edge);
	static void pickNodes(const std::vector<Element*> &nodes, std::vector<Element*> &selection, const eslocal indices[], CubeEdge edge);
	static void pickNodes(const std::vector<Element*> &nodes, std::vector<Element*> &selection, const eslocal indices[], CubeFace face) { ESINFO(GLOBAL_ERROR) << "Implement pickNodes for a face for TRIANGLE6"; }

};

}
}


#endif /* INPUT_MESHGENERATOR_ELEMENTS_2D_TRIANGLE6_H_ */