#include <vtkArrowSource.h>
#include <vtkGlyph2D.h>
#include <vtkGlyph2D.h>
#include <vtkReverseSense.h>
#include <vtkMaskPoints.h>
#include <vtkHedgeHog.h>
#include <vtkBrownianPoints.h>
#include <vtkSphereSource.h>
#include <vtkStructuredGrid.h>
#include <vtkLineSource.h>
#include <vtkLine.h>
#include <vtkTriangle.h>

#include <vtkGenericDataObjectReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkPolyData.h>
#include <vtkGenericDataObjectWriter.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkDecimatePro.h>
#include <vtkAppendFilter.h>
#include <vtkTriangleFilter.h>
#include <vtkUnstructuredGridGeometryFilter.h>
#include <vtkGeometryFilter.h>
#include <vtkUnstructuredGridGeometryFilter.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkExtractUnstructuredGrid.h>
#include <vtkContourGrid.h>
#include <vtkContourFilter.h>
#include <vtkEnSightWriter.h>
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <string>
#include <vtkPoints.h>
#include <vtkXMLMultiBlockDataWriter.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMPIController.h>

#include <vtkObjectFactory.h>
#include <vtkZLibDataCompressor.h>
#include <vtk_zlib.h>

#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

#include <iostream>
#include <fstream>
#include <vtkAppendPolyData.h>
#include <vtkMergeCells.h>

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include <vtkMultiProcessController.h>
#include <vtkMPICommunicator.h>
#include <vtkMPIController.h>

#include "../vtk.h"

using namespace espreso::output;

VTK::VTK(const Mesh &mesh, const std::string &path, double shrinkSubdomain, double shringCluster)
: Store(mesh, path, shrinkSubdomain, shringCluster)
{
	computeCenters();
}

void VTK::storeProperty(const std::string &name, const std::vector<Property> &properties, ElementType eType)
{
	const std::vector<Element*> &elements = _mesh.elements();
	const std::vector<eslocal> &partition = _mesh.getPartition();
	vtkSmartPointer<vtkPolyData> pro = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkUnstructuredGrid> prof = vtkSmartPointer<
			vtkUnstructuredGrid>::New();
	vtkSmartPointer<vtkPoints> po = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkFloatArray> sv = vtkSmartPointer<vtkFloatArray>::New();
	sv->SetName("Vectors");
	sv->SetNumberOfComponents(3);

	vtkIdType it = 0;
	int ite = 0;
	vtkSmartPointer<vtkCellArray> ver = vtkSmartPointer<vtkCellArray>::New();

	if (eType == ElementType::NODES) {
		sv->SetNumberOfTuples(_mesh.nodes().size());
		std::cout << properties.size() << std::endl;
		for (size_t i = 0; i < _mesh.nodes().size(); i++) {
			if (_mesh.nodes()[i]->settings().isSet(properties[0])) {
				//std::cout << _mesh.nodes()[i]->settings(properties[0]).back()->evaluate(i) << "\n";
				int psize = properties.size();
				Point point;
				point = _mesh.coordinates()[_mesh.nodes()[i]->node(0)];
				point = _cCenter + (point - _cCenter) * _shringCluster;
				vtkIdType pid[1];
				pid[0] = po->InsertNextPoint(point.x, point.y, point.z);
				ver->InsertNextCell(1, pid);
				double h[3];
				h[1] = 0;
				h[2] = 0;
				h[3] = 0;

				for (int p = 0; p < psize; p++) {
					h[p] = _mesh.nodes()[i]->settings(properties[p]).back()->evaluate(i);
					//std::cout<< h[p]<<std::endl;
				}
				sv->SetTuple(it, h);
				//po->InsertNextPoint(h);
				ite++;
			}
		}
	} else if (eType == ElementType::ELEMENTS) {
		sv->SetNumberOfTuples(_mesh.elements().size());
		for (size_t i = 0; i < _mesh.elements().size(); i++) {
			if (_mesh.elements()[i]->settings().isSet(properties[0])) {
				//std::cout << _mesh.nodes()[i]->settings(properties[0]).back()->evaluate(i) << "\n";
				int psize = properties.size();
				Point point;
				point = _mesh.coordinates()[_mesh.elements()[i]->node(0)];
				point = _cCenter + (point - _cCenter) * _shringCluster;
				vtkIdType pid[1];
				pid[0] = po->InsertNextPoint(point.x, point.y, point.z);
				ver->InsertNextCell(1, pid);
				double h[3];
				h[1] = 0;
				h[2] = 0;
				h[3] = 0;

				for (int p = 0; p < psize; p++) {
					h[p] = _mesh.elements()[i]->settings(properties[p]).back()->evaluate(i);
					std::cout << h[p] << std::endl;
				}
				sv->SetTuple(it, h);
				//po->InsertNextPoint(h);
				ite++;
			}
		}

	}

	pro->SetPoints(po);
	pro->SetVerts(ver);
	pro->GetPointData()->SetVectors(sv);

	vtkSmartPointer<vtkHedgeHog> hh = vtkSmartPointer<vtkHedgeHog>::New();
	vtkAppendFilter* ap = vtkAppendFilter::New();

	hh->SetInputData(pro);
	hh->SetScaleFactor(0.1);
	hh->SetOutputPointsPrecision(11);
	hh->Update();

	ap->AddInputData(hh->GetOutput());
	ap->Update();
	pro->ShallowCopy(ap->GetOutput());
	vtkAppendFilter* app = vtkAppendFilter::New();
	app->AddInputData(pro);
	app->Update();
	prof->ShallowCopy(app->GetOutput());
	//vtk
	vtkSmartPointer<vtkGenericDataObjectWriter> writervtk = vtkSmartPointer<vtkGenericDataObjectWriter>::New();
	//vtu
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> writervtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	//vtm
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> wvtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	ofstream result;
	//ensight
	vtkEnSightWriter *wcase = vtkEnSightWriter::New();
	vtkUnstructuredGrid* ugcase = vtkUnstructuredGrid::New();
	//add BlockId
	bool FCD = false;
	if (prof->GetCellData()) {
		if (prof->GetCellData()->GetArray("BlockId")) {
			FCD = true;
		}
	}
	if (FCD == false) {
		vtkIntArray *bids = vtkIntArray::New();
		bids->SetName("BlockId");
		for (int i = 0; i < pro->GetNumberOfCells(); i++) {
			bids->InsertNextValue(1);
		}
		prof->GetCellData()->SetScalars(bids);
	}
	int blockids[2];
	blockids[0] = 1;
	blockids[1] = 0;
	stringstream ss;
	//Writers
	switch (config::output::OUTPUT_FORMAT) {
	case config::output::OUTPUT_FORMATAlternatives::VTK_LEGACY_FORMAT:
		std::cout << "LEGACY\n";
		ss << name << config::env::MPIrank << ".vtk";
		writervtk->SetFileName(ss.str().c_str());
		writervtk->SetInputData(pro);
		writervtk->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_BINARY_FORMAT:
		std::cout << "BINARY\n";
		ss << name << config::env::MPIrank << ".vtu";
		writervtu->SetFileName(ss.str().c_str());
		writervtu->SetInputData(prof);
		writervtu->SetDataModeToBinary();
		writervtu->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_MULTIBLOCK_FORMAT:
		std::cout << "MULTIBLOCK\n";
		ss << name << config::env::MPIrank << ".vtu";
		wvtu->SetFileName(ss.str().c_str());
		wvtu->SetInputData(prof);
		wvtu->SetDataModeToBinary();
		wvtu->Write();

		MPI_Barrier(MPI_COMM_WORLD);
		int size = config::env::MPIsize;
		if (config::env::MPIrank == 0) {
			stringstream sss;
			sss << name << ".vtm";
			result.open(sss.str().c_str());
			result << "<?xml version=\"1.0\"?>\n";
			if (!config::output::OUTPUT_COMPRESSION) {
				result << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\">\n";
				std::cout << "Compression: " << config::output::OUTPUT_COMPRESSION << "\n";
			} else {
				result << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\" compressor=\"vtkZLibDataCompressor\">\n";
				std::cout << "Compression: " << config::output::OUTPUT_COMPRESSION << "\n";
			}

			result << " <vtkMultiBlockDataSet>\n";
			for (int i = 0; i < size; i++) {
				result << "  <DataSet index=\"" << i << "\" file=\"" << name << i << ".vtu\">\n  </DataSet>\n";
			}
			result << " </vtkMultiBlockDataSet>\n";
			result << "</VTKFile>\n";
			result.close();
		}
		break;

	case config::output::OUTPUT_FORMATAlternatives::ENSIGHT_FORMAT: {
		vtkMPIController* controller = vtkMPIController::New();
		std::cout << "ENSIGHT\n";
		if (config::env::MPIrank != 0) {
			controller->Send(prof, 0, 1111 + config::env::MPIrank);
		}
		//write ensight format
		ss << name << ".case";
		wcase->SetFileName(ss.str().c_str());
		wcase->SetNumberOfBlocks(1);
		wcase->SetBlockIDs(blockids);
		wcase->SetTimeStep(0);
		if (config::env::MPIrank == 0) {
			vtkAppendFilter* app = vtkAppendFilter::New();
			app->AddInputData(prof);

			for (int i = 1; i < config::env::MPIsize; i++) {
				vtkUnstructuredGrid* h = vtkUnstructuredGrid::New();
				controller->Receive(h, i, 1111 + i);
				app->AddInputData(h);
				std::cout << "prijato\n";
			}
			app->Update();
			ugcase->ShallowCopy(app->GetOutput());
			wcase->SetInputData(ugcase);
			wcase->Write();
			wcase->WriteCaseFile(1);
		}
		break;
	}
	}

	ESINFO(GLOBAL_ERROR) << "Implement store property";
}

