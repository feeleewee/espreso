#include "element.h"
#include <iomanip>
#include <iostream>

using namespace mesh;

std::ostream& mesh::operator<<(std::ostream& os, const Element &e)
{
	for (size_t i = 0; i < e.size(); i++) {
		os << e.node(i) << " ";
	}
	return os;
}

std::ofstream& mesh::operator<<(std::ofstream& os, const Element &e)
{
	eslocal value = e.vtkCode();
	os.write(reinterpret_cast<const char *>(&value), sizeof(eslocal));
	os.write(reinterpret_cast<const char *>(e.indices()), sizeof(eslocal) * e.size());
	return os;
}
