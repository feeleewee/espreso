
#ifndef SRC_INPUT_GENERATOR_SELECTIONS_BLOCKBORDER_H_
#define SRC_INPUT_GENERATOR_SELECTIONS_BLOCKBORDER_H_

#include "../primitives/triple.h"
#include "../elements/element.h"
#include <string>

namespace espreso {

struct BlockSettings;

struct BlockBorder {
	BlockBorder(const std::string &interval);

	Triple<esglobal> start, end;
	Triple<bool> excludeStart, excludeEnd;

	size_t dimension() const;
	BlockBorder intersect(const BlockSettings &block) const;

	CubeFace getFace(const BlockSettings &block) const;
	CubeEdge getEdge(const BlockSettings &block) const;
};

}


#endif /* SRC_INPUT_GENERATOR_SELECTIONS_BLOCKBORDER_H_ */

