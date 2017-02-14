
#include "newtonrhapson.h"
#include "../step.h"
#include "../instance.h"
#include "../solution.h"
#include "../physics/physics.h"

#include "../../basis/utilities/utils.h"
#include "../../configuration/physics/nonlinearsolver.h"

#include "../../solver/generic/LinearSolver.h"

using namespace espreso;

NewtonRhapson::NewtonRhapson(
		Mesh *mesh,
		std::vector<Physics*> &physics,
		std::vector<Instance*> &instances,
		std::vector<LinearSolver*> &linearSolvers,
		store::ResultStore* store,
		const NonLinearSolver &configuration)
: Solver(mesh, physics, instances, linearSolvers, store), _configuration(configuration)
{

}

void NewtonRhapson::run(Step &step)
{
	if (!_configuration.convergence_parameters.temperature && _configuration.convergence_parameters.heat) {
		ESINFO(GLOBAL_ERROR) << "It is not possible to turn off the both 'temperature' and 'heat' convergence.";
	}
	step.solver = 0;

	assembleStiffnessMatrices(step);
	assembleB1(step);
	makeStiffnessMatricesRegular(step);
	assembleB0(step);

	initLinearSolver();
	startLinearSolver();
	storeSolution(step);

	double temperatureResidual = _configuration.convergence_parameters.temperature_residual;
	double heatResidual = _configuration.convergence_parameters.heat_residual;
	double heatResidualNorm = 0;
	if (_configuration.convergence_parameters.temperature) {
		temperatureResidual *= 10;
	}
	if (_configuration.convergence_parameters.heat) {
		heatResidual *= 10;
	}

	while (
		step.solver++ < _configuration.max_iterations &&
		(temperatureResidual > _configuration.convergence_parameters.temperature_residual ||
		heatResidual > _configuration.convergence_parameters.heat_residual)) {

		std::vector<std::vector<double> > T = instances.front()->primalSolution;

		instances.front() = new Instance(instances.front()->domains);
		instances.front()->DOFs = physics.front()->instance()->DOFs;
		instances.front()->primalSolution = physics.front()->instance()->primalSolution;
		instances.front()->solutions = physics.front()->instance()->solutions;
		physics.front()->instance()->solutions.resize(0);
		linearSolvers.front() = new LinearSolver(linearSolvers.front()->configuration, linearSolvers.front()->physics, linearSolvers.front()->constraints);
		physics.front()->_instance = instances.front();

		assembleStiffnessMatrices(step);
		if (_configuration.convergence_parameters.heat) {
			heatResidual = physics.front()->sumSquares(physics.front()->instance()->f, Physics::SumOperation::SUM);
		}
		assembleResidualForces(step);
		sumVectors(physics.front()->instance()->f, physics.front()->instance()->f, physics.front()->instance()->R, 1, -1);
		if (_configuration.convergence_parameters.heat) {
			heatResidual += physics.front()->sumSquares(physics.front()->instance()->f, Physics::SumOperation::SUM, Physics::SumRestriction::DIRICHLET, step.load);
		}
		assembleB1(step);

		subtractSolutionFromB1c(step);

		makeStiffnessMatricesRegular(step);

		initLinearSolver();
		startLinearSolver();

		if (_configuration.line_search) {
			lineSearch(T, physics.front()->instance()->primalSolution, physics.front(), step);
		}
		if (_configuration.convergence_parameters.temperature) {
			temperatureResidual = sqrt(physics.front()->sumSquares(physics.front()->instance()->primalSolution, Physics::SumOperation::AVERAGE));
		}
		sumVectors(physics.front()->instance()->primalSolution, T, physics.front()->instance()->primalSolution);
		if (_configuration.convergence_parameters.temperature) {
			temperatureResidual /= sqrt(physics.front()->sumSquares(physics.front()->instance()->primalSolution, Physics::SumOperation::AVERAGE));;
		}

		storeSolution(step);

		if (_configuration.convergence_parameters.heat) {
			assembleStiffnessMatrices(step);
			assembleResidualForces(step);
			double R;
			// |R| = f - R
			R = physics.front()->sumSquares(physics.front()->instance()->f, Physics::SumOperation::SUM);
			// |R| -= BtLambda
			// TODO
		}
	}
	finalizeLinearSolver();
}



