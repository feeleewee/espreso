
#ifndef SRC_ASSEMBLER_PHYSICS_STRUCTURALMECHANICS_H_
#define SRC_ASSEMBLER_PHYSICS_STRUCTURALMECHANICS_H_

#include "../../basis/containers/point.h"
#include "physics.h"

namespace espreso {

struct StructuralMechanicsConfiguration;
struct ResultsSelectionConfiguration;

struct StructuralMechanics: public virtual Physics
{
	StructuralMechanics(const StructuralMechanicsConfiguration &configuration, const ResultsSelectionConfiguration &propertiesConfiguration, int DOFs);

	virtual MatrixType getMatrixType(size_t domain) const;
	virtual void prepare();
	virtual void preprocessData();
	virtual void setDirichlet();

	virtual void assembleB1(bool withRedundantMultipliers, bool withGluing, bool withScaling);

protected:
	const StructuralMechanicsConfiguration &_configuration;
	const ResultsSelectionConfiguration &_propertiesConfiguration;

	NodeData *_displacement;

	// to handle with non-continuous partition
	std::vector<Point> _cCenter, _cNorm;
	std::vector<double> _cr44, _cr45, _cr46, _cr55, _cr56;
	std::vector<size_t> _cNp;

	std::vector<Point> _dCenter, _dNorm;
	std::vector<double> _dr44, _dr45, _dr46, _dr55, _dr56;
	std::vector<size_t> _dNp;
};

}


#endif /* SRC_ASSEMBLER_PHYSICS_STRUCTURALMECHANICS_H_ */
