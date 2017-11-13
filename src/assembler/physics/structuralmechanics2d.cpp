
#include "../../config/ecf/physics/structuralmechanics.h"
#include "structuralmechanics2d.h"

#include "../../basis/evaluators/evaluator.h"
#include "../step.h"
#include "../instance.h"
#include "../constraints/equalityconstraints.h"

#include "../../old/mesh/settings/property.h"
#include "../../old/mesh/elements/element.h"
#include "../../old/mesh/structures/mesh.h"
#include "../../old/mesh/structures/coordinates.h"
#include "../../old/mesh/structures/elementtypes.h"

#include "../../basis/matrices/denseMatrix.h"
#include "../../solver/specific/sparsesolvers.h"

using namespace espreso;

StructuralMechanics2D::StructuralMechanics2D(Mesh *mesh, Instance *instance, const StructuralMechanicsConfiguration &configuration, const ResultsSelectionConfiguration &propertiesConfiguration)
: Physics("STRUCTURAL MECHANICS 2D", mesh, instance, &configuration), StructuralMechanics(configuration, propertiesConfiguration)
{
//	_equalityConstraints = new EqualityConstraints(*_instance, *_mesh, _mesh->nodes(), _mesh->edges(), pointDOFs(), pointDOFsOffsets());
}

void StructuralMechanics2D::prepare()
{
//	_mesh->loadNodeProperty(_configuration.thickness, { }, { Property::THICKNESS }, 0);
//	for (size_t loadStep = 0; loadStep < _configuration.load_steps; loadStep++) {
//		_mesh->loadNodeProperty(_configuration.load_steps_settings.at(loadStep + 1).displacement, { "X", "Y" }, { Property::DISPLACEMENT_X, Property::DISPLACEMENT_Y }, loadStep);
//		_mesh->loadProperty(_configuration.load_steps_settings.at(loadStep + 1).acceleration    , { "X", "Y" }, { Property::ACCELERATION_X, Property::ACCELERATION_Y }, loadStep);
//		_mesh->loadProperty(_configuration.load_steps_settings.at(loadStep + 1).angular_velocity, { "X", "Y" }, { Property::ANGULAR_VELOCITY_X, Property::ANGULAR_VELOCITY_Y }, loadStep);
//	}
//
//	for (size_t loadStep = 0; loadStep < _configuration.load_steps; loadStep++) {
//		if (_configuration.load_steps_settings.at(loadStep + 1).solver == LoadStepConfiguration::SOLVER::FETI &&
//			_configuration.load_steps_settings.at(loadStep + 1).feti.regularization == FETI_REGULARIZATION::ANALYTIC) {
//
//				_mesh->computeFixPoints(4);
//				break;
//		}
//	}
//
//	StructuralMechanics::prepare();
}


