
#include "assembler.h"

#include "../step.h"
#include "../instance.h"
#include "../physics/physics.h"
#include "../../linearsolver/linearsolver.h"

#include "../../config/ecf/root.h"
#include "../../solver/generic/SparseMatrix.h"
#include "../../basis/logging/timeeval.h"
#include "../../basis/logging/logging.h"
#include "../../basis/utilities/utils.h"
#include "../../mesh/mesh.h"

#include "mpi.h"

#include "../../output/result/resultstore.h"
#include "../../output/result/visualization/separated/vtklegacy.h"

using namespace espreso;

static std::string mNames(espreso::Matrices matrices, const std::string &prefix = "")
{
	return
	std::string(matrices & espreso::Matrices::K           ? prefix + "K "           : "") +
	std::string(matrices & espreso::Matrices::N           ? prefix + "N1 "          : "") +
	std::string(matrices & espreso::Matrices::N           ? prefix + "N2 "          : "") +
	std::string(matrices & espreso::Matrices::N           ? prefix + "RegMat "      : "") +
	std::string(matrices & espreso::Matrices::M           ? prefix + "M "           : "") +
	std::string(matrices & espreso::Matrices::R           ? prefix + "R "           : "") +
	std::string(matrices & espreso::Matrices::f           ? prefix + "f "           : "") +
	std::string(matrices & espreso::Matrices::B0          ? prefix + "B0 "          : "") +
	std::string(matrices & espreso::Matrices::B1          ? prefix + "B1 "          : "") +
	std::string(matrices & espreso::Matrices::B1c         ? prefix + "B1c "         : "") +
	std::string(matrices & espreso::Matrices::B1duplicity ? prefix + "B1duplicity " : "") +
	std::string(matrices & espreso::Matrices::primal      ? prefix + "Primal "      : "") +
	std::string(matrices & espreso::Matrices::dual        ? prefix + "Dual "        : "");
}

Assembler::Assembler(Instance &instance, Physics &physics, Mesh &mesh, Step &step, ResultStore &store, LinearSolver &linearSolver)
: instance(instance), physics(physics), mesh(mesh), step(step), store(store), linearSolver(linearSolver), _timeStatistics(new TimeEval("Physics solver timing"))
{
	_timeStatistics->totalTime.startWithBarrier();
}

Assembler::~Assembler()
{
	delete _timeStatistics;
	for (auto it = _timeEvents.begin(); it != _timeEvents.end(); ++it) {
		delete it->second;
	}
}

void Assembler::preprocessData()
{
	timeWrapper("pre-process data", [&] () {
		physics.preprocessData();
	});
}

void Assembler::updateStructuralMatrices(Matrices matrices)
{
	Matrices updated = matrices & (Matrices::K | Matrices::M | Matrices::f | Matrices::R);

	if (updated) {
		timeWrapper("update " + mNames(updated), [&] () {
			physics.updateMatrix(updated);
		});
	}
}

void Assembler::updateGluingMatrices(Matrices matrices)
{
	if (matrices & Matrices::B1) {
		timeWrapper("update " + mNames(Matrices::B1), [&] () {
			// TODO: create update method
			instance.B1.clear();
			instance.B1.resize(instance.domains);
			instance.inequality.clear();
			instance.inequality.resize(instance.domains);
			instance.B1clustersMap.clear();
			for (size_t d = 0; d < instance.domains; d++) {
				instance.B1[d].type = 'G';
				instance.B1[d].cols = instance.domainDOFCount[d];
				instance.B1c[d].clear();
				instance.LB[d].clear();
				instance.B1duplicity[d].clear();
				instance.inequalityC[d].clear();
				instance.B1subdomainsMap[d].clear();
			}
			instance.block.clear();
			instance.block.resize(3, 0);
			physics.assembleB1(linearSolver.applyB1LagrangeRedundancy(), linearSolver.glueDomainsByLagrangeMultipliers(), linearSolver.applyB1Scaling());
		});

		if (mesh.configuration.output.debug) {
			VTKLegacyDebugInfo::dirichlet(mesh, instance);
			VTKLegacyDebugInfo::gluing(mesh, instance);
		}
		return;
	}

	if (matrices & Matrices::B1c) {
		timeWrapper("update " + mNames(Matrices::B1c), [&] () {
			physics.updateDirichletInB1(linearSolver.applyB1LagrangeRedundancy());
		});
	}

	if (matrices & Matrices::B1duplicity) {
		timeWrapper("update " + mNames(Matrices::B1duplicity), [&] () {
			physics.updateDuplicity();
		});
	}
}


