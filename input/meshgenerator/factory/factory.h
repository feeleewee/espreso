
#ifndef INPUT_MESHGENERATOR_FACTORY_H_
#define INPUT_MESHGENERATOR_FACTORY_H_

#include "settings.h"
#include "../generator.h"
#include "../uniformmesh/cube/generator.h"
#include "../uniformmesh/sphere/generator.h"

namespace esinput {

class MeshFactory {

public:
	static Generator* create(int argc, char** argv, int rank, int size);
};

}


#endif /* INPUT_MESHGENERATOR_FACTORY_H_ */