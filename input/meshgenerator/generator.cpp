
#include "generator.h"
#include "factory/factory.h"

using namespace esinput;


MeshGenerator::MeshGenerator(int argc, char** argv, int rank, int size)
{
	_generator = MeshFactory::create(argc, argv, rank, size);
}

void MeshGenerator::points(mesh::Coordinates &coordinates)
{
	_generator->points(coordinates);
}

void MeshGenerator::elements(std::vector<mesh::Element*> &elements, std::vector<eslocal> &parts)
{
	_generator->elements(elements, parts);
}

void MeshGenerator::fixPoints(std::vector<eslocal> &fixPoints)
{
	_generator->fixPoints(fixPoints);
}

