
#include "elementsregionstore.h"

#include "surfacestore.h"

#include "../elements/element.h"

#include "../../basis/containers/serializededata.h"
#include "../../basis/utilities/parser.h"
#include "../../basis/utilities/utils.h"

using namespace espreso;

ElementsRegionStore::ElementsRegionStore(const std::string &name)
: name(name),

  elements(NULL),
  uniqueElements(NULL),

  nodes(NULL),

  uniqueOffset(0),
  uniqueSize(0),
  uniqueTotalSize(0),

  ecounters(static_cast<int>(Element::CODE::SIZE)),

  surface(NULL)
{

}

ElementsRegionStore::~ElementsRegionStore()
{
	if (elements == NULL) { delete elements; }
	if (uniqueElements != NULL && uniqueElements != elements) {
		delete uniqueElements;
	}
	if (surface != NULL) {
		delete surface;
	}
}

size_t ElementsRegionStore::packedSize() const
{
	if (elements == NULL) {
		return 0;
	}
	return
			Esutils::packedSize(name) +
			Esutils::packedSize(uniqueOffset) +
			Esutils::packedSize(uniqueSize) +
			Esutils::packedSize(uniqueTotalSize) +
			elements->packedSize() + Esutils::packedSize(eintervals) +
			nodes->packedSize() +
			Esutils::packedSize(nintervals) +
			Esutils::packedSize(ecounters);
}

void ElementsRegionStore::pack(char* &p) const
{
	if (elements == NULL) {
		return;
	}
	Esutils::pack(name, p);
	Esutils::pack(uniqueOffset, p);
	Esutils::pack(uniqueSize, p);
	Esutils::pack(uniqueTotalSize, p);
	elements->pack(p);
	Esutils::pack(eintervals, p);
	nodes->pack(p);
	Esutils::pack(nintervals, p);
	Esutils::pack(ecounters, p);
}

void ElementsRegionStore::unpack(const char* &p)
{
	Esutils::unpack(name, p);
	Esutils::unpack(uniqueOffset, p);
	Esutils::unpack(uniqueSize, p);
	Esutils::unpack(uniqueTotalSize, p);
	if (elements == NULL) {
		elements = new serializededata<eslocal, eslocal>(1, tarray<eslocal>(1, 0));
	}
	elements->unpack(p);
	Esutils::unpack(eintervals, p);
	if (nodes == NULL) {
		nodes = new serializededata<eslocal, eslocal>(1, tarray<eslocal>(1, 0));
	}
	nodes->unpack(p);
	Esutils::unpack(nintervals, p);
	Esutils::unpack(ecounters, p);
}

