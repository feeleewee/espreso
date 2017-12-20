
#include "../../configuration.hpp"
#include "heattransfer.h"

espreso::ConvectionConfiguration::ConvectionConfiguration()
{
	type = TYPE::USER;
	REGISTER(type, ECFMetaData()
			.setdescription({ "Type of convection." })
			.setdatatype({ ECFDataType::OPTION })
			.addoption(ECFOption().setname("USER").setdescription("User defined."))
			.addoption(ECFOption().setname("EXTERNAL_NATURAL").setdescription("External natural."))
			.addoption(ECFOption().setname("INTERNAL_NATURAL").setdescription("Internal natural."))
			.addoption(ECFOption().setname("EXTERNAL_FORCED").setdescription("External forced."))
			.addoption(ECFOption().setname("INTERNAL_FORCED").setdescription("Internal forced.")));

	variant = VARIANT::VERTICAL_WALL;
	REGISTER(variant, ECFMetaData()
			.setdescription({ "Convection variant." })
			.setdatatype({ ECFDataType::OPTION })
			.addoption(ECFOption().setname("VERTICAL_WALL").setdescription("Vertical wall."))
			.addoption(ECFOption().setname("INCLINED_WALL").setdescription("Inclined wall."))
			.addoption(ECFOption().setname("HORIZONTAL_CYLINDER").setdescription("Horizontal cylinder."))
			.addoption(ECFOption().setname("SPHERE").setdescription("Sphere."))
			.addoption(ECFOption().setname("HORIZONTAL_PLATE_UP").setdescription("Horizontal place up."))
			.addoption(ECFOption().setname("HORIZONTAL_PLATE_DOWN").setdescription("Horizontal plate down."))
			.addoption(ECFOption().setname("AVERAGE_PLATE").setdescription("Average plate."))
			.addoption(ECFOption().setname("PARALLEL_PLATES").setdescription("Parallel plates."))
			.addoption(ECFOption().setname("CIRCULAR_TUBE").setdescription("Circular tube."))
			.addoption(ECFOption().setname("TUBE").setdescription("Tube.")));

	fluid = FLUID::AIR;
	REGISTER(fluid, ECFMetaData()
			.setdescription({ "Fluid type." })
			.setdatatype({ ECFDataType::OPTION })
			.addoption(ECFOption().setname("AIR").setdescription("Air."))
			.addoption(ECFOption().setname("WATER").setdescription("Water."))
			.addoption(ECFOption().setname("ENGINE_OIL").setdescription("Engine oil."))
			.addoption(ECFOption().setname("TRANSFORMER_OIL").setdescription("Tranformer oil.")));

	heat_transfer_coefficient = external_temperature = "0";
	REGISTER(heat_transfer_coefficient, ECFMetaData()
			.setdescription({ "Heat transfer coefficient." })
			.setdatatype({ ECFDataType::EXPRESSION }));
	REGISTER(external_temperature, ECFMetaData()
			.setdescription({ "External temperature." })
			.setdatatype({ ECFDataType::EXPRESSION }));

	wall_height = tilt_angle = diameter = plate_length = fluid_velocity = plate_distance = length = absolute_pressure = "0";
	REGISTER(wall_height, ECFMetaData()
			.setdescription({ "Wall height." })
			.setdatatype({ ECFDataType::EXPRESSION }));
	REGISTER(tilt_angle, ECFMetaData()
			.setdescription({ "Tilt angle." })
			.setdatatype({ ECFDataType::EXPRESSION }));
	REGISTER(diameter, ECFMetaData()
			.setdescription({ "Diameter." })
			.setdatatype({ ECFDataType::EXPRESSION }));
	REGISTER(plate_length, ECFMetaData()
			.setdescription({ "Plate length." })
			.setdatatype({ ECFDataType::EXPRESSION }));
	REGISTER(fluid_velocity, ECFMetaData()
			.setdescription({ "Fluid velocity." })
			.setdatatype({ ECFDataType::EXPRESSION }));
	REGISTER(plate_distance, ECFMetaData()
			.setdescription({ "Plate distance." })
			.setdatatype({ ECFDataType::EXPRESSION }));
	REGISTER(length, ECFMetaData()
			.setdescription({ "Length." })
			.setdatatype({ ECFDataType::EXPRESSION }));
	REGISTER(absolute_pressure, ECFMetaData()
			.setdescription({ "Absolute pressure." })
			.setdatatype({ ECFDataType::EXPRESSION }));
}