void Assembler::processSolution()
{
	timeWrapper("post-processing", [&] () {
		physics.processSolution();
	});
	storeWrapper(mNames(Matrices::primal), Matrices::primal);
}

void Assembler::solve(Matrices updatedMatrices)
{
	Matrices solverMatrices = Matrices::K | Matrices::M | Matrices::f | Matrices::B1;
	storeWrapper(mNames(solverMatrices), solverMatrices);

	timeWrapper("update linear solver: " + mNames(updatedMatrices), [&] () {
		linearSolver.update(updatedMatrices);
	});

	timeWrapper("run linear solver", [&] () {
		linearSolver.solve();
	});
}

void Assembler::storeSolution()
{
	if (store.storeStep(step)) {
		if (store.isCollected()) {
			mesh.gatherNodeData();
		}
		timeWrapper("store solution", [&] () {
			store.updateSolution(step);
		});
	}
}

void Assembler::storeSubSolution()
{
	timeWrapper("store solution", [&] () {
		// TODO: MESH
		// store.storeSubSolution(step, solutions, {});
	});
}

void Assembler::finalize()
{
	timeWrapper("finalize", [&] () {
		linearSolver.finalize();
	});
	_timeStatistics->totalTime.endWithBarrier();
	_timeStatistics->printStatsMPI();
}

void Assembler::keepK()
{
	timeWrapper("copy K to origK", [&] () {
		#pragma omp parallel for
		for (size_t d = 0; d < instance.domains; d++) {
			instance.origK[d] = instance.K[d];
		}
	});
}

void Assembler::sum(std::vector<std::vector<double> > &z, double a, const std::vector<std::vector<double> > &x, double b, const std::vector<std::vector<double> > &y, const std::string &description)
{
	std::vector<size_t> prefix(x.size());
	for (size_t i = 0; i < x.size(); i++) {
		prefix[i] = x[i].size();
	}
	sum(z, a, x, b, y, prefix, description);
}

void Assembler::sum(std::vector<std::vector<double> > &z, double a, const std::vector<std::vector<double> > &x, double b, const std::vector<std::vector<double> > &y, const std::vector<size_t> &prefix, const std::string &description)
{
	timeWrapper("compute: " + description, [&] () {
		if (z.size() == 0) {
			z.resize(x.size());
		}
		#pragma omp parallel for
		for (size_t d = 0; d < x.size(); d++) {
			if (z[d].size() == 0) {
				z[d].resize(x[d].size());
			}
			if (x[d].size() != y[d].size() || z[d].size() != x[d].size()) {
				ESINFO(ERROR) << "ESPRESO internal error while " << description << ". Vectors have different dimension.";
			}
			for (size_t i = 0; i < x[d].size() && i < prefix[d]; i++) {
				z[d][i] = a * x[d][i] + b * y[d][i];
			}
		}
	});
}

void Assembler::sum(std::vector<SparseMatrix> &A, double beta, std::vector<SparseMatrix> &B, const std::string &description)
{
	timeWrapper("compute: " + description, [&] () {
		#pragma omp parallel for
		for (size_t d = 0; d < instance.domains; d++) {
			A[d].MatAddInPlace(B[d], 'N', beta);
		}
	});
}

/// y = A * x
void Assembler::multiply(std::vector<std::vector<double> > &y, std::vector<SparseMatrix> &A, std::vector<std::vector<double> > &x, const std::string &description)
{
	timeWrapper("compute: " + description, [&] () {
		#pragma omp parallel for
		for (size_t d = 0; d < instance.domains; d++) {
			A[d].MatVec(x[d], y[d], 'N', 0, 0, 0);
		}
	});
}

