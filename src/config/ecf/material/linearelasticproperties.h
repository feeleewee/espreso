
#ifndef SRC_CONFIG_ECF_MATERIAL_LINEARELASTICPROPERTIES_H_
#define SRC_CONFIG_ECF_MATERIAL_LINEARELASTICPROPERTIES_H_

#include "tensor.h"
#include "coordinatesystem.h" // DIMENSION

namespace espreso {

struct LinearElasticPropertiesConfiguration: public ECFObject {

	enum class MODEL {
		ISOTROPIC,
		ORTHOTROPIC,
		ANISOTROPIC
	};

	MODEL model;
	DIMENSION dimension;

	TensorConfiguration poisson_ratio;
	TensorConfiguration young_modulus;
	TensorConfiguration thermal_expansion;
	TensorConfiguration shear_modulus;
	TensorConfiguration anisotropic;

	LinearElasticPropertiesConfiguration();
};

}



#endif /* SRC_CONFIG_ECF_MATERIAL_LINEARELASTICPROPERTIES_H_ */
