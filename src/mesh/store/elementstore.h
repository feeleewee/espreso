
#ifndef SRC_MESH_STORE_ELEMENTSTORE_H_
#define SRC_MESH_STORE_ELEMENTSTORE_H_

#include <cstddef>
#include <string>
#include <vector>

namespace espreso {

template <typename TEBoundaries, typename TEData> class serializededata;
struct Point;
struct Element;

struct ElementStore {

	void store(const std::string &file);

	void permute(const std::vector<eslocal> &permutation) { permute(permutation, distribution); }
	void permute(const std::vector<eslocal> &permutation, const std::vector<size_t> &distribution);

	std::vector<eslocal> gatherElementProcDistribution();

	eslocal size;
	std::vector<size_t> distribution;

	serializededata<eslocal, eslocal>* IDs;
	serializededata<eslocal, eslocal>* nodes;

	serializededata<eslocal, int>* body;
	serializededata<eslocal, int>* material;
	serializededata<eslocal, Element*>* epointers;

	serializededata<eslocal, eslocal>* dual;
	serializededata<eslocal, eslocal>* decomposedDual;

	eslocal firstDomain;
	eslocal ndomains;
	std::vector<size_t> domainDistribution;
	std::vector<size_t> domainElementDistribution;
	std::vector<int> clusters;

	ElementStore(std::vector<Element*> &eclasses);
	~ElementStore();

private:
	std::vector<Element*> &_eclasses;
};

}



#endif /* SRC_MESH_STORE_ELEMENTSTORE_H_ */
