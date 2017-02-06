
#ifndef SRC_ASSEMBLER_OLD_PHYSICS_ASSEMBLER_H_
#define SRC_ASSEMBLER_OLD_PHYSICS_ASSEMBLER_H_

#include <vector>
#include <cstddef>

#include "../../basis/matrices/matrixtype.h"

namespace espreso {

class DenseMatrix;
class Element;
class Coordinates;
class Mesh;
class Constraints;
class ESPRESOSolver;
enum class Property;
class SparseMatrix;

namespace store {
class ResultStore;
}

struct OldPhysics {
	virtual bool singular() const =0;

	virtual void prepareMeshStructures() =0;
	virtual void assembleStiffnessMatrix(const Element* e, DenseMatrix &Ke, std::vector<double> &fe, std::vector<eslocal> &dofs) const =0;
	virtual void assembleStiffnessMatrices() =0;
	virtual void makeStiffnessMatricesRegular() =0;
	virtual void assembleB1() =0;
	virtual void assembleB0() =0;

	virtual void postProcess(store::ResultStore &store, const std::vector<std::vector<double> > &solution) {};

	virtual void saveMeshProperties(store::ResultStore &store) =0;
	virtual void saveMeshResults(store::ResultStore &store, const std::vector<std::vector<double> > &results) =0;
	virtual void saveStiffnessMatrices();
	virtual void saveKernelMatrices();

	std::vector<Property> elementDOFs;
	std::vector<Property> faceDOFs;
	std::vector<Property> edgeDOFs;
	std::vector<Property> pointDOFs;
	std::vector<Property> midPointDOFs;

	std::vector<size_t> matrixSize;

	MatrixType mtype;
	std::vector<SparseMatrix> K, R1, R2, RegMat;
	std::vector<std::vector<double> > f, D;
	std::vector<bool> singularK;

	OldPhysics(Mesh &mesh,
			Constraints &constraints,
			const ESPRESOSolver &configuration,
			MatrixType mtype,
			const std::vector<Property> elementDOFs,
			const std::vector<Property> faceDOFs,
			const std::vector<Property> edgeDOFs,
			const std::vector<Property> pointDOFs,
			const std::vector<Property> midPointDOFs);

	virtual ~OldPhysics();

protected:
	Mesh& _mesh;
	Constraints &_constraints;
	const ESPRESOSolver &_solverConfiguration;
};

}

#endif /* SRC_ASSEMBLER_OLD_PHYSICS_ASSEMBLER_H_ */