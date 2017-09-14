
#ifndef SRC_CONFIG_ECF_PHYSICS_HEATTRANSFER_H_
#define SRC_CONFIG_ECF_PHYSICS_HEATTRANSFER_H_

#include "physics.h"
#include "physicssolver/loadstep.h"

namespace espreso {

struct ConvectionConfiguration: public ECFObject {

	enum class TYPE {
		USER,
		EXTERNAL_NATURAL,
		INTERNAL_NATURAL,
		EXTERNAL_FORCED,
		INTERNAL_FORCED
	};

	enum class VARIANT {
		VERTICAL_WALL,
		INCLINED_WALL,
		HORIZONTAL_CYLINDER,
		SPHERE,
		HORIZONTAL_PLATE_UP,
		HORIZONTAL_PLATE_DOWN,
		AVERAGE_PLATE,
		PARALLEL_PLATES,
		CIRCULAR_TUBE,
		TUBE
	};

	enum class FLUID {
		AIR,
		WATER,
		ENGINE_OIL,
		TRANSFORMER_OIL,
	};

	TYPE type;
	VARIANT variant;
	FLUID fluid;

	std::string heat_transfer_coefficient, external_temperature;
	std::string wall_height, tilt_angle, diameter, plate_length, fluid_velocity, plate_distance, length, absolute_pressure;

	ConvectionConfiguration();
};

struct RadiationConfiguration: public ECFObject {

	std::string emissivity, external_temperature;

	RadiationConfiguration();
};

struct HeatTransferLoadStepConfiguration: public LoadStepConfiguration {

	std::map<std::string, std::string> temperature, heat_source, translation_motions, heat_flux, heat_flow, thickness;
	std::map<std::string, ConvectionConfiguration> convection;
	std::map<std::string, RadiationConfiguration> diffuse_radiation;

	HeatTransferLoadStepConfiguration(DIMENSION dimension);
};

struct HeatTransferConfiguration: public PhysicsConfiguration {

	enum class STABILIZATION {
		SUPG = 0,
		CAU = 1
	};

	STABILIZATION stabilization;
	double sigma;

	std::map<size_t, HeatTransferLoadStepConfiguration> load_steps_settings;

	HeatTransferConfiguration(DIMENSION dimension);
};

}


#endif /* SRC_CONFIG_ECF_PHYSICS_HEATTRANSFER_H_ */