void StructuralMechanics2D::analyticRegularization(size_t domain, bool ortogonalCluster)
{
//	if (_instance->K[domain].mtype != MatrixType::REAL_SYMMETRIC_POSITIVE_DEFINITE) {
//		ESINFO(ERROR) << "Cannot compute analytic regularization of not REAL_SYMMETRIC_POSITIVE_DEFINITE matrix. Set FETI_REGULARIZATION = ALGEBRAIC";
//	}
//
//	ESTEST(MANDATORY) << "Too few FIX POINTS: " << _mesh->fixPoints(domain).size() << (_mesh->fixPoints(domain).size() > 3 ? TEST_PASSED : TEST_FAILED);
//
//	Point center; size_t np; double norm;
//	if (ortogonalCluster) {
//		center = _cCenter[_mesh->getContinuityPartition()[domain]];
//		np = _cNp[_mesh->getContinuityPartition()[domain]];
//		norm = _cNorm[_mesh->getContinuityPartition()[domain]].x;
//	} else {
//		center = _dCenter[domain];
//		np = _dNp[domain];
//		norm = _dNorm[domain].x;
//	}
//
//	_instance->N1[domain].rows = 2 * _mesh->coordinates().localSize(domain);
//	_instance->N1[domain].cols = 3;
//	_instance->N1[domain].nnz = _instance->N1[domain].rows * _instance->N1[domain].cols;
//	_instance->N1[domain].type = 'G';
//
//	_instance->N1[domain].dense_values.reserve(_instance->N1[domain].nnz);
//
//	for (size_t c = 0; c < 2; c++) {
//		std::vector<double> kernel = { 0, 0 };
//		kernel[c] = 1 / std::sqrt(np);
//		for (size_t i = 0; i < _mesh->coordinates().localSize(domain); i++) {
//			_instance->N1[domain].dense_values.insert(_instance->N1[domain].dense_values.end(), kernel.begin(), kernel.end());
//		}
//	}
//
//	for (size_t i = 0; i < _mesh->coordinates().localSize(domain); i++) {
//		const Point &p = _mesh->coordinates().get(i, domain);
//		_instance->N1[domain].dense_values.push_back(-(p.y - center.y) / norm);
//		_instance->N1[domain].dense_values.push_back( (p.x - center.x) / norm);
//	}
//
//	SparseMatrix Nt; // CSR matice s DOFY
//	Nt.rows = 3;
//	Nt.cols = _instance->K[domain].cols;
//	Nt.nnz  = 4 * _mesh->fixPoints(domain).size();
//	Nt.type = 'G';
//
//	std::vector<eslocal> &ROWS = Nt.CSR_I_row_indices;
//	std::vector<eslocal> &COLS = Nt.CSR_J_col_indices;
//	std::vector<double>  &VALS = Nt.CSR_V_values;
//
//	ROWS.reserve(Nt.rows + 1);
//	COLS.reserve(Nt.nnz);
//	VALS.reserve(Nt.nnz);
//
//	ROWS.push_back(1);
//	ROWS.push_back(ROWS.back() + _mesh->fixPoints(domain).size());
//	ROWS.push_back(ROWS.back() + _mesh->fixPoints(domain).size());
//	ROWS.push_back(ROWS.back() + 2 * _mesh->fixPoints(domain).size());
//
//	for (size_t c = 0; c < 2; c++) {
//		std::vector<double> kernel = { 0, 0 };
//
//		kernel[c] = 1;
//		for (size_t i = 0; i < _mesh->fixPoints(domain).size(); i++) {
//			COLS.push_back(_mesh->fixPoints(domain)[i]->DOFIndex(domain, 0) + 1);
//			COLS.push_back(_mesh->fixPoints(domain)[i]->DOFIndex(domain, 1) + 1);
//			VALS.insert(VALS.end(), kernel.begin(), kernel.end());
//		}
//	}
//
//	for (size_t i = 0; i < _mesh->fixPoints(domain).size(); i++) {
//		const Point &p = _mesh->coordinates()[_mesh->fixPoints(domain)[i]->node(0)];
//		COLS.push_back(_mesh->fixPoints(domain)[i]->DOFIndex(domain, 0) + 1);
//		COLS.push_back(_mesh->fixPoints(domain)[i]->DOFIndex(domain, 1) + 1);
//		VALS.push_back(-p.y);
//		VALS.push_back( p.x);
//	}
//
//	SparseMatrix N;
//	Nt.MatTranspose( N );
//
//	_instance->RegMat[domain].MatMat(Nt, 'N', N);
//	_instance->RegMat[domain].MatTranspose();
//	_instance->RegMat[domain].RemoveLower();
//
//	SparseSolverCPU NtN;
//	NtN.ImportMatrix(_instance->RegMat[domain]);
//	_instance->RegMat[domain].Clear();
//
//	NtN.Factorization("Create RegMat");
//	NtN.SolveMat_Sparse(Nt);
//	NtN.Clear();
//
//	_instance->RegMat[domain].MatMat(N, 'N', Nt);
//	_instance->RegMat[domain].MatScale(_instance->K[domain].getDiagonalMaximum());
}

std::vector<std::pair<ElementType, Property> > StructuralMechanics2D::propertiesToStore() const
{
	return {};
}


