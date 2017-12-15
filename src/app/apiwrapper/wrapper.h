
#ifndef ESPRESO_WRAPPER_H_
#define ESPRESO_WRAPPER_H_

#include <iostream>
#include <list>
#include <vector>

#include "../../basis/logging/timeeval.h"
#include "../../config/ecf/ecf.h"

namespace espreso {
struct Environment;
class Instance;
class Physics;
class TimeStepSolver;
class LoadStepSolver;
class Assembler;
struct Step;
class Mesh;
class ECFConfiguration;
class FETISolver;
class ResultStore;
}

struct FETI4IStructMatrix {
	FETI4IStructMatrix(eslocal type, eslocal offset): type(type), offset(offset) {};

	std::vector<eslocal> eType;
	std::vector<std::vector<eslocal> > eNodes;
	std::vector<std::vector<eslocal> > eDOFs;
	std::vector<std::vector<double> > eMatrices;

	eslocal type;
	eslocal offset;
};

struct FETI4IStructInstance {
	FETI4IStructInstance(FETI4IStructMatrix &matrix, eslocal *l2g, size_t size);
	~FETI4IStructInstance();

	espreso::Instance *instance;
	espreso::Physics * physics;
	espreso::FETISolver *linearSolver;
	espreso::ResultStore *store;
	espreso::Step *step;
	espreso::Assembler *assembler;
	espreso::TimeStepSolver *timeStepSolver;
	espreso::LoadStepSolver *loadStepSolver;

	espreso::Mesh *mesh;
	espreso::ECFConfiguration configuration;
};

namespace espreso {

struct DataHolder {
	static ECFConfiguration *configuration;
	static std::list<FETI4IStructMatrix*> matrices;
	static std::list<FETI4IStructInstance*> instances;
	static TimeEval timeStatistics;
};

}


#endif /* ESPRESO_WRAPPER_H_ */