void VTK::storeValues(const std::string &name, size_t dimension, const std::vector<std::vector<double> > &values, ElementType eType)
{
	const std::vector<Element*> &elements = _mesh.elements();
	const std::vector<eslocal> &_partPtrs = _mesh.getPartition();

	vtkUnstructuredGrid* VTKGrid = vtkUnstructuredGrid::New();

	size_t n_nodsClust = 0;
	for (size_t iEl = 0; iEl < elements.size(); iEl++) {
		n_nodsClust += elements[iEl]->nodes();
	}
	size_t cnt = 0, n_points = 0;
	for (size_t d = 0; d < _mesh.parts(); d++) {
		n_points += _mesh.coordinates().localSize(d);
	}

	//Points
	int counter = 0;
	vtkPoints* points = vtkPoints::New();
	for (size_t d = 0; d < _mesh.parts(); d++) {
		for (size_t i = 0; i < _mesh.coordinates().localSize(d); i++) {
			Point xyz = _mesh.coordinates().get(i, d);
			xyz = _sCenters[d] + (xyz - _sCenters[d]) * _shrinkSubdomain;
			points->InsertNextPoint(xyz.x, xyz.y, xyz.z);
			counter++;
		}
	}
	VTKGrid->SetPoints(points);

	VTKGrid->Allocate(static_cast<vtkIdType>(n_nodsClust));
	vtkIdType tmp[100]; //max number of  node

	//Cells
	size_t i = 0;
	cnt = 0;
	for (size_t part = 0; part + 1 < _partPtrs.size(); part++) {
		for (eslocal ii = 0; ii < _partPtrs[part + 1] - _partPtrs[part]; ii++) {
			for (size_t j = 0; j < _mesh.elements()[i]->nodes(); j++) {
				tmp[j] = _mesh.coordinates().localIndex(elements[i]->node(j), part) + cnt;
			}
			int code = _mesh.elements()[i]->vtkCode();
			VTKGrid->InsertNextCell(code, elements[i]->nodes(), &tmp[0]);
			i++;
		}
		cnt += _mesh.coordinates().localSize(part);
	}

	float *decomposition_array = new float[_mesh.elements().size()];

	//Decomposition
	counter = 0;
	for (size_t part = 0; part + 1 < _partPtrs.size(); part++) {
		for (eslocal i = 0; i < _partPtrs[part + 1] - _partPtrs[part]; i++) {
			float part_redefine = part;
			decomposition_array[counter] = part_redefine;
			counter++;
		}
	}

	vtkNew<vtkFloatArray> decomposition;
	decomposition->SetName("decomposition");
	decomposition->SetNumberOfComponents(1);
	decomposition->SetArray(decomposition_array, static_cast<vtkIdType>(elements.size()), 1);
	VTKGrid->GetCellData()->AddArray(decomposition.GetPointer());

	//Values
	if (eType == ElementType::NODES) {
		int mycounter = 0;
		for (size_t i = 0; i < values.size(); i++) {
			mycounter += values[i].size();
		}

		const unsigned int dofs = values[0].size() / _mesh.coordinates().localSize(0);
		double value_array[mycounter];
		counter = 0;

		for (size_t i = 0; i < values.size(); i++) {
			for (size_t j = 0; j < (values[i].size() / dofs); j++) {
				for (int k = 0; k < dofs; k++) {
					value_array[dofs * counter + k] = values[i][dofs * j + k];
				}
				counter++;
			}
		}

		vtkNew<vtkDoubleArray> value;
		value->SetName("Values");
		value->SetNumberOfComponents(dofs);
		value->SetNumberOfTuples(static_cast<vtkIdType>(counter));
		VTKGrid->GetPointData()->AddArray(value.GetPointer());

		double* valueData = value_array;
		vtkIdType numTuples = value->GetNumberOfTuples();
		for (vtkIdType i = 0, counter = 0; i < numTuples; i++, counter++) {
			double values[dofs];
			for (int k = 0; k < dofs; k++) {
				values[k] = {valueData[i*dofs+k]};
			}
			value->SetTypedTuple(counter, values);
		}
	} else if (eType == ElementType::ELEMENTS) {
		float *value_array = new float[elements.size()];

		counter = 0;
		for (size_t part = 0; part + 1 < _partPtrs.size(); part++) {
			for (eslocal i = 0; i < _partPtrs[part + 1] - _partPtrs[part]; i++) {
				float part_redefine = part;
				value_array[counter] = part_redefine;
				counter++;
			}
		}

		vtkNew<vtkFloatArray> value;
		value->SetName("Values");
		value->SetNumberOfComponents(1);
		value->SetArray(decomposition_array, static_cast<vtkIdType>(elements.size()), 1);
		VTKGrid->GetCellData()->AddArray(value.GetPointer());

	}
	//surface and decimation

	vtkSmartPointer<vtkDecimatePro> decimate = vtkSmartPointer<vtkDecimatePro>::New();
	vtkAppendFilter* ap = vtkAppendFilter::New();
	vtkUnstructuredGrid* decimated = vtkUnstructuredGrid::New();
	vtkGeometryFilter* gf = vtkGeometryFilter::New();
	vtkDataSetSurfaceFilter* dssf = vtkDataSetSurfaceFilter::New();
	vtkTriangleFilter* tf = vtkTriangleFilter::New();

	if (config::output::OUTPUT_DECIMATION != 0) {
		gf->SetInputData(VTKGrid);
		gf->Update();

		dssf->SetInputData(gf->GetOutput());
		dssf->Update();

		tf->SetInputData(dssf->GetOutput());
		tf->Update();

		decimate->SetInputData(tf->GetOutput());
		decimate->SetTargetReduction(config::output::OUTPUT_DECIMATION);
		decimate->Update();

		ap->AddInputData(decimate->GetOutput());
		ap->Update();

		decimated->ShallowCopy(ap->GetOutput());
	}

	std::stringstream ss;
	//Compresor
	vtkZLibDataCompressor *myZlibCompressor = vtkZLibDataCompressor::New();
	myZlibCompressor->SetCompressionLevel(9);
	//vtk
	vtkSmartPointer<vtkGenericDataObjectWriter> writervtk = vtkSmartPointer<vtkGenericDataObjectWriter>::New();
	//vtu
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> writervtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	//vtm
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> wvtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	ofstream result;
	//ensight
	vtkEnSightWriter *wcase = vtkEnSightWriter::New();
	vtkUnstructuredGrid* ugcase = vtkUnstructuredGrid::New();
	//add BlockId
	bool FCD = false;
	if (VTKGrid->GetCellData()) {
		if (VTKGrid->GetCellData()->GetArray("BlockId")) {
			FCD = true;
		}
	}
	if (FCD == false) {
		vtkIntArray *bids = vtkIntArray::New();
		bids->SetName("BlockId");
		for (int i = 0; i < VTKGrid->GetNumberOfCells(); i++) {
			bids->InsertNextValue(1);
		}
		VTKGrid->GetCellData()->SetScalars(bids);
	}
	int blockids[2];
	blockids[0] = 1;
	blockids[1] = 0;

	//Writers
	switch (config::output::OUTPUT_FORMAT) {
	case config::output::OUTPUT_FORMATAlternatives::VTK_LEGACY_FORMAT:
		std::cout << "LEGACY\n";
		ss << name << config::env::MPIrank << ".vtk";
		writervtk->SetFileName(ss.str().c_str());
		if (config::output::OUTPUT_DECIMATION == 0) {
			writervtk->SetInputData(VTKGrid);
		} else {
			writervtk->SetInputData(decimated);
		}
		writervtk->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_BINARY_FORMAT:
		std::cout << "BINARY\n";
		ss << name << config::env::MPIrank << ".vtu";
		writervtu->SetFileName(ss.str().c_str());
		if (config::output::OUTPUT_DECIMATION == 0) {
			writervtu->SetInputData(VTKGrid);
		} else {
			writervtu->SetInputData(decimated);
		}
		writervtu->SetDataModeToBinary();
		writervtu->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_MULTIBLOCK_FORMAT:
		std::cout << "MULTIBLOCK\n";
		ss << name << config::env::MPIrank << ".vtu";
		wvtu->SetFileName(ss.str().c_str());
		if (config::output::OUTPUT_DECIMATION == 0) {
			wvtu->SetInputData(VTKGrid);
		} else {
			wvtu->SetInputData(decimated);
		}
		wvtu->SetDataModeToBinary();
		wvtu->Write();

		MPI_Barrier(MPI_COMM_WORLD);
		int size = config::env::MPIsize;
		if (config::env::MPIrank == 0) {
			stringstream sss;
			sss << name << ".vtm";
			result.open(sss.str().c_str());
			result << "<?xml version=\"1.0\"?>\n";
			if (!config::output::OUTPUT_COMPRESSION) {
				result << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\">\n";
				std::cout << "Compression: " << config::output::OUTPUT_COMPRESSION << "\n";
			} else {
				result << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\" compressor=\"vtkZLibDataCompressor\">\n";
				std::cout << "Compression: " << config::output::OUTPUT_COMPRESSION << "\n";
			}

			result << " <vtkMultiBlockDataSet>\n";
			for (int i = 0; i < size; i++) {
				result << "  <DataSet index=\"" << i << "\" file=\"" << name << i << ".vtu\">\n  </DataSet>\n";
			}
			result << " </vtkMultiBlockDataSet>\n";
			result << "</VTKFile>\n";
			result.close();
		}
		break;

	case config::output::OUTPUT_FORMATAlternatives::ENSIGHT_FORMAT: {
		std::cout << "ENSIGHT\n";
		vtkMPIController* controller;
		controller = vtkMPIController::New();
		controller->Initialize();
		if (config::env::MPIrank != 0) {
			if (config::output::OUTPUT_DECIMATION == 0) {
				controller->Send(VTKGrid, 0, 1111 + config::env::MPIrank);
			} else {
				controller->Send(decimated, 0, 1111 + config::env::MPIrank);
			}
		}

		//write ensight format
		ss << name << ".case";
		wcase->SetFileName(ss.str().c_str());
		wcase->SetNumberOfBlocks(1);
		wcase->SetBlockIDs(blockids);
		wcase->SetTimeStep(0);
		if (config::env::MPIrank == 0) {
			vtkAppendFilter* app = vtkAppendFilter::New();
			if (config::output::OUTPUT_DECIMATION == 0) {
				app->AddInputData(VTKGrid);
			} else {
				app->AddInputData(decimated);
			}
			for (int i = 1; i < config::env::MPIsize; i++) {
				vtkUnstructuredGrid* h = vtkUnstructuredGrid::New();
				controller->Receive(h, i, 1111 + i);
				app->AddInputData(h);
				std::cout << "prijato\n";
			}
			app->Update();
			ugcase->ShallowCopy(app->GetOutput());
			wcase->SetInputData(ugcase);
			wcase->Write();
			wcase->WriteCaseFile(1);
		}
	}
		break;
	}
}