// v = x * y
double Assembler::multiply(std::vector<std::vector<double> > &x, std::vector<std::vector<double> > &y, const std::string &description)
{
	double sum = 0;
	timeWrapper("compute: " + description, [&] () {
		double psum = 0;
		#pragma omp parallel for reduction(+:psum)
		for (size_t d = 0; d < x.size(); d++) {
			if (x[d].size() != y[d].size()) {
				ESINFO(ERROR) << "ESPRESO internal error while " << description << ". Vectors have different dimension.";
			}
			for (size_t i = 0; i < x[d].size(); i++) {
				psum += x[d][i] * y[d][i];
			}
		}
		sum = psum;
	});
	return sum;
}

double Assembler::sumSquares(const std::vector<std::vector<double> > &data, SumRestriction restriction, const std::string &description)
{
	double result;
	timeWrapper(description, [&] () {
		result = physics.sumSquares(data, restriction);
	});
	return result;
}

void Assembler::addToDirichletInB1(double a, const std::vector<std::vector<double> > &x)
{
	timeWrapper("subtract primal solution from dirichlet", [&] () {
		#pragma omp parallel for
		for (size_t d = 0; d < instance.domains; d++) {
			for (size_t j = 0; j < instance.B1[d].J_col_indices.size(); j++) {
				if (instance.B1[d].I_row_indices[j] > (eslocal)instance.block[Instance::CONSTRAINT::DIRICHLET]) {
					break;
				}
				instance.B1c[d][j] += a * x[d][instance.B1[d].J_col_indices[j] - 1];
			}
		}
	});
}

double Assembler::maxAbsValue(const std::vector<std::vector<double> > &v, const std::string &description)
{
	double gmax;
	timeWrapper(description, [&] () {
		double max = 0;
		for (size_t p = 0; p < v.size(); p++) {
			max = std::max(max, std::fabs(*std::max_element(v[p].begin(), v[p].end(), [] (const double v1, const double v2) { return std::fabs(v1) < std::fabs(v2); })));
		}

		MPI_Allreduce(&max, &gmax, 1, MPI_DOUBLE, MPI_MAX, environment->MPICommunicator);
	});
	return gmax;
}

double Assembler::lineSearch(const std::vector<std::vector<double> > &U, std::vector<std::vector<double> > &deltaU, std::vector<std::vector<double> > &F_ext)
{
	double alpha = 1;
	timeWrapper("line search", [&] () {
		auto multiply = [] (const std::vector<std::vector<double> > &v1, const std::vector<std::vector<double> > &v2) {
			double cmul = 0, gmul;

			#pragma omp parallel for
			for (size_t d = 0; d < v1.size(); d++) {
				double dmul = 0;
				for (size_t i = 0; i < v1[d].size(); i++) {
					dmul += v1[d][i] * v2[d][i];
				}
				#pragma omp atomic
				cmul += dmul;
			}
			MPI_Allreduce(&cmul, &gmul, 1, MPI_DOUBLE, MPI_SUM, environment->MPICommunicator);
			return gmul;
		};

		double a = 0, b = 1;
		double fa = 0, fb = 0, fx = 0, faStart = 0;

		std::vector<std::vector<double> > solution = deltaU;
		std::vector<std::vector<double> > F_ext_r = F_ext;

		for (size_t i = 0; i < 6; i++) {
			sum(solution, 1, U, alpha, deltaU, "U = U + alpha * delta U (line search)");

			solution.swap(instance.primalSolution);
			physics.updateMatrix(Matrices::R);
			solution.swap(instance.primalSolution);

			if (i == 0) {
				faStart = multiply(deltaU, instance.f);
				sum(F_ext_r, 1, F_ext, -1, instance.R, "F_ext - R");

				fb = multiply(deltaU, F_ext_r);
				if ((faStart < 0 && fb < 0) || (faStart >= 0 && fb >= 0)) {
					return;
				}
				fa = faStart;
			} else {
				sum(F_ext_r, 1, F_ext, -1, instance.R, "F_ext - R");
				fx = multiply(deltaU, F_ext_r);
				if (fa * fx < 0) {
					b = alpha;
					fb = fx;
				} else if (fb * fx < 0) {
					a = alpha;
					fa = fx;
				}

				if (fabs(fx) <= 0.5 * faStart) {
					alpha = a - fa * ((b - a ) / (fb - fa));
					break;
				}
			}

			alpha = a - fa * ((b - a ) / (fb - fa));
		}

		if (alpha < 0.1) {
			alpha = 0.1;
		}
		if (alpha > .99) {
			alpha = 1;
		}

		sum(solution, 0, U, alpha, deltaU, "delta U = alpha * delta U (line search)");
		solution.swap(deltaU);
	});
	return alpha;
}

