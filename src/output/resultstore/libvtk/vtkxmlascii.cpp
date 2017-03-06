
#include "../vtkxmlascii.h"

#include "vtkNew.h"

#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkDoubleArray.h"

#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridWriter.h"

using namespace espreso::output;

VTKXMLASCII::VTKXMLASCII(const OutputConfiguration &output, const Mesh *mesh, const std::string &path)
: VTKXML(output, mesh, path)
{
	_writer = vtkXMLUnstructuredGridWriter::New();
	_writer->SetDataModeToAscii();
	_writer->SetCompressorTypeToNone();
}

VTKXMLASCII::~VTKXMLASCII()
{
	_writer->Delete();
}

template <typename TVTKType, typename TType>
static void* addPointData(vtkUnstructuredGrid * VTKGrid, const std::string &name, size_t points, const std::vector<std::vector<TType> > &data)
{
	size_t size = 0;
	for (size_t i = 0; i < data.size(); i++) {
		size += data[i].size();
	}
	TType *array = new TType[size];
	for (size_t i = 0, offset = 0; i < data.size(); offset += data[i++].size()) {
		memcpy(array + offset, data[i].data(), data[i].size() * sizeof(TType));
	}

	vtkNew<TVTKType> vtkArray;
	vtkArray->SetName(name.c_str());
	vtkArray->SetNumberOfComponents(size / points);
	vtkArray->SetArray(array, static_cast<vtkIdType>(size), 1);

	VTKGrid->GetPointData()->AddArray(vtkArray.GetPointer());
	if (VTKGrid->GetPointData()->GetNumberOfArrays() == 1) {
		VTKGrid->GetPointData()->SetActiveScalars(name.c_str());
	}
	return array;
}

template <typename TVTKType, typename TType>
static void addPointData(vtkUnstructuredGrid * VTKGrid, const std::string &name, size_t points, const std::vector<TType> &data)
{
	vtkNew<TVTKType> vtkArray;
	vtkArray->SetName(name.c_str());
	vtkArray->SetNumberOfComponents(data.size() / points);
	vtkArray->SetArray(const_cast<TType*>(data.data()), static_cast<vtkIdType>(data.size()), 1);

	VTKGrid->GetPointData()->AddArray(vtkArray.GetPointer());
	if (VTKGrid->GetPointData()->GetNumberOfArrays() == 1) {
		VTKGrid->GetPointData()->SetActiveScalars(name.c_str());
	}
}

template <typename TVTKType, typename TType>
static void* addCellData(vtkUnstructuredGrid * VTKGrid, const std::string &name, size_t cells, const std::vector<std::vector<TType> > &data)
{
	size_t size = 0;
	for (size_t i = 0; i < data.size(); i++) {
		size += data[i].size();
	}
	TType *array = new TType[size];
	for (size_t i = 0, offset = 0; i < data.size(); offset += data[i++].size()) {
		memcpy(array + offset, data[i].data(), data[i].size() * sizeof(TType));
	}
	vtkNew<TVTKType> vtkArray;
	vtkArray->SetName(name.c_str());
	vtkArray->SetNumberOfComponents(size / cells);
	vtkArray->SetArray(array, static_cast<vtkIdType>(size), 1);

	VTKGrid->GetCellData()->AddArray(vtkArray.GetPointer());
	return array;
}

template <typename TVTKType, typename TType>
static void addCellData(vtkUnstructuredGrid * VTKGrid, const std::string &name, size_t cells, const std::vector<TType> &data)
{
	vtkNew<TVTKType> vtkArray;
	vtkArray->SetName(name.c_str());
	vtkArray->SetNumberOfComponents(data.size() / cells);
	vtkArray->SetArray(const_cast<TType*>(data.data()), static_cast<vtkIdType>(data.size()), 1);

	VTKGrid->GetCellData()->AddArray(vtkArray.GetPointer());
}

void VTKXMLASCII::storePointData(const std::string &name, size_t points, const std::vector<std::vector<int> > &data)
{
	_VTKDataArrays.push_back(addPointData<vtkIntArray>(_VTKGrid, name, points, data));
}

void VTKXMLASCII::storePointData(const std::string &name, size_t points, const std::vector<std::vector<long> > &data)
{
	_VTKDataArrays.push_back(addPointData<vtkLongArray>(_VTKGrid, name, points, data));
}

void VTKXMLASCII::storePointData(const std::string &name, size_t points, const std::vector<std::vector<double> > &data)
{
	_VTKDataArrays.push_back(addPointData<vtkDoubleArray>(_VTKGrid, name, points, data));
}

void VTKXMLASCII::storePointData(const std::string &name, size_t points, const std::vector<int> &data)
{
	addPointData<vtkIntArray>(_VTKGrid, name, points, data);
}

void VTKXMLASCII::storePointData(const std::string &name, size_t points, const std::vector<long> &data)
{
	addPointData<vtkLongArray>(_VTKGrid, name, points, data);
}

void VTKXMLASCII::storePointData(const std::string &name, size_t points, const std::vector<double> &data)
{
	addPointData<vtkDoubleArray>(_VTKGrid, name, points, data);
}


void VTKXMLASCII::storeCellData(const std::string &name, size_t cells, const std::vector<std::vector<int> > &data)
{
	_VTKDataArrays.push_back(addCellData<vtkIntArray>(_VTKGrid, name, cells, data));
}

void VTKXMLASCII::storeCellData(const std::string &name, size_t cells, const std::vector<std::vector<long> > &data)
{
	_VTKDataArrays.push_back(addCellData<vtkLongArray>(_VTKGrid, name, cells, data));
}

void VTKXMLASCII::storeCellData(const std::string &name, size_t cells, const std::vector<std::vector<double> > &data)
{
	_VTKDataArrays.push_back(addCellData<vtkDoubleArray>(_VTKGrid, name, cells, data));
}

void VTKXMLASCII::storeCellData(const std::string &name, size_t cells, const std::vector<int> &data)
{
	addCellData<vtkIntArray>(_VTKGrid, name, cells, data);
}

void VTKXMLASCII::storeCellData(const std::string &name, size_t cells, const std::vector<long> &data)
{
	addCellData<vtkLongArray>(_VTKGrid, name, cells, data);
}

void VTKXMLASCII::storeCellData(const std::string &name, size_t cells, const std::vector<double> &data)
{
	addCellData<vtkDoubleArray>(_VTKGrid, name, cells, data);
}



