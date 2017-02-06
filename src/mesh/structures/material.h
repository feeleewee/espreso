
#ifndef SRC_MESH_MATERIALS_MATERIAL_H_
#define SRC_MESH_MATERIALS_MATERIAL_H_

#include <vector>
#include <string>
#include <algorithm>

#include "../../configuration/materialparameters.h"
#include "../../configuration/physics.h"

namespace espreso {

class Coordinates;
class Evaluator;
struct Configuration;

class Material {

public:
	Material(const Coordinates &coordinates);
	Material(const Coordinates &coordinates, const Configuration &configuration);

	virtual ~Material();

	const Evaluator* get(MATERIAL_PARAMETER parameter) const { return _values[static_cast<int>(parameter)]; }
	MATERIAL_MODEL getModel(PHYSICS physics) const { return _models[(size_t)physics]; }

	void set(MATERIAL_PARAMETER parameter, const std::string &value);
	void set(MATERIAL_PARAMETER parameter, Evaluator* value);
	void setModel(PHYSICS physics, MATERIAL_MODEL model) { _models[(size_t)physics] = model; }

	void store(std::ofstream& os);
	void load(std::ifstream& is);

protected:
	const Coordinates &_coordinates;

	std::vector<MATERIAL_MODEL> _models;
	std::vector<Evaluator*> _values;
};

}



#endif /* SRC_MESH_MATERIALS_MATERIAL_H_ */
