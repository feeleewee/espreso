
#ifndef SRC_ASSEMBLER_SOLUTION_H_
#define SRC_ASSEMBLER_SOLUTION_H_

#include <cstddef>
#include <vector>
#include <string>

#include "step.h"

namespace espreso {

class Mesh;
enum class ElementType;

struct Solution {

	Solution(const Mesh &mesh, const std::string &name, ElementType eType, size_t DOFs, std::vector<std::vector<double> > &data);
	Solution(const Mesh &mesh, const std::string &name, ElementType eType, size_t DOFs);

	void fill(double value);

	inline double get(size_t DOF, eslocal domain, eslocal index) const
	{
		return data[domain][index * DOFs + DOF];
	}

	std::string name;
	ElementType eType;
	size_t DOFs;
	std::vector<std::vector<double> > &data;

protected:
	// when no data are provided, store it here
	std::vector<std::vector<double> > _data;
};

}



#endif /* SRC_ASSEMBLER_SOLUTION_H_ */