espreso::RadiationConfiguration::RadiationConfiguration()
{
	emissivity = external_temperature = "0";
	REGISTER(emissivity, ECFMetaData()
			.setdescription({ "Emissivity." })
			.setdatatype({ ECFDataType::EXPRESSION }));
	REGISTER(external_temperature, ECFMetaData()
			.setdescription({ "External temperature." })
			.setdatatype({ ECFDataType::EXPRESSION }));
}

espreso::HeatTransferLoadStepConfiguration::HeatTransferLoadStepConfiguration(DIMENSION dimension)
: LoadStepConfiguration("temperature", "heat")
{
	REGISTER(temperature, ECFMetaData()
			.setdescription({ "The name of a region.", "Temperature of a given region." })
			.setdatatype({ ECFDataType::REGION, ECFDataType::EXPRESSION })
			.setpattern({ "MY_REGION", "273.15" }));
	REGISTER(heat_source, ECFMetaData()
			.setdescription({ "The name of a region.", "Heat source of a given region." })
			.setdatatype({ ECFDataType::REGION, ECFDataType::EXPRESSION })
			.setpattern({ "MY_REGION", "273.15" }));
	REGISTER(translation_motions, ECFMetaData()
			.setdescription({ "The name of a region.", "Translation motion of a given region." })
			.setdatatype({ ECFDataType::REGION, ECFDataType::EXPRESSION })
			.setpattern({ "MY_REGION", dimension == DIMENSION::D2 ? "X 0, Y 0" : "X 0, Y 0, Z 0" }));
	REGISTER(heat_flux, ECFMetaData()
			.setdescription({ "The name of a region.", "Heat flux on a given region." })
			.setdatatype({ ECFDataType::REGION, ECFDataType::EXPRESSION })
			.setpattern({ "MY_REGION", "500" }));
	REGISTER(heat_flow, ECFMetaData()
			.setdescription({ "The name of a region.", "Heat flow on a given region." })
			.setdatatype({ ECFDataType::REGION, ECFDataType::EXPRESSION })
			.setpattern({ "MY_REGION", "500" }));

	REGISTER(convection, ECFMetaData()
			.setdescription({ "The name of a region.", "Convection on a given region." })
			.setdatatype({ ECFDataType::REGION })
			.setpattern({ "MY_REGION" }));
	REGISTER(diffuse_radiation, ECFMetaData()
			.setdescription({ "The name of a region.", "Radiation on a given region." })
			.setdatatype({ ECFDataType::REGION })
			.setpattern({ "MY_REGION" }));
}

espreso::HeatTransferConfiguration::HeatTransferConfiguration(DIMENSION dimension)
: PhysicsConfiguration(dimension, MaterialConfiguration::PHYSICAL_MODEL::THERMAL)
{
	REGISTER(
			materials,
			ECFMetaData()
				.setdescription({ "The name of a material.", "Material description." })
				.setdatatype({ ECFDataType::STRING })
                .setpattern({ "MY_MATERIAL" }),
			dimension, MaterialConfiguration::PHYSICAL_MODEL::THERMAL);
	moveLastBefore(PNAME(material_set));

	stabilization = STABILIZATION::SUPG;
	REGISTER(stabilization, ECFMetaData()
			.setdescription({ "Stabilization." })
			.setdatatype({ ECFDataType::OPTION })
			.addoption(ECFOption().setname("SUPG").setdescription("SUPG stabilization."))
			.addoption(ECFOption().setname("CAU").setdescription("CAU stabilization.")));

	sigma = 0;
	REGISTER(sigma, ECFMetaData()
			.setdescription({ "Inconsistent stabilization parameter." })
			.setdatatype({ ECFDataType::FLOAT }));

	REGISTER(
			load_steps_settings,
			ECFMetaData()
				.setdescription({ "Settings for each load step.", "Load step index." })
				.setdatatype({ ECFDataType::LOAD_STEP })
				.setpattern({ "1" }),
			dimension);
}


