
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

VTKXMLASCII::VTKXMLASCII(const OutputConfiguration &output, const Mesh *mesh, const std::string &path, MeshInfo::InfoMode mode)
: VTKXML(output, mesh, path, mode)
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
static void addPointData(vtkUnstructuredGrid * VTKGrid, const std::string &name, size_t components, const std::vector<TType> &data)
{
	vtkNew<TVTKType> vtkArray;
	vtkArray->SetName(name.c_str());
	vtkArray->SetNumberOfComponents(components);
	vtkArray->SetArray(const_cast<TType*>(data.data()), static_cast<vtkIdType>(data.size()), 1);

	VTKGrid->GetPointData()->AddArray(vtkArray.GetPointer());
	if (VTKGrid->GetPointData()->GetNumberOfArrays() == 1) {
		VTKGrid->GetPointData()->SetActiveScalars(name.c_str());
	}
}

template <typename TVTKType, typename TType>
static void addCellData(vtkUnstructuredGrid * VTKGrid, const std::string &name, size_t components, const std::vector<TType> &data)
{
	vtkNew<TVTKType> vtkArray;
	vtkArray->SetName(name.c_str());
	vtkArray->SetNumberOfComponents(components);
	vtkArray->SetArray(const_cast<TType*>(data.data()), static_cast<vtkIdType>(data.size()), 1);

	VTKGrid->GetCellData()->AddArray(vtkArray.GetPointer());
}


void VTKXMLASCII::storePointData(const std::string &name, size_t components, const std::vector<int> &data)
{
	addPointData<vtkIntArray>(_VTKGrid, name, components, data);
}

void VTKXMLASCII::storePointData(const std::string &name, size_t components, const std::vector<long> &data)
{
	addPointData<vtkLongArray>(_VTKGrid, name, components, data);
}

void VTKXMLASCII::storePointData(const std::string &name, size_t components, const std::vector<double> &data)
{
	addPointData<vtkDoubleArray>(_VTKGrid, name, components, data);
}


void VTKXMLASCII::storeCellData(const std::string &name, size_t components, const std::vector<int> &data)
{
	addCellData<vtkIntArray>(_VTKGrid, name, components, data);
}

void VTKXMLASCII::storeCellData(const std::string &name, size_t components, const std::vector<long> &data)
{
	addCellData<vtkLongArray>(_VTKGrid, name, components, data);
}

void VTKXMLASCII::storeCellData(const std::string &name, size_t components, const std::vector<double> &data)
{
	addCellData<vtkDoubleArray>(_VTKGrid, name, components, data);
}



