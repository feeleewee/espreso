
#ifndef SRC_OUTPUT_RESULT_VISUALIZATION_VTKLEGACY_H_
#define SRC_OUTPUT_RESULT_VISUALIZATION_VTKLEGACY_H_

#include <string>
#include <vector>

#include "distributedvisualization.h"

namespace espreso {

struct VTKLegacy: public DistributedVisualization {

protected:
	VTKLegacy(const Mesh &mesh, double clusterShrinkRatio, double domainShrinkRatio);

	void mesh(const std::string &name);
	void solution(const std::string &name);
	void nodesIntervals(const std::string &name);
	void externalIntervals(const std::string &name);
	void sharedInterface(const std::string &name);
	void domainSurface(const std::string &name);
	void corners(const std::string &name);
	void sFixPoints(const std::string &name);
	void iFixPoints(const std::string &name);

	void points(const std::string &name, const std::vector<eslocal> &points, const std::vector<eslocal> &distribution);

	double _clusterShrinkRatio, _domainShrinkRatio;
};

struct VTKLegacyDebugInfo: public VTKLegacy {

	VTKLegacyDebugInfo(const Mesh &mesh, double clusterShrinkRatio, double domainShrinkRatio);

	void updateMesh()
	{
		mesh(_path + "mesh");
		nodesIntervals(_path + "nodeintervals");
		externalIntervals(_path + "externalIntervals");
		sharedInterface(_path + "sharedinterfaces");
		domainSurface(_path + "domainSurface");
		corners(_path + "corners");
		sFixPoints(_path + "sfixpoints");
		iFixPoints(_path + "ifixpoints");
	}
	void updateSolution(const Step &step)
	{
		solution(_path + "solution");
	}

protected:
	std::string _path;
};

}


#endif /* SRC_OUTPUT_RESULT_VISUALIZATION_VTKLEGACY_H_ */