void StructuralMechanics2D::assembleMaterialMatrix(const Step &step, const OldElement *e, eslocal node, double temp, DenseMatrix &K) const
{
//	const MaterialConfiguration* material = _mesh->materials()[e->param(OldElement::MATERIAL)];
//	double Ex, Ey, mi;
//
//	switch (material->linear_elastic_properties.model) {
//
//	case LinearElasticPropertiesConfiguration::MODEL::ISOTROPIC:
//		Ex = Ey = material->linear_elastic_properties.young_modulus.get(0, 0).evaluate(_mesh->coordinates()[e->node(node)], step.currentTime, temp);
//		mi = material->linear_elastic_properties.poisson_ratio.get(0, 0).evaluate(_mesh->coordinates()[e->node(node)], step.currentTime, temp);
//		break;
//
//	case LinearElasticPropertiesConfiguration::MODEL::ANISOTROPIC:
//		ESINFO(ERROR) << "Implement ANISOTROPIC MATERIAL";
//		break;
//
//	case LinearElasticPropertiesConfiguration::MODEL::ORTHOTROPIC:
//		Ex = material->linear_elastic_properties.young_modulus.get(0, 0).evaluate(_mesh->coordinates()[e->node(node)], step.currentTime, temp);
//		Ey = material->linear_elastic_properties.young_modulus.get(1, 1).evaluate(_mesh->coordinates()[e->node(node)], step.currentTime, temp);
//		mi = material->linear_elastic_properties.poisson_ratio.get(0, 0).evaluate(_mesh->coordinates()[e->node(node)], step.currentTime, temp);
//		break;
//
//	default:
//		ESINFO(ERROR) << "Linear elasticity 2D not supports set material model";
//	}
//
//	switch (material->linear_elastic_properties.model) {
//
//	case LinearElasticPropertiesConfiguration::MODEL::ISOTROPIC:
//	{
//
//		switch (_configuration.element_behaviour) {
//
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::PLANE_STRAIN:
//		{
//			double k = Ex * (1 - mi) / ((1 + mi) * (1 - 2 * mi));
//			K(node, 0) = k * 1;
//			K(node, 1) = k * 1;
//			K(node, 2) = k * ((1 - 2 * mi) / (2 * (1 - mi)));
//			K(node, 3) = k * (mi / (1 - mi));
//			K(node, 4) = 0;
//			K(node, 5) = 0;
//			K(node, 6) = k * (mi / (1 - mi));
//			K(node, 7) = 0;
//			K(node, 8) = 0;
//			return;
//		}
//
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::PLANE_STRESS:
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::PLANE_STRESS_WITH_THICKNESS:
//		{
//			double k = Ex / (1 - mi * mi);
//			K(node, 0) = k * 1;
//			K(node, 1) = k * 1;
//			K(node, 2) = k * ((1 -  mi) / 2);
//			K(node, 3) = k * mi;
//			K(node, 4) = 0;
//			K(node, 5) = 0;
//			K(node, 6) = k * mi;
//			K(node, 7) = 0;
//			K(node, 8) = 0;
//			return;
//		}
//
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::AXISYMMETRIC:
//		{
//			K.resize(e->nodes(), 16);
//			double k = Ex * (1 - mi) / ((1 + mi) * (1 - 2 * mi));
//			K(node,  0) = k * 1;
//			K(node,  1) = k * 1;
//			K(node,  2) = k * ((1 - 2 * mi) / (2 * (1 - mi)));
//			K(node,  3) = k * 1;
//
//			K(node,  4) = k * (mi / (1 - mi));
//			K(node,  5) = 0;
//			K(node,  6) = k * (mi / (1 - mi));
//			K(node,  7) = 0;
//			K(node,  8) = k * (mi / (1 - mi));
//			K(node,  9) = 0;
//
//			K(node, 10) = k * (mi / (1 - mi));
//			K(node, 11) = 0;
//			K(node, 12) = 0;
//			K(node, 13) = k * (mi / (1 - mi));
//			K(node, 14) = k * (mi / (1 - mi));
//			K(node, 15) = 0;
//			return;
//		}
//		}
//		break;
//	}
//
//	case LinearElasticPropertiesConfiguration::MODEL::ORTHOTROPIC:
//	{
//		ESINFO(ERROR) << "IMPLEMENT: MATERIAL_MODEL::LINEAR_ELASTIC_ORTHOTROPIC";
//		return;
//	}
//
//	case LinearElasticPropertiesConfiguration::MODEL::ANISOTROPIC:
//	{
//		ESINFO(ERROR) << "IMPLEMENT: MATERIAL_MODEL::LINEAR_ELASTIC_ANISOTROPIC";
//		return;
//	}
//
//	default:
//		ESINFO(ERROR) << "Structural mechanics 2D not supports set material model";
//	}
}