void VTK::mesh(const Mesh &mesh, const std::string &path, double shrinkSubdomain, double shrinkCluster)
{
	ESINFO(GLOBAL_ERROR) << "Implement mesh";
	const std::vector<Element*> &elements = mesh.elements();
	const std::vector<eslocal> &_partPtrs = mesh.getPartition();

	vtkUnstructuredGrid* m = vtkUnstructuredGrid::New();

	size_t n_nodsClust = 0;
	for (size_t iEl = 0; iEl < elements.size(); iEl++) {
		n_nodsClust += elements[iEl]->nodes();
	}
	size_t cnt = 0, n_points = 0;
	for (size_t d = 0; d < mesh.parts(); d++) {
		n_points += mesh.coordinates().localSize(d);
	}

	double shrinking = 0.90;
	const Coordinates &_coordinates = mesh.coordinates();
	double *coord_xyz = new double[n_points * 3];

	int counter = 0;
	for (size_t d = 0; d < mesh.parts(); d++) {
		Point center;
		for (size_t c = 0; c < _coordinates.localSize(d); c++) {
			center += _coordinates.get(c, d);
		}
		center /= _coordinates.localSize(d);

		for (size_t i = 0; i < _coordinates.localSize(d); i++) {
			Point xyz = _coordinates.get(i, d);
			xyz = center + (xyz - center) * shrinking;
			coord_xyz[3 * counter + 0] = xyz.x;
			coord_xyz[3 * counter + 1] = xyz.y;
			coord_xyz[3 * counter + 2] = xyz.z;
			counter++;
		}
	}

	vtkNew<vtkDoubleArray> pointArray;
	pointArray->SetNumberOfComponents(3);

	size_t numpoints = n_points * 3;
	pointArray->SetArray(coord_xyz, numpoints, 1);
	vtkNew<vtkPoints> points;
	points->SetData(pointArray.GetPointer());
	m->SetPoints(points.GetPointer());

	m->Allocate(static_cast<vtkIdType>(n_nodsClust));
	vtkIdType tmp[100]; //max number of  node

	size_t i = 0;
	cnt = 0;
	for (size_t part = 0; part + 1 < _partPtrs.size(); part++) {
		for (eslocal ii = 0; ii < _partPtrs[part + 1] - _partPtrs[part]; ii++) {
			for (size_t j = 0; j < elements[i]->nodes(); j++) {
				tmp[j] = elements[i]->node(j) + cnt;
			}
			m->InsertNextCell(elements[i]->vtkCode(), elements[i]->nodes(), &tmp[0]);
			i++;
		}
		cnt += _coordinates.localSize(part);
	}
	vtkSmartPointer<vtkGenericDataObjectWriter> mw = vtkSmartPointer<vtkGenericDataObjectWriter>::New();
	mw->SetFileName("mesh.vtk");
	mw->SetInputData(m);
	mw->Write();

	//add BlockId
	bool FCD = false;
	if (m->GetCellData()) {
		if (m->GetCellData()->GetArray("BlockId")) {
			FCD = true;
		}
	}
	if (FCD == false) {
		vtkIntArray *bids = vtkIntArray::New();
		bids->SetName("BlockId");
		for (i = 0; i < m->GetNumberOfCells(); i++) {
			bids->InsertNextValue(1);
		}
		m->GetCellData()->SetScalars(bids);
	}
	int blockids[2];
	blockids[0] = 1;
	blockids[1] = 0;

	//MultiProces controler	
	vtkUnstructuredGrid* ugcase = vtkUnstructuredGrid::New();
	vtkMPIController* controller = vtkMPIController::New();

	int rank = config::env::MPIrank;
	if (rank != 0) {
		controller->Send(m, 0, 1111 + rank);
	}

	//vtk
	vtkSmartPointer<vtkGenericDataObjectWriter> writervtk = vtkSmartPointer<vtkGenericDataObjectWriter>::New();
	std::stringstream ss;

	//vtu
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> writervtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	std::stringstream sss;

	//vtm
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> wvtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	std::stringstream ssss;
	ofstream result;
	//ensight
	vtkEnSightWriter *wcase = vtkEnSightWriter::New();

	switch (config::output::OUTPUT_FORMAT) {
	case config::output::OUTPUT_FORMATAlternatives::VTK_LEGACY_FORMAT:
		std::cout << "LEGACY\n";
		ss << path << ".vtk";
		writervtk->SetFileName(ss.str().c_str());
		writervtk->SetInputData(m);
		writervtk->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_BINARY_FORMAT:
		std::cout << "BINARY\n";
		sss << path << ".vtu";
		writervtu->SetFileName(sss.str().c_str());
		writervtu->SetInputData(m);
		writervtu->SetDataModeToBinary();
		writervtu->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_MULTIBLOCK_FORMAT:
		std::cout << "MULTIBLOCK\n";
		ssss << path << ".vtu";
		wvtu->SetFileName(ssss.str().c_str());
		wvtu->SetInputData(m);
		wvtu->SetDataModeToBinary();
		wvtu->Write();

		MPI_Barrier(MPI_COMM_WORLD);
		int size = config::env::MPIsize;
		if (config::env::MPIrank == 0) {
			result.open("mesh_result.vtm");
			result << "<?xml version=\"1.0\"?>\n";
			result << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\" compressor=\"vtkZLibDataCompressor\">\n";
			std::cout << "Compression: " << config::output::OUTPUT_COMPRESSION << "\n";
			result << " <vtkMultiBlockDataSet>\n";
			for (int i = 0; i < size; i++) {
				result << "  <DataSet index=\"" << i << "\" file=\"mesh" << i << ".vtu\">\n  </DataSet>\n";
			}
			result << " </vtkMultiBlockDataSet>\n";
			result << "</VTKFile>\n";
			result.close();
		}
		break;

	case config::output::OUTPUT_FORMATAlternatives::ENSIGHT_FORMAT:
		std::cout << "ENSIGHT\n";

		//write ensight format
		wcase->SetFileName("result.case");
		wcase->SetNumberOfBlocks(1);
		wcase->SetBlockIDs(blockids);
		wcase->SetTimeStep(0);
		if (config::env::MPIrank == 0) {
			vtkAppendFilter* app = vtkAppendFilter::New();
			app->AddInputData(m);
			for (int i = 1; i < config::env::MPIsize; i++) {
				vtkUnstructuredGrid* h = vtkUnstructuredGrid::New();
				controller->Receive(h, i, 1111 + i);
				app->AddInputData(h);
				std::cout << "prijato\n";
			}
			app->Update();
			ugcase->ShallowCopy(app->GetOutput());
			wcase->SetInputData(ugcase);
			wcase->Write();
			wcase->WriteCaseFile(1);
		}
		break;
	}

	std::cout << "SAVE GENERIC VTK DATA\n";
}

