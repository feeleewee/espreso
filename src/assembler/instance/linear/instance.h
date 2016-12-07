
#ifndef SRC_ASSEMBLER_INSTANCE_LINEAR_INSTANCE_H_
#define SRC_ASSEMBLER_INSTANCE_LINEAR_INSTANCE_H_

#include "../instance.h"
#include "esoutput.h"

namespace espreso {

template <class TPhysics>
struct LinearInstance: public Instance
{
public:
	LinearInstance(const ESPRESOSolver &configuration, Mesh &mesh): Instance(mesh),
	_configuration(configuration),
	_constrains(configuration, mesh),
	_physics(mesh, _constrains, configuration),
	_linearSolver(configuration, _physics, _constrains),
	_store(mesh, "results", output->domain_shrink_ratio, output->cluster_shrink_ratio)
	{
		_timeStatistics.totalTime.startWithBarrier();
	};

	virtual void init();
	virtual void solve(std::vector<std::vector<double> > &solution);
	virtual void finalize();

	virtual ~LinearInstance() {};

	virtual const Physics& physics() const { return _physics; }
	virtual const Constraints& constraints() const { return _constrains; }

protected:
	const ESPRESOSolver &_configuration;
	Constraints _constrains;
	TPhysics _physics;
	LinearSolver _linearSolver;
	store::VTK _store;
};

}

#include "instance.hpp"

#endif /* SRC_ASSEMBLER_INSTANCE_LINEAR_INSTANCE_H_ */
