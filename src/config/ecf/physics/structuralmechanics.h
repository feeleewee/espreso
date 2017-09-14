
#ifndef SRC_CONFIG_ECF_PHYSICS_STRUCTURALMECHANICS_H_
#define SRC_CONFIG_ECF_PHYSICS_STRUCTURALMECHANICS_H_

#include "physics.h"
#include "physicssolver/loadstep.h"

namespace espreso {

struct StructuralMechanicsLoadStepConfiguration: public LoadStepConfiguration {

	std::map<std::string, std::string> angular_velocity, temperature, displacement, acceleration, normal_pressure, obstacle, normal_direction;

	StructuralMechanicsLoadStepConfiguration(DIMENSION dimension);
};

struct StructuralMechanicsConfiguration: public PhysicsConfiguration {

	enum class ELEMENT_BEHAVIOUR {
		PLANE_STRAIN = 0,
		AXISYMMETRIC = 1,
		PLANE_STRESS = 2,
		PLANE_STRESS_WITH_THICKNESS = 3
	};

	ELEMENT_BEHAVIOUR element_behaviour;

	std::map<size_t, StructuralMechanicsLoadStepConfiguration> load_steps_settings;

	StructuralMechanicsConfiguration(DIMENSION dimension);
};

}


#endif /* SRC_CONFIG_ECF_PHYSICS_STRUCTURALMECHANICS_H_ */
