
#ifndef SRC_ASSEMBLER_PHYSICS_TRANSIENT_ASSEMBLER_H_
#define SRC_ASSEMBLER_PHYSICS_TRANSIENT_ASSEMBLER_H_

#include "../assembler.h"

namespace espreso {

struct TransientPhysics: public Physics {

	virtual bool singular() const
	{
		return false;
	}

	virtual void assemble()
	{
		ESINFO(PROGRESS2) << "Assemble matrices K, M, and RHS";
		const std::map<eslocal, double> &forces_x = _mesh.coordinates().property(FORCES_X).values();
		const std::map<eslocal, double> &forces_y = _mesh.coordinates().property(FORCES_Y).values();
		const std::map<eslocal, double> &forces_z = _mesh.coordinates().property(FORCES_Z).values();

		K.resize(_mesh.parts());
		M.resize(_mesh.parts());
		f.resize(_mesh.parts());
		cilk_for (size_t p = 0; p < _mesh.parts(); p++) {
			composeSubdomain(p);
			K[p].mtype = mtype;

			const std::vector<eslocal> &l2g = _mesh.coordinates().localToCluster(p);
			for (eslocal i = 0; i < l2g.size(); i++) {
				size_t n = _mesh.subdomainBoundaries()[l2g[i]].size();
				if (forces_x.find(l2g[i]) != forces_x.end()) {
					f[p][DOFs.size() * i + 0] = forces_x.at(l2g[i]) / n;
				}
				if (forces_y.find(l2g[i]) != forces_y.end()) {
					f[p][DOFs.size() * i + 1] = forces_y.at(l2g[i]) / n;
				}
				if (forces_z.find(l2g[i]) != forces_z.end()) {
					f[p][DOFs.size() * i + 2] = forces_z.at(l2g[i]) / n;
				}
			}

			ESINFO(PROGRESS2) << Info::plain() << ".";
		}
		ESINFO(PROGRESS2);
	}

	virtual void save()
	{
		ESINFO(PROGRESS2) << "Save matrices K, M, RHS, and A constant";
		for (size_t p = 0; p < _mesh.parts(); p++) {
			std::ofstream osK(Logging::prepareFile(p, "K").c_str());
			osK << K[p];
			osK.close();

			std::ofstream osM(Logging::prepareFile(p, "M").c_str());
			osM << M[p];
			osM.close();

			std::ofstream osF(Logging::prepareFile(p, "f").c_str());
			osF << f[p];
			osF.close();
		}

		std::ofstream osA(Logging::prepareFile("A").c_str());
		osA << A;
		osA.close();
	}

	TransientPhysics(const Mesh &mesh, std::vector<DOFType> DOFs, SparseMatrix::MatrixType mtype): Physics(mesh, DOFs, mtype) {};
	virtual ~TransientPhysics() {};

	std::vector<SparseMatrix> M; // T will be deleted
	std::vector<double> A;

protected:
	virtual void composeSubdomain(size_t subdomain) =0;
};

}


#endif /* SRC_ASSEMBLER_PHYSICS_TRANSIENT_ASSEMBLER_H_ */
