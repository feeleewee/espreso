
#ifndef ASSEMBLER_ASSEMBLER_H_
#define ASSEMBLER_ASSEMBLER_H_

#include "esoutput.h"
#include "essolver.h"
#include "esmesh.h"
#include "esbem.h"
#include "../libespreso/espreso.h"

namespace assembler {

struct FEM {
	FEM(mesh::Mesh &mesh): mesh(mesh) { };

	mesh::Mesh &mesh;
};

struct BEM {
	BEM(mesh::Mesh &mesh, mesh::SurfaceMesh &surface): mesh(mesh), surface(surface) { };

	mesh::Mesh &mesh;
	mesh::SurfaceMesh &surface;
};

struct API {

	API(SparseCSRMatrix<eslocal> &K,
		ESPRESOStructDoubleVector &rhs,
		ESPRESOStructMap &dirichlet,
		ESPRESOStructIntVector &l2g,
		ESPRESOStructIntVector &neighbourRanks)
	:K(K), rhs(rhs), dirichlet(dirichlet), l2g(l2g), neighbourRanks(neighbourRanks), indexing(0) { };

	SparseCSRMatrix<eslocal> &K;
	ESPRESOStructDoubleVector &rhs;
	ESPRESOStructMap &dirichlet;
	ESPRESOStructIntVector &l2g;
	ESPRESOStructIntVector &neighbourRanks;
	eslocal indexing;
};


class AssemblerBase {

public:
	virtual void init() = 0;
	virtual void pre_solve_update() = 0;
	virtual void post_solve_update() = 0;
	virtual void solve() = 0;
	virtual void finalize() = 0;

	virtual ~AssemblerBase() {};
};

template <class TInput>
class Assembler: public AssemblerBase {

protected:
	Assembler(TInput &input): _input(input), _verbose(true) {};

	virtual size_t subdomains();
	virtual size_t rank();
	virtual size_t size();

	TInput _input;

	bool _verbose;
	TimeEval _timeStatistics;

};

}

#include "assembler.hpp"

#endif /* ASSEMBLER_ASSEMBLER_H_ */