void VTK::properties(const Mesh &mesh, const std::string &path,
		std::vector<Property> properties, double shrinkSubdomain,
		double shrinkCluster) {
	ESINFO(GLOBAL_ERROR) << "Implement properties";
	std::cout << path << "\n";
	const std::vector<Element*> &elements = mesh.elements();
	const std::vector<eslocal> &partition = mesh.getPartition();
	vtkSmartPointer<vtkUnstructuredGrid> pro = vtkSmartPointer<
			vtkUnstructuredGrid>::New();
	//pro->SetDimensions(elements.size(),elements.size(),0);
	vtkSmartPointer<vtkPoints> po = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkFloatArray> sv = vtkSmartPointer<vtkFloatArray>::New();

	sv->SetName("Vectors");
	sv->SetNumberOfComponents(3);
	sv->SetNumberOfTuples(elements.size());
	vtkIdType it = 0;

	for (size_t p = 0; p < mesh.parts(); p++) {
		for (size_t e = partition[p]; e < partition[p + 1]; e++) {
			// TODO: first evaluate on nodes and then compute average
//			Point mid;
//			for (size_t i = 0; i < elements[e]->nodes(); i++) {
//				mid += mesh.coordinates().get(elements[e]->node(i), p);
//			}
//			mid /= elements[e]->nodes();
//
//			po->InsertNextPoint(mid.x,mid.y,mid.z);
//
//			const std::vector<Evaluator*> &ux = elements[e]->settings(properties[0]);
//			const std::vector<Evaluator*> &uy = elements[e]->settings(properties[1]);
//
//			double h[3];
//			h[0]=ux.back()->evaluate(mid.x,mid.y,mid.z)/elements[e]->nodes();
//			h[1]=uy.back()->evaluate(mid.x,mid.y,mid.z)/elements[e]->nodes();
//			h[2]=0;
//			sv->SetTuple(it,h);
//			it++;
		}
	}

	pro->SetPoints(po);
	pro->GetPointData()->SetVectors(sv);

	vtkSmartPointer<vtkHedgeHog> hh = vtkSmartPointer<vtkHedgeHog>::New();
	hh->SetInputData(pro);
	hh->SetScaleFactor(0.1);
	hh->SetOutputPointsPrecision(11);
	hh->Update();

	vtkAppendFilter* ap = vtkAppendFilter::New();
	ap->AddInputData(hh->GetOutput());
	ap->Update();
	pro->ShallowCopy(ap->GetOutput());

	//add BlockId
	bool FCD = false;
	if (pro->GetCellData()) {
		if (pro->GetCellData()->GetArray("BlockId")) {
			FCD = true;
		}
	}
	if (FCD == false) {
		vtkIntArray *bids = vtkIntArray::New();
		bids->SetName("BlockId");
		for (int i = 0; i < pro->GetNumberOfCells(); i++) {
			bids->InsertNextValue(1);
		}
		pro->GetCellData()->SetScalars(bids);
	}
	int blockids[2];
	blockids[0] = 1;
	blockids[1] = 0;

	//MultiProces controler	
	vtkUnstructuredGrid* ugcase = vtkUnstructuredGrid::New();
	vtkMPIController* controller = vtkMPIController::New();

	int rank = config::env::MPIrank;
	if (rank != 0) {
		controller->Send(pro, 0, 1111 + rank);
	}

	//vtk
	vtkSmartPointer<vtkGenericDataObjectWriter> writervtk = vtkSmartPointer<vtkGenericDataObjectWriter>::New();
	std::stringstream ss;

	//vtu
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> writervtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	std::stringstream sss;

	//vtm
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> wvtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	std::stringstream ssss;
	ofstream result;
	//ensight
	vtkEnSightWriter *wcase = vtkEnSightWriter::New();

	switch (config::output::OUTPUT_FORMAT) {
	case config::output::OUTPUT_FORMATAlternatives::VTK_LEGACY_FORMAT:
		std::cout << "LEGACY\n";
		ss << path << config::env::MPIrank << ".vtk";
		writervtk->SetFileName(ss.str().c_str());
		writervtk->SetInputData(pro);
		writervtk->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_BINARY_FORMAT:
		std::cout << "BINARY\n";
		sss << path << config::env::MPIrank << ".vtu";
		writervtu->SetFileName(sss.str().c_str());
		writervtu->SetInputData(pro);
		writervtu->SetDataModeToBinary();
		writervtu->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_MULTIBLOCK_FORMAT:
		std::cout << "MULTIBLOCK\n";
		ssss << path << config::env::MPIrank << ".vtu";
		wvtu->SetFileName(ssss.str().c_str());
		wvtu->SetInputData(pro);
		wvtu->SetDataModeToBinary();
		wvtu->Write();

		MPI_Barrier(MPI_COMM_WORLD);
		int size = config::env::MPIsize;
		if (config::env::MPIrank == 0) {
			result.open("meshP_result.vtm");
			result << "<?xml version=\"1.0\"?>\n";
			result << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\" compressor=\"vtkZLibDataCompressor\">\n";
			std::cout << "Compression: " << config::output::OUTPUT_COMPRESSION << "\n";
			result << " <vtkMultiBlockDataSet>\n";
			for (int i = 0; i < size; i++) {
				result << "  <DataSet index=\"" << i
						<< "\" file=\"meshTranslationMotion" << i
						<< ".vtu\">\n  </DataSet>\n";
			}
			result << " </vtkMultiBlockDataSet>\n";
			result << "</VTKFile>\n";
			result.close();
		}
		break;

	case config::output::OUTPUT_FORMATAlternatives::ENSIGHT_FORMAT:
		std::cout << "ENSIGHT\n";

		//write ensight format
		wcase->SetFileName("meshP_result.case");
		wcase->SetNumberOfBlocks(1);
		wcase->SetBlockIDs(blockids);
		wcase->SetTimeStep(0);
		if (config::env::MPIrank == 0) {
			vtkAppendFilter* app = vtkAppendFilter::New();
			app->AddInputData(pro);
			for (int i = 1; i < config::env::MPIsize; i++) {
				vtkUnstructuredGrid* h = vtkUnstructuredGrid::New();
				controller->Receive(h, i, 1111 + i);
				app->AddInputData(h);
				std::cout << "prijato\n";
			}
			app->Update();
			ugcase->ShallowCopy(app->GetOutput());
			wcase->SetInputData(ugcase);
			wcase->Write();
			wcase->WriteCaseFile(1);
		}
		break;
	}

	std::cout << "Properties saved\n";
}

