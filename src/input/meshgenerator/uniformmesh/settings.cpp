#include "settings.h"

using namespace espreso::input;

void UniformSettings::defaultUniformSettings()
{
	for (size_t i = 0; i < 3; i++) { // x, y, z
		subdomainsInCluster[i] = 2;
		elementsInSubdomain[i] = 5;
		materialsLayers[i] = 1;
	}

	cornerCount = 0;
	corners     = true;
	edges       = false;
	faces       = false;

	parameters = {
		{"SUBDOMAINS_X", subdomainsInCluster[0], "Number of sub-domains in a cluster in x-axis."},
		{"SUBDOMAINS_Y", subdomainsInCluster[1], "Number of sub-domains in a cluster in y-axis."},
		{"SUBDOMAINS_Z", subdomainsInCluster[2], "Number of sub-domains in a cluster in z-axis."},

		{"ELEMENTS_X", elementsInSubdomain[0], "Number of elements in a sub-domain in x-axis."},
		{"ELEMENTS_Y", elementsInSubdomain[1], "Number of elements in a sub-domain in y-axis."},
		{"ELEMENTS_Z", elementsInSubdomain[2], "Number of elements in a sub-domain in z-axis."},

		{"MATERIALS_X", materialsLayers[0], "Number of materials layers in x-axis."},
		{"MATERIALS_Y", materialsLayers[1], "Number of materials layers in y-axis."},
		{"MATERIALS_Z", materialsLayers[2], "Number of materials layers in z-axis."},

		{"CORNER_COUNT"      , cornerCount, "The number of corners."},
		{"CORNERS_IN_CORNERS", corners    , "Set corners to corners points."},
		{"CORNERS_IN_EDGES"  , edges      , "Set corners on edges."},
		{"CORNERS_IN_FACES"  , faces      , "Set corners on faces."},
	};
}

UniformSettings::UniformSettings(const Configuration &configuration, size_t index, size_t size)
: Settings(index, size)
{
	defaultUniformSettings();
	parameters.insert(parameters.end(), Settings::parameters.begin(), Settings::parameters.end());
	ParametersReader::configuration(configuration, parameters);
}

UniformSettings::UniformSettings(size_t index, size_t size)
: Settings(index, size)
{
	defaultUniformSettings();
	parameters.insert(parameters.end(), Settings::parameters.begin(), Settings::parameters.end());
}