#include "boundaries.h"

using namespace mesh;

std::ostream& mesh::operator<<(std::ostream& os, const Boundaries &b)
{
	std::set<eslocal>::const_iterator it;

	for (size_t i = 0; i < b._boundaries.size(); i++) {
		os << i << ": ";
		for (it = b._boundaries[i].begin(); it != b._boundaries[i].end(); ++it) {
			os << *it << " ";
		}
		os << "\n";
	}
	return os;
}