void VTK::fixPoints(const Mesh &mesh, const std::string &path, double shrinkSubdomain, double shringCluster)
{
	vtkUnstructuredGrid* fp = vtkUnstructuredGrid::New();
	std::vector<std::vector<eslocal> > fixPoints(mesh.parts());
	const Coordinates &_coordinates = mesh.coordinates();
	const std::vector<eslocal> &_partPtrs = mesh.getPartition();
	const std::vector<Element*> &elements = mesh.elements();
	size_t parts = _coordinates.parts();
	double shrinking = 0.90;

	//points
	for (size_t p = 0; p < mesh.parts(); p++) {
		for (size_t i = 0; i < mesh.fixPoints(p).size(); i++) {
			fixPoints[p].push_back(mesh.fixPoints(p)[i]->node(0));
		}
	}

	size_t n_nodsClust = 0;
	for (size_t iEl = 0; iEl < elements.size(); iEl++) {
		n_nodsClust += elements[iEl]->nodes();
	}
	double *coord_xyz = new double[mesh.parts() * fixPoints.size() * 3];

	int counter = 0;
	for (size_t d = 0; d < mesh.parts(); d++) {
		Point center;
		for (size_t c = 0; c < fixPoints[d].size(); c++) {
			center += _coordinates.get(fixPoints[d][c], d);

		}
		center /= fixPoints[d].size();
		//std::cout<<center<<std::endl;
		for (size_t i = 0; i < fixPoints[d].size(); i++) {
			Point xyz = _coordinates.get(fixPoints[d][i], d);
			xyz = center + (xyz - center) * shrinking;
			coord_xyz[3 * counter + 0] = xyz.x;
			coord_xyz[3 * counter + 1] = xyz.y;
			coord_xyz[3 * counter + 2] = xyz.z;
			counter++;
		}
	}
	//std::cout<<std::endl<<mesh.parts()*fixPoints.size()<<std::endl;
	vtkNew<vtkDoubleArray> pointArray;
	pointArray->SetNumberOfComponents(3);
	size_t numpoints = mesh.parts() * fixPoints.size() * 3;
	pointArray->SetArray(coord_xyz, numpoints, 1);
	vtkNew<vtkPoints> points;
	points->SetData(pointArray.GetPointer());
	fp->SetPoints(points.GetPointer());
	fp->Allocate(static_cast<vtkIdType>(n_nodsClust));

	//cells
	size_t offset = 0;
	size_t cnt = 0;
	size_t i = 0;
	vtkIdType tmp[100]; //max number of  nodevtkIdType

	for (size_t p = 0; p < fixPoints.size(); p++) {
		for (size_t j = 0; j < fixPoints[i].size(); j++) {
			tmp[j] = cnt + j;
		}
		fp->InsertNextCell(2, fixPoints[p].size(), &tmp[0]);

		i++;
		cnt += fixPoints[p].size();
	}
	//decomposition
	float *decomposition_array = new float[fixPoints.size()];

	for (size_t p = 0; p < fixPoints.size(); p++) {
		decomposition_array[p] = p;

	}

	vtkNew<vtkFloatArray> decomposition;
	decomposition->SetName("decomposition");
	decomposition->SetNumberOfComponents(1);
	decomposition->SetArray(decomposition_array, static_cast<vtkIdType>(fixPoints.size()), 1);
	fp->GetCellData()->AddArray(decomposition.GetPointer());

	//add BlockId
	bool FCD = false;
	if (fp->GetCellData()) {
		if (fp->GetCellData()->GetArray("BlockId")) {
			FCD = true;
		}
	}
	if (FCD == false) {
		vtkIntArray *bids = vtkIntArray::New();
		bids->SetName("BlockId");
		for (i = 0; i < fp->GetNumberOfCells(); i++) {
			bids->InsertNextValue(1);
		}
		fp->GetCellData()->SetScalars(bids);
	}
	int blockids[2];
	blockids[0] = 1;
	blockids[1] = 0;

	//MultiProces controler	
	vtkUnstructuredGrid* ugcase = vtkUnstructuredGrid::New();
	vtkMPIController* controller = vtkMPIController::New();

	int rank = config::env::MPIrank;
	if (rank != 0) {
		controller->Send(fp, 0, 1111 + rank);
	}

	//vtk
	vtkSmartPointer<vtkGenericDataObjectWriter> writervtk = vtkSmartPointer<vtkGenericDataObjectWriter>::New();
	std::stringstream ss;

	//vtu
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> writervtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	std::stringstream sss;

	//vtm
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> wvtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	std::stringstream ssss;
	ofstream result;
	//ensight
	vtkEnSightWriter *wcase = vtkEnSightWriter::New();

	switch (config::output::OUTPUT_FORMAT) {
	case config::output::OUTPUT_FORMATAlternatives::VTK_LEGACY_FORMAT:
		std::cout << "LEGACY\n";
		ss << path << config::env::MPIrank << ".vtk";
		writervtk->SetFileName(ss.str().c_str());
		writervtk->SetInputData(fp);
		writervtk->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_BINARY_FORMAT:
		std::cout << "BINARY\n";
		sss << path << config::env::MPIrank << ".vtu";
		writervtu->SetFileName(sss.str().c_str());
		writervtu->SetInputData(fp);
		writervtu->SetDataModeToBinary();
		writervtu->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_MULTIBLOCK_FORMAT:
		std::cout << "MULTIBLOCK\n";
		ssss << path << config::env::MPIrank << ".vtu";
		wvtu->SetFileName(ssss.str().c_str());
		wvtu->SetInputData(fp);
		wvtu->SetDataModeToBinary();
		wvtu->Write();

		MPI_Barrier(MPI_COMM_WORLD);
		int size = config::env::MPIsize;
		if (config::env::MPIrank == 0) {
			result.open("meshFP_result.vtm");
			result << "<?xml version=\"1.0\"?>\n";
			result << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\" compressor=\"vtkZLibDataCompressor\">\n";
			std::cout << "Compression: " << config::output::OUTPUT_COMPRESSION << "\n";
			result << " <vtkMultiBlockDataSet>\n";
			for (int i = 0; i < size; i++) {
				result << "  <DataSet index=\"" << i
						<< "\" file=\"meshFixPoints" << i
						<< ".vtu\">\n  </DataSet>\n";
			}
			result << " </vtkMultiBlockDataSet>\n";
			result << "</VTKFile>\n";
			result.close();
		}
		break;

	case config::output::OUTPUT_FORMATAlternatives::ENSIGHT_FORMAT:
		std::cout << "ENSIGHT\n";

		//write ensight format
		wcase->SetFileName("meshFP_result.case");
		wcase->SetNumberOfBlocks(1);
		wcase->SetBlockIDs(blockids);
		wcase->SetTimeStep(0);
		if (config::env::MPIrank == 0) {
			vtkAppendFilter* app = vtkAppendFilter::New();
			app->AddInputData(fp);
			for (int i = 1; i < config::env::MPIsize; i++) {
				vtkUnstructuredGrid* h = vtkUnstructuredGrid::New();
				controller->Receive(h, i, 1111 + i);
				app->AddInputData(h);
				std::cout << "prijato\n";
			}
			app->Update();
			ugcase->ShallowCopy(app->GetOutput());
			wcase->SetInputData(ugcase);
			wcase->Write();
			wcase->WriteCaseFile(1);
		}
		break;
	}

	std::cout << "SAVE FIX POINTS TO VTK\n";
}

void VTK::corners(const Mesh &mesh, const std::string &path, double shrinkSubdomain, double shringCluster)
{
	vtkUnstructuredGrid* c = vtkUnstructuredGrid::New();
	const Coordinates &_coordinates = mesh.coordinates();
	const std::vector<eslocal> &_partPtrs = mesh.getPartition();
	const std::vector<Element*> &elements = mesh.elements();
	size_t parts = _coordinates.parts();
	double shrinking = 0.90;
	std::vector<std::vector<eslocal> > corners(mesh.parts());

	for (size_t i = 0; i < mesh.corners().size(); i++) {
		for (size_t d = 0; d < mesh.corners()[i]->domains().size(); d++) {
			size_t p = mesh.corners()[i]->domains()[d];
			corners[p].push_back(mesh.corners()[i]->node(0));
		}
	}

	size_t n_nodsClust = 0;
	for (size_t iEl = 0; iEl < elements.size(); iEl++) {
		n_nodsClust += elements[iEl]->nodes();
	}
	double *coord_xyz = new double[mesh.parts() * corners.size() * 3];

	int counter = 0;
	for (size_t d = 0; d < mesh.parts(); d++) {
		Point center;
		for (size_t c = 0; c < corners[d].size(); c++) {
			center += mesh.coordinates()[corners[d][c]];
		}
		center /= corners[d].size();
		//std::cout<<center<<std::endl;
		for (size_t i = 0; i < corners[d].size(); i++) {
			Point xyz = mesh.coordinates()[corners[d][i]];
			xyz = center + (xyz - center) * shrinking;

			coord_xyz[3 * counter + 0] = xyz.x;
			coord_xyz[3 * counter + 1] = xyz.y;
			coord_xyz[3 * counter + 2] = xyz.z;
			counter++;
		}
	}
	//std::cout<<std::endl<<mesh.parts()*fixPoints.size()<<std::endl;
	vtkNew<vtkDoubleArray> pointArray;
	pointArray->SetNumberOfComponents(3);
	size_t numpoints = mesh.parts() * corners.size() * 3;
	pointArray->SetArray(coord_xyz, numpoints, 1);
	vtkNew<vtkPoints> points;
	points->SetData(pointArray.GetPointer());
	c->SetPoints(points.GetPointer());
	c->Allocate(static_cast<vtkIdType>(n_nodsClust));

	//cells
	size_t offset = 0;
	size_t cnt = 0;
	size_t i = 0;
	vtkIdType tmp[100]; //max number of  nodevtkIdType

	for (size_t p = 0; p < corners.size(); p++) {
		for (size_t j = 0; j < corners[i].size(); j++) {
			tmp[j] = cnt + j;
		}
		c->InsertNextCell(2, corners[p].size(), &tmp[0]);

		i++;
		cnt += corners[p].size();
	}
	//decomposition
	float *decomposition_array = new float[corners.size()];

	for (size_t p = 0; p < corners.size(); p++) {
		decomposition_array[p] = p;

	}

	vtkNew<vtkFloatArray> decomposition;
	decomposition->SetName("decomposition");
	decomposition->SetNumberOfComponents(1);
	decomposition->SetArray(decomposition_array, static_cast<vtkIdType>(corners.size()), 1);
	c->GetCellData()->AddArray(decomposition.GetPointer());

	//add BlockId
	bool FCD = false;
	if (c->GetCellData()) {
		if (c->GetCellData()->GetArray("BlockId")) {
			FCD = true;
		}
	}
	if (FCD == false) {
		vtkIntArray *bids = vtkIntArray::New();
		bids->SetName("BlockId");
		for (i = 0; i < c->GetNumberOfCells(); i++) {
			bids->InsertNextValue(1);
		}
		c->GetCellData()->SetScalars(bids);
	}
	int blockids[2];
	blockids[0] = 1;
	blockids[1] = 0;

	//MultiProces controler	
	vtkUnstructuredGrid* ugcase = vtkUnstructuredGrid::New();
	vtkMPIController* controller = vtkMPIController::New();

	int rank = config::env::MPIrank;
	if (rank != 0) {
		controller->Send(c, 0, 1111 + rank);
	}

	//vtk
	vtkSmartPointer<vtkGenericDataObjectWriter> writervtk = vtkSmartPointer<vtkGenericDataObjectWriter>::New();
	std::stringstream ss;

	//vtu
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> writervtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	std::stringstream sss;

	//vtm
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> wvtu = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
	std::stringstream ssss;
	ofstream result;
	//ensight
	vtkEnSightWriter *wcase = vtkEnSightWriter::New();

	switch (config::output::OUTPUT_FORMAT) {
	case config::output::OUTPUT_FORMATAlternatives::VTK_LEGACY_FORMAT:
		std::cout << "LEGACY\n";
		ss << path << config::env::MPIrank << ".vtk";
		writervtk->SetFileName(ss.str().c_str());
		writervtk->SetInputData(c);
		writervtk->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_BINARY_FORMAT:
		std::cout << "BINARY\n";
		sss << path << config::env::MPIrank << ".vtu";
		writervtu->SetFileName(sss.str().c_str());
		writervtu->SetInputData(c);
		writervtu->SetDataModeToBinary();
		writervtu->Write();
		break;

	case config::output::OUTPUT_FORMATAlternatives::VTK_MULTIBLOCK_FORMAT:
		std::cout << "MULTIBLOCK\n";
		ssss << path << config::env::MPIrank << ".vtu";
		wvtu->SetFileName(ssss.str().c_str());
		wvtu->SetInputData(c);
		wvtu->SetDataModeToBinary();
		wvtu->Write();

		MPI_Barrier(MPI_COMM_WORLD);
		int size = config::env::MPIsize;
		if (config::env::MPIrank == 0) {
			result.open("meshC_result.vtm");
			result << "<?xml version=\"1.0\"?>\n";
			result << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\" compressor=\"vtkZLibDataCompressor\">\n";
			std::cout << "Compression: " << config::output::OUTPUT_COMPRESSION << "\n";
			result << " <vtkMultiBlockDataSet>\n";
			for (int i = 0; i < size; i++) {
				result << "  <DataSet index=\"" << i << "\" file=\"meshCorners"
						<< i << ".vtu\">\n  </DataSet>\n";
			}
			result << " </vtkMultiBlockDataSet>\n";
			result << "</VTKFile>\n";
			result.close();
		}
		break;

	case config::output::OUTPUT_FORMATAlternatives::ENSIGHT_FORMAT:
		std::cout << "ENSIGHT\n";

		//write ensight format
		wcase->SetFileName("meshC_result.case");
		wcase->SetNumberOfBlocks(1);
		wcase->SetBlockIDs(blockids);
		wcase->SetTimeStep(0);
		if (config::env::MPIrank == 0) {
			vtkAppendFilter* app = vtkAppendFilter::New();
			app->AddInputData(c);
			for (int i = 1; i < config::env::MPIsize; i++) {
				vtkUnstructuredGrid* h = vtkUnstructuredGrid::New();
				controller->Receive(h, i, 1111 + i);
				app->AddInputData(h);
				std::cout << "prijato\n";
			}
			app->Update();
			ugcase->ShallowCopy(app->GetOutput());
			wcase->SetInputData(ugcase);
			wcase->Write();
			wcase->WriteCaseFile(1);
		}
		break;
	}

	std::cout << "SAVE CORNERS TO VTK\n";
}