void StructuralMechanics2D::processElement(const Step &step, Matrices matrices, const OldElement *e, DenseMatrix &Ke, DenseMatrix &Me, DenseMatrix &Re, DenseMatrix &fe, const std::vector<Solution*> &solution) const
{
//	DenseMatrix Ce(4, 4), XY(1, 2), coordinates(e->nodes(), 2), J, invJ(2, 2), dND, B, precision, rhsT;
//	DenseMatrix K(e->nodes(), 9), TE(e->nodes(), 2), thickness(e->nodes(), 1), inertia(e->nodes(), 2), dens(e->nodes(), 1);
//	DenseMatrix gpK(e->nodes(), 9), gpTE(1, 2), gpThickness(1, 1), gpInertia(1, 2), gpDens(1, 1);
//	double detJ, temp, initTemp, CP = 1;
//	Point center;
//
//	const MaterialConfiguration* material = _mesh->materials()[e->param(OldElement::MATERIAL)];
//
//	for (size_t i = 0; i < e->nodes(); i++) {
//		initTemp = e->getProperty(Property::INITIAL_TEMPERATURE, step.step, _mesh->coordinates()[e->node(i)], step.currentTime, 0, 0);
//		temp = e->getProperty(Property::TEMPERATURE, step.step, _mesh->coordinates()[e->node(i)], step.currentTime, 0, initTemp);
//		inertia(i, 0) = e->sumProperty(Property::ACCELERATION_X, step.step, _mesh->coordinates()[e->node(i)], step.currentTime, temp, 0);
//		inertia(i, 1) = e->sumProperty(Property::ACCELERATION_Y, step.step, _mesh->coordinates()[e->node(i)], step.currentTime, temp, 0);
//		thickness(i, 0) = e->getProperty(Property::THICKNESS, step.step, _mesh->coordinates()[e->node(i)], step.currentTime, temp, 1);
//		coordinates(i, 0) = _mesh->coordinates()[e->node(i)].x;
//		coordinates(i, 1) = _mesh->coordinates()[e->node(i)].y;
//		center += _mesh->coordinates()[e->node(i)];
//		dens(i, 0) = material->density.evaluate(_mesh->coordinates()[e->node(i)], step.currentTime, temp);
//		switch (material->linear_elastic_properties.model) {
//		case LinearElasticPropertiesConfiguration::MODEL::ISOTROPIC:
//			TE(i, 0) = TE(i, 1) = (temp - initTemp) * material->linear_elastic_properties.thermal_expansion.get(0, 0).evaluate(_mesh->coordinates()[e->node(i)], step.currentTime, temp);
//			break;
//		case LinearElasticPropertiesConfiguration::MODEL::ORTHOTROPIC:
//			TE(i, 0) = (temp - initTemp) * material->linear_elastic_properties.thermal_expansion.get(0, 0).evaluate(_mesh->coordinates()[e->node(i)], step.currentTime, temp);
//			TE(i, 1) = (temp - initTemp) * material->linear_elastic_properties.thermal_expansion.get(1, 1).evaluate(_mesh->coordinates()[e->node(i)], step.currentTime, temp);
//			break;
//		default:
//			ESINFO(GLOBAL_ERROR) << "Invalid LINEAR ELASTIC model.";
//		}
//		assembleMaterialMatrix(step, e, i, temp, K);
//	}
//	center /= e->nodes();
//
//	eslocal Ksize = pointDOFs().size() * e->nodes();
//
//	Ke.resize(0, 0);
//	Me.resize(0, 0);
//	Re.resize(0, 0);
//	fe.resize(0, 0);
//	if (matrices & (Matrices::K | Matrices::R)) {
//		Ke.resize(Ksize, Ksize);
//		Ke = 0;
//	}
//	if (matrices & Matrices::M) {
//		Me.resize(Ksize, Ksize);
//		Me = 0;
//	}
//	if (matrices & Matrices::R) {
//		Re.resize(Ksize, 1);
//		Re = 0;
//	}
//	if (matrices & Matrices::f) {
//		fe.resize(Ksize, 1);
//		fe = 0;
//	}
//
//	for (size_t gp = 0; gp < e->gaussePoints(); gp++) {
//		J.multiply(e->dN()[gp], coordinates);
//		detJ = determinant2x2(J.values());
//		inverse2x2(J.values(), invJ.values(), detJ);
//
//		gpThickness.multiply(e->N()[gp], thickness);
//		gpK.multiply(e->N()[gp], K);
//		dND.multiply(invJ, e->dN()[gp]);
//		gpDens.multiply(e->N()[gp], dens);
//
//		if (matrices & Matrices::f) {
//			gpTE.multiply(e->N()[gp], TE);
//			gpInertia.multiply(e->N()[gp], inertia);
//			XY.multiply(e->N()[gp], coordinates);
//		}
//
//		if (matrices & Matrices::M) {
//			Me.multiply(e->N()[gp], e->N()[gp], gpDens(0, 0) * detJ * e->weighFactor()[gp] * CP, 1, true);
//		}
//
//		switch (_configuration.element_behaviour) {
//
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::PLANE_STRESS:
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::PLANE_STRAIN:
//			gpThickness(0, 0) = 1;
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::PLANE_STRESS_WITH_THICKNESS:
//
//			Ce.resize(3, 3);
//			Ce(0, 0) = gpK(0, 0);
//			Ce(1, 1) = gpK(0, 1);
//			Ce(2, 2) = gpK(0, 2);
//			Ce(0, 1) = gpK(0, 3);
//			Ce(0, 2) = gpK(0, 4);
//			Ce(1, 2) = gpK(0, 5);
//			Ce(1, 0) = gpK(0, 6);
//			Ce(2, 0) = gpK(0, 7);
//			Ce(2, 1) = gpK(0, 8);
//
//			B.resize(Ce.rows(), Ksize);
//			distribute3x2(B.values(), dND.values(), dND.rows(), dND.columns());
//
//			if (matrices & (Matrices::K | Matrices::R)) {
//				Ke.multiply(B, Ce * B, detJ * e->weighFactor()[gp] * gpThickness(0, 0), 1, true);
//			}
//
//			if (matrices & Matrices::f) {
//				precision.resize(Ce.rows(), 1);
//				precision(0, 0) = gpTE(0, 1);
//				precision(1, 0) = gpTE(0, 1);
//				precision(2, 0) = 0;
//				rhsT.multiply(B, Ce * precision, detJ * e->weighFactor()[gp] * gpThickness(0, 0), 0, true, false);
//				for (eslocal i = 0; i < Ksize; i++) {
//					fe(i, 0) += gpDens(0, 0) * detJ * e->weighFactor()[gp] * gpThickness(0, 0) * e->N()[gp](0, i % e->nodes()) * gpInertia(0, i / e->nodes());
//					fe(i, 0) += gpDens(0, 0) * detJ * e->weighFactor()[gp] * gpThickness(0, 0) * e->N()[gp](0, i % e->nodes()) * XY(0, i / e->nodes()) * pow(e->getProperty(Property::ANGULAR_VELOCITY_Z, step.step, center, step.currentTime, 0, 0), 2);
//					fe(i, 0) += rhsT(i, 0);
//				}
//			}
//			break;
//
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::AXISYMMETRIC:
//
//			Ce.resize(4, 4);
//			Ce(0, 0) = gpK(0,  0);
//			Ce(1, 1) = gpK(0,  1);
//			Ce(2, 2) = gpK(0,  2);
//			Ce(3, 3) = gpK(0,  3);
//			Ce(0, 1) = gpK(0,  4);
//			Ce(0, 2) = gpK(0,  5);
//			Ce(0, 3) = gpK(0,  6);
//			Ce(1, 2) = gpK(0,  7);
//			Ce(1, 3) = gpK(0,  8);
//			Ce(2, 3) = gpK(0,  9);
//			Ce(1, 0) = gpK(0, 10);
//			Ce(2, 0) = gpK(0, 11);
//			Ce(2, 1) = gpK(0, 12);
//			Ce(3, 0) = gpK(0, 13);
//			Ce(3, 1) = gpK(0, 14);
//			Ce(3, 2) = gpK(0, 15);
//
//			B.resize(Ce.rows(), Ksize);
//			distribute4x2(B.values(), dND.values(), dND.rows(), dND.columns());
//			for(size_t i = 0; i < e->N()[gp].columns(); i++) {
//				B(2, i) = e->N()[gp](0, i) / XY(0, 0);
//			}
//
//			if (matrices & (Matrices::K | Matrices::R)) {
//				Ke.multiply(B, Ce * B, detJ * e->weighFactor()[gp] * 2 * M_PI * XY(0, 0), 1, true);
//			}
//
//			if (matrices & Matrices::f) {
//				precision.resize(Ce.rows(), 1);
//				precision(0, 0) = gpTE(0, 0);
//				precision(1, 0) = gpTE(0, 1);
//				precision(2, 0) = precision(3, 0) = 0;
//				rhsT.multiply(B, Ce * precision, detJ * e->weighFactor()[gp] * 2 * M_PI * XY(0, 0), 0, true, false);
//				for (eslocal i = 0; i < Ksize; i++) {
//					fe(i, 0) += gpDens(0, 0) * detJ * e->weighFactor()[gp] * 2 * M_PI * XY(0, 0) * e->N()[gp](0, i % e->nodes()) * gpInertia(0, i / e->nodes());
//					fe(i, 0) += rhsT(i, 0);
//				}
//				for (eslocal i = 0; i < Ksize / 2; i++) {
//					fe(i, 0) += gpDens(0, 0) * detJ * e->weighFactor()[gp] * 2 * M_PI * XY(0, 0) * e->N()[gp](0, i % e->nodes()) * XY(0, 0) * pow(e->getProperty(Property::ANGULAR_VELOCITY_Y, step.step, center, step.currentTime, 0, 0), 2);
//					fe(Ksize / 2 + i, 0) += gpDens(0, 0) * detJ * e->weighFactor()[gp] * 2 * M_PI * XY(0, 0) * e->N()[gp](0, i % e->nodes()) * XY(0, 1) * pow(e->getProperty(Property::ANGULAR_VELOCITY_X, step.step, center, step.currentTime, 0, 0), 2);
//				}
//			}
//			break;
//		}
//	}
}

