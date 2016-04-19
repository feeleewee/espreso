
#ifndef INPUT_MESHGENERATOR_UNIFORMMESH_SETTINGS_H_
#define INPUT_MESHGENERATOR_UNIFORMMESH_SETTINGS_H_

#include "../settings.h"
#include "esmesh.h"

namespace espreso {
namespace input {

struct UniformSettings: public Settings {

	UniformSettings(const Options &options, size_t index, size_t size);
	UniformSettings(size_t index, size_t size);

	std::vector<Description> description;

	size_t subdomainsInCluster[3];
	size_t elementsInSubdomain[3];
	size_t materialsLayers[3];

	size_t cornerCount;
	bool corners;
	bool edges;
	bool faces;
};

inline std::ostream& operator<<(std::ostream& os, const UniformSettings &s)
{
	os << Settings(s);
	os << "subdomainsInCluster: " << s.subdomainsInCluster[0] << " : " << s.subdomainsInCluster[1] << " : " << s.subdomainsInCluster[2] << "\n";
	os << "elementsInSubdomain: " << s.elementsInSubdomain[0] << " : " << s.elementsInSubdomain[1] << " : " << s.elementsInSubdomain[2] << "\n";
	os << "materialsLayers: " << s.materialsLayers[0] << " : " << s.materialsLayers[1] << " : " << s.materialsLayers[2] << "\n";
	return os;
}

}
}



#endif /* INPUT_MESHGENERATOR_UNIFORMMESH_SETTINGS_H_ */