void VTK::store(std::vector<std::vector<double> > &displasment, double shrinkSubdomain, double shrinkCluster)
{
	storeValues("displacement", 3, displasment, ElementType::NODES);
	return;
	/*
	 //TO DO compresion
	 std::cout << "Compression: " << config::output::OUTPUT_COMPRESSION << "\n";
	 //pokus();

	 const std::vector<Element*> &elements = _mesh.elements();
	 const std::vector<eslocal> &_partPtrs = _mesh.getPartition();

	 VTKGrid = vtkUnstructuredGrid::New();

	 size_t n_nodsClust = 0;
	 for (size_t iEl = 0; iEl < elements.size(); iEl++) {
	 n_nodsClust += elements[iEl]->nodes();
	 }
	 size_t cnt = 0, n_points = 0;
	 for (size_t d = 0; d < _mesh.parts(); d++) {
	 n_points += _mesh.coordinates().localSize(d);
	 }

	 double shrinking = 0.90;
	 const Coordinates &_coordinates = _mesh.coordinates();
	 double *coord_xyz = new double[n_points * 3];

	 int counter = 0;
	 for (size_t d = 0; d < _mesh.parts(); d++) {
	 Point center;
	 for (size_t c = 0; c < _coordinates.localSize(d); c++) {
	 center += _coordinates.get(c, d);
	 }
	 center /= _coordinates.localSize(d);

	 for (size_t i = 0; i < _coordinates.localSize(d); i++) {
	 Point xyz = _coordinates.get(i, d);
	 xyz = center + (xyz - center) * shrinking;
	 coord_xyz[3 * counter + 0] = xyz.x;
	 coord_xyz[3 * counter + 1] = xyz.y;
	 coord_xyz[3 * counter + 2] = xyz.z;
	 counter++;
	 }
	 }

	 vtkNew < vtkDoubleArray > pointArray;
	 pointArray->SetNumberOfComponents(3);

	 size_t numpoints = n_points * 3;
	 pointArray->SetArray(coord_xyz, numpoints, 1);
	 vtkNew < vtkPoints > points;
	 points->SetData(pointArray.GetPointer());
	 VTKGrid->SetPoints(points.GetPointer());

	 VTKGrid->Allocate(static_cast<vtkIdType>(n_nodsClust));
	 vtkIdType tmp[100]; //max number of  node

	 size_t i = 0;
	 cnt = 0;
	 for (size_t part = 0; part + 1 < _partPtrs.size(); part++) {
	 for (eslocal ii = 0; ii < _partPtrs[part + 1] - _partPtrs[part]; ii++) {
	 for (size_t j = 0; j < elements[i]->nodes(); j++) {
	 tmp[j] = elements[i]->node(j) + cnt;
	 }
	 VTKGrid->InsertNextCell(elements[i]->vtkCode(), elements[i]->nodes(), &tmp[0]);
	 i++;
	 }
	 cnt += _coordinates.localSize(part);
	 }
	 //decompositon

	 float *decomposition_array = new float[elements.size()];

	 counter = 0;
	 for (size_t part = 0; part + 1 < _partPtrs.size(); part++) {
	 for (eslocal i = 0; i < _partPtrs[part + 1] - _partPtrs[part]; i++) {
	 float part_redefine = part;
	 decomposition_array[counter] = part_redefine;
	 counter++;
	 }
	 }

	 vtkNew < vtkFloatArray > decomposition;
	 decomposition->SetName("decomposition");
	 decomposition->SetNumberOfComponents(1);
	 decomposition->SetArray(decomposition_array,
	 static_cast<vtkIdType>(elements.size()), 1);
	 VTKGrid->GetCellData()->AddArray(decomposition.GetPointer());

	 float *d_array = new float[elements.size()];
	 counter = 0;
	 for (size_t part = 0; part + 1 < _partPtrs.size(); part++) {
	 for (eslocal i = 0; i < _partPtrs[part + 1] - _partPtrs[part]; i++) {
	 float part_redefine = part;
	 if (part < (_partPtrs.size() / 2)) {
	 d_array[counter] = 1;
	 } else {
	 d_array[counter] = 0;
	 }
	 counter++;
	 }
	 }

	 vtkNew < vtkFloatArray > dec;
	 dec->SetName("pokus");
	 dec->SetNumberOfComponents(1);
	 dec->SetArray(d_array, static_cast<vtkIdType>(elements.size()), 0);
	 VTKGrid->GetCellData()->AddArray(dec.GetPointer());

	 //displacement
	 int mycounter = 0;
	 for (size_t i = 0; i < displasment.size(); i++) {
	 mycounter += displasment[i].size();
	 }

	 const unsigned int dofs= displasment[0].size() / _mesh.coordinates().localSize(0);

	 double displacement_array[mycounter];

	 counter = 0;



	 for (size_t i = 0; i < displasment.size(); i++) {
	 for (size_t j = 0; j < (displasment[i].size() / dofs); j++) {
	 for(int k=0;k<dofs;k++){
	 displacement_array[dofs * counter + k] = displasment[i][dofs * j + k];
	 }
	 //displacement_array[3 * counter + 1] = displasment[i][3 * j + 1];
	 //displacement_array[3 * counter + 2] = displasment[i][3 * j + 2];

	 counter++;
	 }
	 }

	 std::cout << "NUMBER OF DOFS: " << displasment[0].size() / _mesh.coordinates().localSize(0) << "\n";

	 vtkNew < vtkDoubleArray > displacement;
	 displacement->SetName("displacement");
	 //displacement->SetNumberOfComponents(3);
	 displacement->SetNumberOfComponents(dofs);
	 displacement->SetNumberOfTuples(static_cast<vtkIdType>(counter));
	 VTKGrid->GetPointData()->AddArray(displacement.GetPointer());

	 double* displacementData = displacement_array;
	 vtkIdType numTuples = displacement->GetNumberOfTuples();
	 for (vtkIdType i = 0, counter = 0; i < numTuples; i++, counter++) {
	 //double values[3] = { displacementData[i * 3], displacementData[i * 3 + 1], displacementData[i * 3 + 2] };
	 double values[dofs];
	 for(int k=0;k<dofs;k++){
	 values[k] = {displacementData[i*dofs+k]} ;
	 }

	 displacement->SetTypedTuple(counter, values);
	 }

	 //surface and decimation

	 std::cout << "Decimation: " << config::output::OUTPUT_DECIMATION << "\n";
	 vtkSmartPointer < vtkDecimatePro > decimate = vtkSmartPointer
	 < vtkDecimatePro > ::New();
	 vtkAppendFilter* ap = vtkAppendFilter::New();
	 vtkUnstructuredGrid* decimated = vtkUnstructuredGrid::New();
	 vtkGeometryFilter* gf = vtkGeometryFilter::New();
	 vtkDataSetSurfaceFilter* dssf = vtkDataSetSurfaceFilter::New();
	 vtkTriangleFilter* tf = vtkTriangleFilter::New();

	 vtkSmartPointer < vtkContourFilter > cg = vtkSmartPointer < vtkContourFilter
	 > ::New();

	 if (config::output::OUTPUT_DECIMATION != 0) {
	 gf->SetInputData(VTKGrid);
	 gf->Update();

	 dssf->SetInputData(gf->GetOutput());
	 dssf->Update();

	 tf->SetInputData(dssf->GetOutput());
	 tf->Update();

	 decimate->SetInputData(tf->GetOutput());
	 decimate->SetTargetReduction(config::output::OUTPUT_DECIMATION);
	 decimate->Update();

	 ap->AddInputData(decimate->GetOutput());
	 ap->Update();

	 decimated->ShallowCopy(ap->GetOutput());
	 }

	 //add BlockId
	 bool FCD = false;
	 if (VTKGrid->GetCellData()) {
	 if (VTKGrid->GetCellData()->GetArray("BlockId")) {
	 FCD = true;
	 }
	 }
	 if (FCD == false) {
	 vtkIntArray *bids = vtkIntArray::New();
	 bids->SetName("BlockId");
	 for (i = 0; i < VTKGrid->GetNumberOfCells(); i++) {
	 bids->InsertNextValue(1);
	 }
	 VTKGrid->GetCellData()->SetScalars(bids);
	 }
	 int blockids[2];
	 blockids[0] = 1;
	 blockids[1] = 0;

	 //MultiProces controler
	 vtkUnstructuredGrid* ugcase = vtkUnstructuredGrid::New();
	 vtkMPIController* controller = vtkMPIController::New();
	 if(init==false){
	 controller->Initialize();
	 init=true;
	 }
	 int rank = config::env::MPIrank;


	 //Compresor
	 vtkZLibDataCompressor *myZlibCompressor = vtkZLibDataCompressor::New();
	 myZlibCompressor->SetCompressionLevel(9);

	 //vtk
	 vtkSmartPointer < vtkGenericDataObjectWriter > writervtk = vtkSmartPointer
	 < vtkGenericDataObjectWriter > ::New();
	 std::stringstream ss;

	 //vtu
	 vtkSmartPointer < vtkXMLUnstructuredGridWriter > writervtu = vtkSmartPointer
	 < vtkXMLUnstructuredGridWriter > ::New();
	 std::stringstream sss;

	 //vtm
	 vtkSmartPointer < vtkXMLUnstructuredGridWriter > wvtu = vtkSmartPointer
	 < vtkXMLUnstructuredGridWriter > ::New();
	 std::stringstream ssss;
	 ofstream result;
	 //ensight
	 vtkEnSightWriter *wcase = vtkEnSightWriter::New();

	 switch (config::output::OUTPUT_FORMAT) {
	 case config::output::OUTPUT_FORMATAlternatives::VTK_LEGACY_FORMAT:
	 std::cout << "LEGACY\n";
	 ss << _path << config::env::MPIrank<<".vtk";
	 writervtk->SetFileName(ss.str().c_str());
	 if (config::output::OUTPUT_DECIMATION == 0) {
	 writervtk->SetInputData(VTKGrid);
	 } else {
	 writervtk->SetInputData(decimated);
	 }
	 if (config::output::OUTPUT_COMPRESSION) {
	 // writervtk->SetCompressor(myZlibCompressor);
	 std::cout << "Compression: " << config::output::OUTPUT_COMPRESSION
	 << "\n";
	 }

	 writervtk->Write();

	 break;

	 case config::output::OUTPUT_FORMATAlternatives::VTK_BINARY_FORMAT:
	 std::cout << "BINARY\n";
	 sss << _path << ".vtu";
	 writervtu->SetFileName(sss.str().c_str());
	 if (config::output::OUTPUT_DECIMATION == 0) {
	 writervtu->SetInputData(VTKGrid);
	 } else {
	 writervtu->SetInputData(decimated);
	 }
	 writervtu->SetDataModeToBinary();
	 writervtu->Write();
	 break;

	 case config::output::OUTPUT_FORMATAlternatives::VTK_MULTIBLOCK_FORMAT:
	 std::cout << "MULTIBLOCK\n";
	 ssss << _path << ".vtu";
	 wvtu->SetFileName(ssss.str().c_str());
	 if (config::output::OUTPUT_DECIMATION == 0) {
	 wvtu->SetInputData(VTKGrid);
	 } else {
	 wvtu->SetInputData(decimated);
	 }
	 wvtu->SetDataModeToBinary();
	 wvtu->Write();

	 MPI_Barrier(MPI_COMM_WORLD);
	 int size = config::env::MPIsize;
	 if (config::env::MPIrank == 0) {
	 result.open("result.vtm");
	 result << "<?xml version=\"1.0\"?>\n";
	 if (!config::output::OUTPUT_COMPRESSION) {
	 result
	 << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\">\n";
	 std::cout << "Compression: "
	 << config::output::OUTPUT_COMPRESSION << "\n";
	 } else {
	 result
	 << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\" compressor=\"vtkZLibDataCompressor\">\n";
	 std::cout << "Compression: "
	 << config::output::OUTPUT_COMPRESSION << "\n";
	 }

	 result << " <vtkMultiBlockDataSet>\n";
	 for (int i = 0; i < size; i++) {
	 result << "  <DataSet index=\"" << i << "\" file=\"result" << i
	 << ".vtu\">\n  </DataSet>\n";
	 }
	 result << " </vtkMultiBlockDataSet>\n";
	 result << "</VTKFile>\n";
	 result.close();
	 }
	 break;

	 case config::output::OUTPUT_FORMATAlternatives::ENSIGHT_FORMAT:
	 std::cout << "ENSIGHT\n";
	 if (rank != 0) {
	 controller->Send(VTKGrid, 0, 1111 + rank);
	 }

	 //write ensight format
	 wcase->SetFileName("result.case");
	 wcase->SetNumberOfBlocks(1);
	 wcase->SetBlockIDs(blockids);
	 wcase->SetTimeStep(0);
	 if (config::env::MPIrank == 0) {
	 vtkAppendFilter* app = vtkAppendFilter::New();
	 app->AddInputData(VTKGrid);
	 for (int i = 1; i < config::env::MPIsize; i++) {
	 vtkUnstructuredGrid* h = vtkUnstructuredGrid::New();
	 controller->Receive(h, i, 1111 + i);
	 app->AddInputData(h);
	 std::cout << "prijato\n";
	 }
	 app->Update();
	 ugcase->ShallowCopy(app->GetOutput());
	 wcase->SetInputData(ugcase);
	 wcase->Write();
	 wcase->WriteCaseFile(1);
	 }
	 break;
	 }*/
}

