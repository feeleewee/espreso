
#ifndef SRC_CONFIGURATION_PHYSICS_ADVECTIONDIFFUSION2D_H_
#define SRC_CONFIGURATION_PHYSICS_ADVECTIONDIFFUSION2D_H_

#include "../material/coordinatesystem.h"
#include "../material/holder.h"
#include "solver.h"
#include "../solver.h"
#include "advectiondiffusionconvection.h"
#include "advectiondiffusionradiation.h"
#include "advectiondiffusionsolver.h"

namespace espreso {

struct AdvectionDiffusion2DMaterial: public Configuration {

	PARAMETER(MaterialParam<MATERIAL_PARAMETER::DENSITY>                , density, "Density", {"0"});
	PARAMETER(MaterialParam<MATERIAL_PARAMETER::HEAT_CAPACITY>          , Cp     , "Termal capacity."       , {"0"});
	PARAMETER(MaterialParam<MATERIAL_PARAMETER::THERMAL_CONDUCTIVITY_XX>, KXX    , "Termal conductivity XX.", {"1"});
	PARAMETER(MaterialParam<MATERIAL_PARAMETER::THERMAL_CONDUCTIVITY_YY>, KYY    , "Termal conductivity YY.", {"1"});
	PARAMETER(MaterialParam<MATERIAL_PARAMETER::THERMAL_CONDUCTIVITY_XY>, KXY    , "Termal conductivity XY.", {"1"});
	PARAMETER(MaterialParam<MATERIAL_PARAMETER::THERMAL_CONDUCTIVITY_YX>, KYX    , "Termal conductivity YX.", {"1"});

	OPTION(MATERIAL_MODEL, model, "Material model", MATERIAL_MODEL::ISOTROPIC, OPTIONS({
		{ "ISOTROPIC"  , MATERIAL_MODEL::ISOTROPIC  , "Isotropic." },
		{ "DIAGONAL"   , MATERIAL_MODEL::DIAGONAL   , "Diagonal." },
		{ "SYMMETRIC"  , MATERIAL_MODEL::SYMMETRIC  , "Symmetric." },
		{ "ANISOTROPIC", MATERIAL_MODEL::ANISOTROPIC, "Anisotropic." }
	}));

	SUBCONFIG(CoordinateSystem, coordinate_system, "Element coordinate system.");
};

struct AdvectionDiffusion2DConfiguration: public Configuration {

	enum class STABILIZATION {
		SUPG = 0,
		CAU = 1
	};

	OPTION(STABILIZATION, stabilization, "The type of the stabilization.", STABILIZATION::SUPG, OPTIONS({
		{ "SUPG", STABILIZATION::SUPG, "SUPG stabilization." },
		{ "CAU" , STABILIZATION::CAU , "CAU stabilization." },
	}));
	PARAMETER(double, sigma, "Inconsistent stabilization parameters.", 0);
	PARAMETER(bool, tangent_matrix_correction, "Add derivation matrix to stiffness matrix.", 0);

	PARAMETER(bool, newassembler, "New version of assembler.", 1);

	SUBCONFIG(PhysicsSolver<AdvectionDiffusionNonLinearConvergence>, physics_solver, "Settings of physics solver.");

	OPTION(SOLVER_LIBRARY, solver_library, "Linear solver used for computing a system.", SOLVER_LIBRARY::ESPRESO, OPTIONS({
		{ "ESPRESO", SOLVER_LIBRARY::ESPRESO, "ESPRESO solver [FETI methods]" },
		{ "HYPRE"  , SOLVER_LIBRARY::HYPRE  , "Hypre solver [multigrid methods]" },
	}));

	SUBCONFIG(ESPRESOSolver, espreso, "Internal FETI solver options.");
	SUBCONFIG(HypreSolver  , hypre  , "Multigrid solver setting.");

	SUBMAPTOMAP(size_t, std::string, std::string, heat_flux, "Heat flux", "1", "Heat flux settings for load step '1'", "<REGION>", "<EXPRESSION>");
	SUBMAPTOMAP(size_t, std::string, std::string, heat_flow, "Heat flow", "1", "Heat flow settings for load step '1'", "<REGION>", "<EXPRESSION>");

	SUBMAPTOMAPTOCONFIG(size_t, std::string, AdvectionDiffusionConvection, convection, "Region with convective heat flux",
			"<REGION_NAME>", "Convection for a given region", "1", "Settings for load step '1'");
	SUBMAPTOMAPTOCONFIG(size_t, std::string, AdvectionDiffusionRadiation, diffuse_radiation, "Region with diffuse radiation",
			"<REGION_NAME>", "Diffuse radiation for a given region", "1", "Settings for load step '1'");

	SUBMAP(std::string, std::string, initial_temperature , "Regions initial temperature", "<REGION>", "<EXPRESSION>");

	SUBMAPTOMAP(size_t, std::string, std::string, temperature        , "Temperature"       , "1", "Temperature settings for load step '1'", "<REGION>", "<EXPRESSION>");
	SUBMAPTOMAP(size_t, std::string, std::string, heat_source        , "Heat source"       , "1", "Heat source settings for load step '1'", "<REGION>", "<EXPRESSION>");
	SUBMAPTOMAP(size_t, std::string, std::string, translation_motions, "Translation motion", "1", "Translation motion settings for load step '1'", "<REGION>", "<EXPRESSION>");
	SUBMAPTOMAP(size_t, std::string, std::string, thickness          , "Thickness"         , "1", "Thickness settings for load step '1'", "<REGION>", "<EXPRESSION>");

	SUBMAPTOCONFIG(std::string, AdvectionDiffusion2DMaterial, materials, "Material description.", "<MATERIAL_NAME>", "Material description");
	SUBMAP(std::string, std::string, material_set, "Assign materials to regions", "<REGION>", "<MATERIAL_NAME>");

	PARAMETER(bool, post_process, "Turn on/off results post processing.", true);

};

}



#endif /* SRC_CONFIGURATION_PHYSICS_ADVECTIONDIFFUSION2D_H_ */
