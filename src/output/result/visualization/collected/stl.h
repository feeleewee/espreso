
#ifndef SRC_OUTPUT_RESULT_VISUALIZATION_COLLECTED_STL_H_
#define SRC_OUTPUT_RESULT_VISUALIZATION_COLLECTED_STL_H_

#include <string>
#include <sstream>

#include "collectedvisualization.h"
#include "../stlwritter.h"

namespace espreso {

struct Step;
class Mesh;

struct STL: public CollectedVisualization {
	STL(const std::string &name, const Mesh &mesh, const OutputConfiguration &configuration);
	~STL();

	void updateMesh();
	void updateSolution(const Step &step);

protected:
	std::string _path;
	std::string _name;

	const STLBinaryWriter _writer;
};

}


#endif /* SRC_OUTPUT_RESULT_VISUALIZATION_COLLECTED_STL_H_ */
