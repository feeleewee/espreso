
#ifndef INPUT_MESHGENERATOR_UNIFORMMESH_CUBE_GENERATOR_H_
#define INPUT_MESHGENERATOR_UNIFORMMESH_CUBE_GENERATOR_H_

#include "../generator.h"
#include "settings.h"
#include "utils.h"

namespace esinput {

template<class TElement>
class CubeGenerator: public UniformGenerator<TElement> {

public:
	CubeGenerator(int argc, char** argv, int rank, int size);

	void points(mesh::Coordinates &coordinates);
	void boundaryConditions(mesh::Coordinates &coordinates);
	void clusterBoundaries(mesh::Boundaries &boundaries);

protected:
	CubeSettings _settings;
	size_t _cluster[3];
};

}

#include "generator.hpp"



#endif /* INPUT_MESHGENERATOR_UNIFORMMESH_CUBE_GENERATOR_H_ */