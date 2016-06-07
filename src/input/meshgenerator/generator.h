
#ifndef INPUT_MESHGENERATOR_GENERATOR_H_
#define INPUT_MESHGENERATOR_GENERATOR_H_

#include "../loader.h"
#include "elements/elements.h"
#include "settings.h"

namespace espreso {
namespace input {

class Generator: public Loader {

protected:
	Generator(Mesh &mesh, const Settings &settings): Loader(mesh), _settings(settings)
	{
		switch (_settings.physics) {
		case config::assembler::PHYSICSalternative::LINEAR_ELASTICITY:
			config::assembler::PHYSICS = config::assembler::PHYSICSalternative::LINEAR_ELASTICITY;
			config::assembler::TIME_STEPS = 1;
			_DOFs = 3;
			break;
		case config::assembler::PHYSICSalternative::TEMPERATURE:
			config::assembler::PHYSICS = config::assembler::PHYSICSalternative::TEMPERATURE;
			config::assembler::TIME_STEPS = 1;
			_DOFs = 1;
			break;
		case config::assembler::PHYSICSalternative::TRANSIENT_ELASTICITY:
			config::assembler::PHYSICS = config::assembler::PHYSICSalternative::TRANSIENT_ELASTICITY;
			_DOFs = 3;
			break;
		default:
			ESINFO(ERROR) << "Unknown assembler: ASSEMBLER = " << static_cast<int>(_settings.physics);
		}
	};

	virtual ~Generator() { };

	void elements(std::vector<Element*> &elements)
	{
		elementsMesh(elements);
		elementsMaterials(elements);
	}

	virtual void elementsMesh(std::vector<Element*> &elements) = 0;
	virtual void elementsMaterials(std::vector<Element*> &elements) = 0;

	void materials(std::vector<Material> &materials)
	{
		materials = _settings.materials;
	}

	const Settings _settings;
	size_t _DOFs;
};

}
}


#endif /* INPUT_MESHGENERATOR_GENERATOR_H_ */
