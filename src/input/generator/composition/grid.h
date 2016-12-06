
#ifndef SRC_INPUT_GENERATOR_COMPOSITION_GRID_H_
#define SRC_INPUT_GENERATOR_COMPOSITION_GRID_H_

#include "../../loader.h"

#include "../primitives/triple.h"

namespace espreso {

struct GridConfiguration;
enum class ELEMENT_TYPE;

namespace input {

struct BlockGenerator;

struct GridSettings {

	GridSettings();
	GridSettings(const GridConfiguration &configuration);

	ELEMENT_TYPE etype;

	Triple<double> start, end;
	Triple<Expression> projection, rotation;
	Triple<size_t> blocks, clusters, domains, elements;

	std::vector<bool> nonempty;
	bool uniformDecomposition;
};

class Grid: public Loader {

public:
	Grid(const GlobalConfiguration &configuration, Mesh &mesh, size_t index, size_t size);
	virtual ~Grid();

	static void load(const GlobalConfiguration &configuration, Mesh &mesh, size_t index, size_t size);

	virtual void points(Coordinates &coordinates);
	virtual void elements(std::vector<Element*> &elements, std::vector<Element*> &faces, std::vector<Element*> &edges);
	virtual void materials(std::vector<Material> &materials);
	virtual void neighbours(std::vector<Element*> &nodes, std::vector<int> &neighbours);
	virtual void regions(
			std::vector<Evaluator*> &evaluators,
			std::vector<Region> &regions,
			std::vector<Element*> &elements,
			std::vector<Element*> &faces,
			std::vector<Element*> &edges,
			std::vector<Element*> &nodes);

	virtual bool partitiate(const std::vector<Element*> &nodes, std::vector<eslocal> &partsPtrs, std::vector<std::vector<Element*> > &fixPoints, std::vector<Element*> &corners);

protected:
	GridSettings _settings;
	BlockGenerator* _block;
	Triple<size_t> _clusterOffset;
	Triple<size_t> _subnodes;

	size_t _index;
	size_t _size;

	std::vector<int> _cMap;
	int _clusterIndex;
};


}
}



#endif /* SRC_INPUT_GENERATOR_COMPOSITION_GRID_H_ */