void Assembler::setRegularizationCallback()
{
	instance.computeKernelsCallback = [&] (FETI_REGULARIZATION regularization, size_t scSize, bool ortogonalCluster) {

		timeWrapper("regularize " + mNames(Matrices::K), [&] () {
			physics.makeStiffnessMatricesRegular(regularization, scSize, ortogonalCluster);
		});

		storeWrapper(mNames(Matrices::N), Matrices::N);
	};

	instance.computeKernelCallback = [&] (FETI_REGULARIZATION regularization, size_t scSize, size_t domain, bool ortogonalCluster) {
		physics.makeStiffnessMatrixRegular(regularization, scSize, domain, ortogonalCluster);

		storeWrapper(mNames(Matrices::N) + "[domain " + std::to_string(domain) + "]", Matrices::N, domain);
	};
}

void Assembler::setRegularizationFromOrigKCallback()
{
	instance.computeKernelsFromOrigKCallback = [&] (FETI_REGULARIZATION regularization, size_t scSize, bool ortogonalCluster) {
		instance.K.swap(instance.origK);
		instance.N1.swap(instance.origKN1);
		instance.N2.swap(instance.origKN2);
		instance.RegMat.swap(instance.origRegMat);
		timeWrapper("regularize orig" + mNames(Matrices::K), [&] () {
			physics.makeStiffnessMatricesRegular(regularization, scSize, ortogonalCluster);
		});

		storeWrapper("orig" + mNames(Matrices::N), Matrices::N);
		instance.K.swap(instance.origK);
		instance.N1.swap(instance.origKN1);
		instance.N2.swap(instance.origKN2);
		instance.RegMat.swap(instance.origRegMat);
	};

	instance.computeKernelFromOrigKCallback = [&] (FETI_REGULARIZATION regularization, size_t scSize, size_t domain, bool ortogonalCluster) {
		instance.K[domain].swap(instance.origK[domain]);
		instance.N1[domain].swap(instance.origKN1[domain]);
		instance.N2[domain].swap(instance.origKN2[domain]);
		instance.RegMat[domain].swap(instance.origRegMat[domain]);
		physics.makeStiffnessMatrixRegular(regularization, scSize, domain, ortogonalCluster);

		storeWrapper("orig" + mNames(Matrices::N) + "[domain " + std::to_string(domain) + "]", Matrices::N, domain);
		instance.K[domain].swap(instance.origK[domain]);
		instance.N1[domain].swap(instance.origKN1[domain]);
		instance.N2[domain].swap(instance.origKN2[domain]);
		instance.RegMat[domain].swap(instance.origRegMat[domain]);
	};
}

void Assembler::setEmptyRegularizationCallback()
{
	instance.N1.clear();
	instance.N2.clear();
	instance.RegMat.clear();

	instance.N1.resize(instance.domains);
	instance.N2.resize(instance.domains);
	instance.RegMat.resize(instance.domains);

	instance.computeKernelsCallback = [&] (FETI_REGULARIZATION regularization, size_t scSize, bool ortogonalCluster) {
		storeWrapper(mNames(Matrices::N), Matrices::N);
	};

	instance.computeKernelCallback = [&] (FETI_REGULARIZATION regularization, size_t scSize, size_t domain, bool ortogonalCluster) {
		storeWrapper(mNames(Matrices::N) + "[domain " + std::to_string(domain) + "]", Matrices::N, domain);
	};
}

