
#include "vtk.h"
#include "esconfig.h"

using namespace espreso::output;

void VTK_Full::coordinatesDisplacement(const std::vector<std::vector<double> > &displacement, size_t dofs)
{

	size_t size = 0;
	for (size_t p = 0; p < _mesh.parts(); p++) {
		size += _mesh.coordinates().localSize(p);
	}

	_vtk << "\n";
	_vtk << "POINT_DATA " << size << "\n";
	_vtk << "SCALARS displacements float " << dofs << "\n";
	_vtk << "LOOKUP_TABLE default\n";
	for (size_t p = 0; p < displacement.size(); p++) {
		for (size_t i = 0; i < displacement[p].size() / dofs; i++) {
			for (size_t d = 0; d < dofs; d++) {
				_vtk << displacement[p][dofs * i + d] << " ";
			}
			_vtk << "\n";
		}
	}
}

void VTK_Full::mesh(const mesh::Mesh &mesh, const std::string &path, double shrinkSubdomain, double shringCluster)
{
	VTK_Full output(mesh, path);
	output.store(shrinkSubdomain, shringCluster);
}

void VTK_Full::fixPoints(const mesh::Mesh &mesh, const std::string &path, double shrinkSubdomain, double shringCluster)
{
	VTK_Full output(mesh, path);
	output.store(mesh.getFixPoints(), shrinkSubdomain, shringCluster);
}

void VTK_Full::corners(const mesh::Mesh &mesh, const std::string &path, double shrinkSubdomain, double shringCluster)
{
	VTK_Full output(mesh, path);
	std::vector<std::vector<eslocal> > corners(mesh.parts());

	for (size_t p = 0; p < mesh.parts(); p++) {
		for (size_t i = 0; i < mesh.coordinates().localToCluster(p).size(); i++) {
			if (mesh.subdomainBoundaries().isCorner(mesh.coordinates().localToCluster(p)[i])) {
				corners[p].push_back(i);
			}
		}
	}

	output.store(corners, shrinkSubdomain, shringCluster);
}

void VTK_Full::dirichlet(const mesh::Mesh &mesh, const std::string &path, double shrinkSubdomain, double shringCluster)
{
	VTK_Full outputx(mesh, path + "X");
	VTK_Full outputy(mesh, path + "Y");
	VTK_Full outputz(mesh, path + "Z");

	std::vector<std::vector<eslocal> > dx(mesh.parts());
	std::vector<std::vector<eslocal> > dy(mesh.parts());
	std::vector<std::vector<eslocal> > dz(mesh.parts());

	auto &dxMap = mesh.coordinates().property(mesh::DIRICHLET_X).values();
	auto &dyMap = mesh.coordinates().property(mesh::DIRICHLET_Y).values();
	auto &dzMap = mesh.coordinates().property(mesh::DIRICHLET_Z).values();

	for (size_t p = 0; p < mesh.parts(); p++) {
		auto &l2c = mesh.coordinates().localToCluster(p);
		for (size_t i = 0; i < l2c.size(); i++) {
			if (dxMap.find(l2c[i]) != dxMap.end()) {
				dx[p].push_back(i);
			}
			if (dyMap.find(l2c[i]) != dyMap.end()) {
				dy[p].push_back(i);
			}
			if (dzMap.find(l2c[i]) != dzMap.end()) {
				dz[p].push_back(i);
			}
		}
	}

	outputx.store(dx, shrinkSubdomain, shringCluster);
	outputy.store(dy, shrinkSubdomain, shringCluster);
	outputz.store(dz, shrinkSubdomain, shringCluster);
}

void VTK_Full::averaging(const mesh::Mesh &mesh, const std::string &path, double shrinkSubdomain, double shringCluster)
{
	VTK_Full output(mesh, path);
	std::vector<std::vector<eslocal> > averaging(mesh.parts());

	for (size_t p = 0; p < mesh.parts(); p++) {
		for (size_t i = 0; i < mesh.coordinates().localToCluster(p).size(); i++) {
			if (mesh.subdomainBoundaries().isAveraging(mesh.coordinates().localToCluster(p)[i])) {
				auto &nodes = mesh.subdomainBoundaries().averaging(mesh.coordinates().localToCluster(p)[i]);
				for (size_t n = 0; n < nodes.size(); n++) {
					averaging[p].push_back(mesh.coordinates().localIndex(nodes[n], p));
				}
			}
		}
	}

	output.store(averaging, shrinkSubdomain, shringCluster);
}