void VTK::gluing(const Mesh &mesh, const EqualityConstraints &constraints, const std::string &path, size_t dofs, double shrinkSubdomain, double shrinkCluster)
{
	/*
	 vtkPolyData* gp[dofs];
	 vtkPoints* po[dofs];
	 vtkPoints* points[dofs];
	 vtkCellArray* lines[dofs];
	 vtkSmartPointer<vtkCellArray> ver[dofs];
	 for(int i=0;i<dofs;i++){
	 gp[i]=vtkPolyData::New();
	 po[i]=vtkPoints::New();
	 points[i]=vtkPoints::New();
	 lines[i]=vtkCellArray::New();
	 ver[i]=vtkSmartPointer<vtkCellArray>::New();
	 }
	 vtkGenericDataObjectWriter* wg=vtkGenericDataObjectWriter::New();
	 vtkAppendPolyData* ap=vtkAppendPolyData::New();
	 //int V=mesh.coordinates().localSize(0) * constraints.B1.size();
	 std::vector<int> row;
	 std::vector<int> val;

	 MPI_Status status;
	 int matr=0;
	 int l=0;


	 //        const Boundaries &sBoundaries = mesh.subdomainBoundaries();
	 //sBoundaries[5] -> vector subdomen
	 //const Boundaries &cBoundaries = mesh.clusterBoundaries();
	 //constraints.B1[0].I_row_indices
	 //constraints.B1[0].J_col_indices
	 //constraints.B1[0].V_values


	 for (int y = 0; y < mesh.parts(); y++) {
	 for(int i=0;i<constraints.B1[y].I_row_indices.size();i++){
	 int h=constraints.B1[y].I_row_indices[i];//I souradnice ke srovnani
	 int index1=constraints.B1[y].J_col_indices[i]-1;

	 //ESINFO(ALWAYS) << h << ":" << index1 << "\n";

	 Point *p1=new Point[dofs];
	 for(int pd=0;pd<dofs;pd++){
	 p1[pd]=mesh.coordinates().get(index1/dofs,y);
	 p1[pd]=centers[y]+(p1[pd]-centers[y])*0.9;
	 vtkIdType pid[1];
	 pid[0] = points[pd]->InsertNextPoint(p1[pd].x,p1[pd].y,p1[pd].z);
	 ver[pd]->InsertNextCell(1,pid);
	 }

	 row.push_back(h);
	 val.push_back(constraints.B1[y].V_values[i]);
	 matr++;


	 for(int p=0;p<mesh.parts();p++){
	 if(y<=p){
	 for(int k=0;k<constraints.B1[p].I_row_indices.size();k++){
	 if(constraints.B1[p].I_row_indices[k]==h){
	 if((constraints.B1[y].V_values[i]+constraints.B1[p].V_values[k])==0){


	 int index2=constraints.B1[p].J_col_indices[k]-1;

	 Point p2[dofs];
	 for(int pd=0;pd<dofs;pd++){
	 p2[pd]=mesh.coordinates().get(index2/dofs,p);
	 p2[pd]=centers[p]+(p2[pd]-centers[p])*0.9;
	 po[pd]->InsertNextPoint(p1[pd].x,p1[pd].y,p1[pd].z);
	 po[pd]->InsertNextPoint(p2[pd].x,p2[pd].y,p2[pd].z);
	 vtkLine* ls=vtkLine::New();
	 ls->GetPointIds()->SetId(0,l);
	 ls->GetPointIds()->SetId(1,l+1);
	 lines[pd]->InsertNextCell(ls);
	 }

	 l+=2;
	 //std::cout<<"prvni: "<<p1.x<<" "<<p1.y<<" "<<p1.z<<" druha: "<<p2.x<<" "<<p2.y<<" "<<p2.z<<std::endl;


	 }
	 else{
	 //std::cout<<"!!!"<<std::endl;
	 }
	 }
	 }
	 }
	 }
	 }
	 }

	 //MPI dirichlet control
	 if(config::env::MPIsize>1){
	 MPI_Barrier(MPI_COMM_WORLD);

	 vtkPolyData* PPoints[dofs];
	 for(int i=0;i<dofs;i++){
	 PPoints[i]=vtkPolyData::New();
	 PPoints[i]->SetPoints(points[i]);
	 }


	 int rank = config::env::MPIrank;

	 for(int i=0;i<config::env::MPIsize;i++){
	 std::vector<int> row2;
	 vtkPolyData* PPoints2[dofs];
	 for(int pd=0;pd<dofs;pd++){
	 PPoints2[pd]=vtkPolyData::New();
	 }
	 std::vector<int> val2;
	 int vs;

	 if(i==rank){
	 row2=row;
	 for(int pd=0;pd<dofs;pd++){
	 PPoints2[pd]->DeepCopy(PPoints[pd]);
	 }
	 val2=val;
	 vs=row2.size();
	 }
	 MPI_Barrier(MPI_COMM_WORLD);
	 MPI_Bcast(&vs,1,MPI_INT,i,MPI_COMM_WORLD);

	 row2.resize(vs);
	 val2.resize(vs);

	 MPI_Bcast(row2.data(),row2.size(),MPI_INT,i,MPI_COMM_WORLD);
	 MPI_Bcast(val2.data(),val2.size(),MPI_INT,i,MPI_COMM_WORLD);
	 for(int pd=0;pd<dofs;pd++){
	 controller->Broadcast(PPoints2[pd],i);
	 }

	 if(i!=rank){
	 int lg;
	 if(row.size()<row2.size()){
	 lg=row.size();
	 }
	 else{
	 lg=row2.size();
	 }

	 for(int i=0;i<lg;i++){
	 for(int y=0;y<lg;y++){
	 if(row.at(i)==row2.at(y)){
	 if((val.at(i)+val2.at(y))==0){
	 vtkIdType dru=0;
	 for(vtkIdType jed=0;jed<dofs;jed++){
	 double p1[3];
	 double p2[3];
	 PPoints[jed]->GetPoint(i,p1);
	 PPoints2[jed]->GetPoint(y,p2);
	 dru++;
	 po[jed]->InsertNextPoint(p1[0],p1[1],p1[2]);
	 po[jed]->InsertNextPoint(p2[0],p2[1],p2[2]);

	 vtkLine* ls=vtkLine::New();
	 ls->GetPointIds()->SetId(0,l);
	 ls->GetPointIds()->SetId(1,l+1);
	 lines[jed]->InsertNextCell(ls);
	 }
	 l+=2;
	 }
	 }
	 }
	 }
	 }


	 MPI_Barrier(MPI_COMM_WORLD);
	 }
	 }
	 */
	/*for(int i=0;i<config::env::MPIsize;i++){
	 int clust2[2];
	 std::vector<int> bufclust;
	 std::vector<int> bufrow;
	 std::vector<int> bufpointx;
	 std::vector<int> bufpointy;
	 int bufsize;
	 vtkPolyData* PPoints2=vtkPolyData::New();
	 std::cout<<"Proces "<<rank<<" "<<i<<std::endl;
	 if(rank==i){
	 //std::cout<<"cluster"<<i<<std::endl<<constraints.B1clustersMap<<std::endl;
	 for(int j=0;j<mesh.parts();j++){
	 for(int k=0;k<constraints.B1[j].I_row_indices.size();k++){
	 for(int c=0;c<constraints.B1clustersMap.size();c++){
	 int row2=constraints.B1clustersMap[c][0];
	 clust2[0]=constraints.B1clustersMap[c][1];
	 clust2[1]=constraints.B1clustersMap[c][2];
	 if(clust2[1]<config::env::MPIsize && clust2[1]>=0){
	 if(row.at(k)==row2){
	 //send message to cluster clust2[1] to give me point at row2
	 //std::cout<<"Nasli se body mezi clustery "<<row.at(k)<<" "<<row2<<" "<<clust2[1]<<std::endl;
	 bufclust.push_back(clust2[1]);
	 bufrow.push_back(row2);

	 double p1[3];
	 PPoints->GetPoint(k,p1);
	 bufpointx.push_back(p1[0]);
	 bufpointy.push_back(p1[1]);
	 }
	 }
	 }
	 }
	 }
	 bufsize=bufclust.size();
	 }
	 MPI_Bcast(&bufsize,1,MPI_INT,i,MPI_COMM_WORLD);

	 bufclust.resize(bufsize);
	 bufrow.resize(bufsize);
	 bufpointx.resize(bufsize);
	 bufpointy.resize(bufsize);

	 MPI_Bcast(bufclust.data(),bufclust.size(),MPI_INT,i,MPI_COMM_WORLD);
	 MPI_Bcast(bufrow.data(),bufrow.size(),MPI_INT,i,MPI_COMM_WORLD);
	 MPI_Bcast(bufpointx.data(),bufpointx.size(),MPI_INT,i,MPI_COMM_WORLD);
	 MPI_Bcast(bufpointy.data(),bufpointy.size(),MPI_INT,i,MPI_COMM_WORLD);

	 //std::cout<<bufclust.at(0)<<std::endl;
	 for(int c=0;c<bufclust.size();c++){
	 if(bufclust.at(c)==rank){
	 //std::cout<<"Poslany clust:  "<<bufclust.at(c)<<" "<<i<<" "<<rank<<" row: "<<bufrow.at(c)<<std::endl;
	 for(int sm=0;sm<mesh.parts();sm++){
	 for(int s=0;s<constraints.B1[sm].I_row_indices.size();s++){
	 if(bufrow.at(c)==constraints.B1[sm].I_row_indices[s]){
	 double p1[3];
	 vtkIdType it=s;
	 PPoints->GetPoint(it,p1);

	 double p2[3];
	 p2[0]=bufpointx.at(c);
	 p2[1]=bufpointy.at(c);
	 p2[2]=0;

	 //make line
	 po->InsertNextPoint(p1[0],p1[1],p1[2]);
	 po->InsertNextPoint(p2[0],p2[1],p2[2]);

	 vtkLine* ls=vtkLine::New();
	 ls->GetPointIds()->SetId(0,l);
	 ls->GetPointIds()->SetId(1,l+1);
	 lines->InsertNextCell(ls);
	 l+=2;
	 //std::cout<<"Udelana lajna..."<<std::endl;
	 }
	 }
	 }
	 }
	 }

	 MPI_Barrier(MPI_COMM_WORLD);
	 }*/

	/*


	 //write the polydata
	 for(int i=0;i<dofs;i++){
	 gp[i]->SetPoints(po[i]);
	 gp[i]->SetLines(lines[i]);

	 ap->AddInputData(gp[i]);

	 vtkPolyData* gp2=vtkPolyData::New();
	 gp2->SetPoints(points[i]);
	 gp2->SetVerts(ver[i]);

	 ap->AddInputData(gp2);
	 ap->Update();

	 std::cout << "LEGACY\n";
	 std::stringstream ss;
	 ss << path << config::env::MPIrank<<i<<".vtk";
	 wg->SetFileName(ss.str().c_str());
	 wg->SetInputData(ap->GetOutput());
	 wg->Write();
	 }

	 */
}