void StructuralMechanics2D::processFace(const Step &step, Matrices matrices, const OldElement *e, DenseMatrix &Ke, DenseMatrix &Me, DenseMatrix &Re, DenseMatrix &fe, const std::vector<Solution*> &solution) const
{
	ESINFO(ERROR) << "Structural mechanics 2D cannot process face";
}

void StructuralMechanics2D::processEdge(const Step &step, Matrices matrices, const OldElement *e, DenseMatrix &Ke, DenseMatrix &Me, DenseMatrix &Re, DenseMatrix &fe, const std::vector<Solution*> &solution) const
{
	if (!e->hasProperty(Property::PRESSURE, step.step)) {
		Ke.resize(0, 0);
		Me.resize(0, 0);
		Re.resize(0, 0);
		fe.resize(0, 0);
		return;
	}
	if (!(matrices & (Matrices::K | Matrices::f))) {
		Ke.resize(0, 0);
		Me.resize(0, 0);
		Re.resize(0, 0);
		fe.resize(0, 0);
		return;
	}

	DenseMatrix coordinates(e->nodes(), 2), dND(1, 2), P(e->nodes(), 1), normal(2, 1), matThickness(e->nodes(), 1), XY(1, 2);
	DenseMatrix gpP(1, 1), gpQ(1, 2), gpThickness(1, 1);

	eslocal Ksize = pointDOFs().size() * e->nodes();
	Ke.resize(0, 0);
	Me.resize(0, 0);
	Re.resize(0, 0);
	fe.resize(0, 0);

	if (matrices & Matrices::f) {
		fe.resize(Ksize, 1);
		fe = 0;
	}

//	for (size_t n = 0; n < e->nodes(); n++) {
//		coordinates(n, 0) = _mesh->coordinates()[e->node(n)].x;
//		coordinates(n, 1) = _mesh->coordinates()[e->node(n)].y;
//		P(n, 0) = e->getProperty(Property::PRESSURE, step.step, _mesh->coordinates()[e->node(n)], step.currentTime, 0, 0);
//		matThickness(n, 0) = e->getProperty(Property::THICKNESS, step.step, _mesh->coordinates()[e->node(n)], step.currentTime, 0, 1);
//	}
//
//	for (size_t gp = 0; gp < e->gaussePoints(); gp++) {
//		dND.multiply(e->dN()[gp], coordinates);
//		double J = dND.norm();
//		Point n(-dND(0, 1), dND(0, 0), 0);
//		e->rotateOutside(e->parentElements()[0], _mesh->coordinates(), n);
//		normal(0, 0) = n.x / J;
//		normal(1, 0) = n.y / J;
//		gpP.multiply(e->N()[gp], P);
//		gpQ.multiply(normal, gpP);
//		gpThickness.multiply(e->N()[gp], matThickness);
//
//		switch (_configuration.element_behaviour) {
//
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::PLANE_STRESS:
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::PLANE_STRAIN:
//			gpThickness(0, 0) = 1;
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::PLANE_STRESS_WITH_THICKNESS:
//			for (eslocal i = 0; i < Ksize; i++) {
//				fe(i, 0) += gpThickness(0, 0) * J * e->weighFactor()[gp] * e->N()[gp](0, i % e->nodes()) * gpQ(0, i / e->nodes());
//			}
//			break;
//
//		case StructuralMechanicsConfiguration::ELEMENT_BEHAVIOUR::AXISYMMETRIC:
//			XY.multiply(e->N()[gp], coordinates);
//			for (eslocal i = 0; i < Ksize; i++) {
//				fe(i, 0) += gpThickness(0, 0) * J * e->weighFactor()[gp] * 2 * M_PI * XY(0, 0) * e->N()[gp](0, i % e->nodes()) * gpQ(0, i / e->nodes());
//			}
//			break;
//		}
//	}
}

void StructuralMechanics2D::processNode(const Step &step, Matrices matrices, const OldElement *e, DenseMatrix &Ke, DenseMatrix &Me, DenseMatrix &Re, DenseMatrix &fe, const std::vector<Solution*> &solution) const
{
	if (
			e->hasProperty(Property::FORCE_X, step.step) ||
			e->hasProperty(Property::FORCE_Y, step.step)) {

		Ke.resize(0, 0);
		Me.resize(0, 0);
		Re.resize(0, 0);
		fe.resize(pointDOFs().size(), 0);

//		fe(0, 0) = e->sumProperty(Property::FORCE_X, step.step, _mesh->coordinates()[e->node(0)], step.currentTime, 0, 0);
//		fe(1, 0) = e->sumProperty(Property::FORCE_Y, step.step, _mesh->coordinates()[e->node(0)], step.currentTime, 0, 0);
		return;
	}
	Ke.resize(0, 0);
	Me.resize(0, 0);
	Re.resize(0, 0);
	fe.resize(0, 0);
}

void StructuralMechanics2D::postProcessElement(const Step &step, const OldElement *e, std::vector<Solution*> &solution)
{

}

void StructuralMechanics2D::processSolution(const Step &step)
{
}





