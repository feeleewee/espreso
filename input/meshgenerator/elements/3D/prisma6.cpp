
#include "prisma6.h"

using namespace esinput;

size_t Prisma6::subelements = 2;
size_t Prisma6::subnodes[] = { 0, 0, 0 };

void Prisma6::addElements(std::vector<mesh::Element*> &elements, const eslocal indices[])
{
	eslocal prisma[8];
	prisma[0] = indices[0];
	prisma[1] = indices[1];
	prisma[2] = indices[3];
	prisma[3] = indices[3];
	prisma[4] = indices[4];
	prisma[5] = indices[5];
	prisma[6] = indices[7];
	prisma[7] = indices[7];
	elements.push_back(new mesh::Prisma6(prisma));

	prisma[0] = indices[0];
	prisma[1] = indices[3];
	prisma[2] = indices[2];
	prisma[3] = indices[2];
	prisma[4] = indices[4];
	prisma[5] = indices[7];
	prisma[6] = indices[6];
	prisma[7] = indices[6];
	elements.push_back(new mesh::Prisma6(prisma));
}