void Assembler::setB0Callback()
{
	instance.assembleB0Callback = [&] (FETI_B0_TYPE type, const std::vector<SparseMatrix> &kernels) {
		timeWrapper("compute B0", [&] () {
			instance.B0.clear();
			instance.B0.resize(instance.domains);
			for (size_t d = 0; d < instance.domains; d++) {
				instance.B0[d].type = 'G';
				instance.B0subdomainsMap[d].clear();
			}
			switch (type) {
			case FETI_B0_TYPE::CORNERS:
				physics.assembleB0FromCorners();
				break;
			case FETI_B0_TYPE::KERNELS:
				physics.assembleB0FromKernels(kernels);
				break;
			default:
				ESINFO(GLOBAL_ERROR) << "Unknown type of B0";
			}
		});

		storeWrapper(mNames(Matrices::B0), Matrices::B0);
	};
}



void Assembler::timeWrapper(const std::string &action, std::function<void(void)> operations)
{
	std::string fulldesc(physics.name() + ": " + action);

	ESINFO(PROGRESS2) << fulldesc;

	TimeEvent *event;
	if (_timeEvents.find(fulldesc) != _timeEvents.end()) {
		event = _timeEvents[fulldesc];
	} else {
		_timeEvents[fulldesc] = event = new TimeEvent(fulldesc);
		_timeStatistics->addPointerToEvent(event);
	}

	event->start();
	operations();
	event->endWithBarrier();
}

template<typename TType>
void storeData(TType &data, size_t domain, const std::string &name)
{
	std::ofstream os(Logging::prepareFile(domain, name));
	os.precision(10);
	os << data;
	os.close();
}


bool Assembler::checkForStore(const std::string &name)
{
	if (environment->print_matrices) {
		std::string fulldesc(physics.name() + ": store " + name);
		ESINFO(ALWAYS_ON_ROOT) << Info::TextColor::BLUE << fulldesc;
	}
	return environment->print_matrices;
}

void Assembler::storeMatrices(Matrices matrices, size_t domain)
{
	auto storeMatrix = [&] (std::vector<SparseMatrix> &data, Matrices matrix, const std::string &name) {
		if (matrices & matrix) {
			storeData(data[domain], domain, name);
		}
	};

	auto storeVector = [&] (std::vector<std::vector<double> > &data, Matrices matrix, const std::string &name) {
		if (matrices & matrix) {
			storeData(data[domain], domain, name);
		}
	};

	storeMatrix(instance.K, Matrices::K, "K");
	storeMatrix(instance.N1, Matrices::N, "N1");
	storeMatrix(instance.N2, Matrices::N, "N2");
	storeMatrix(instance.RegMat, Matrices::N, "RegMat");
	storeMatrix(instance.M, Matrices::M, "M");
	storeVector(instance.R, Matrices::R, "R");
	storeVector(instance.f, Matrices::f, "f");
	storeMatrix(instance.B0, Matrices::B0, "B0");
	storeMatrix(instance.B1, Matrices::B1, "B1");
	storeVector(instance.B1c, (Matrices::B1 | Matrices::B1c), "B1c");
	storeVector(instance.B1duplicity, (Matrices::B1 | Matrices::B1duplicity), "B1duplicity");
	storeVector(instance.primalSolution, Matrices::primal, "solution");
	storeVector(instance.dualSolution, Matrices::dual, "dualSolution");
}

void Assembler::storeWrapper(const std::string &name, Matrices matrices)
{
	if (checkForStore(name)) {
		for (size_t d = 0; d < instance.domains; d++) {
			storeMatrices(matrices, d);
		}
	}
}

void Assembler::storeWrapper(const std::string &name, Matrices matrices, size_t domain)
{
	if (checkForStore(name)) {
		storeMatrices(matrices, domain);
	}
}

void Assembler::storeWrapper(const std::string &name, std::vector<SparseMatrix> &matrices)
{
	if (checkForStore(name)) {
		for (size_t d = 0; d < matrices.size(); d++) {
			storeData(matrices[d], d, name);
		}
	}
}

void Assembler::storeWrapper(const std::string &name, std::vector<std::vector<double> > &data)
{
	if (checkForStore(name)) {
		for (size_t d = 0; d < data.size(); d++) {
			storeData(data[d], d, name);
		}
	}
}


