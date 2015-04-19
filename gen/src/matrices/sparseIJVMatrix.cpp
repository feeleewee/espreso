#include "sparseIJVMatrix.h"

SparseIJVMatrix::SparseIJVMatrix(const DenseMatrix &other): Matrix(other.type(), other.rows(), other.columns())
{
	MKL_INT nnz = other.nonZeroValues();
	_rowIndices.reserve(nnz);
	_colIndices.reserve(nnz);
	_values.reserve(nnz);

	for(size_t r = 0; r < other.rows(); r++) {
		for(size_t c = (_type == Matrix::SYMETRIC)? r: 0; c < other.columns(); c++) {
			if (other(r, c) != 0) {
				_rowIndices.push_back(r);
				_colIndices.push_back(c);
				_values.push_back(other(r, c));
			}
		}
	}
}

SparseIJVMatrix::SparseIJVMatrix(const SparseDOKMatrix &other): Matrix(other.type(), other.rows(), other.columns())
{
	MKL_INT nnz = other.nonZeroValues();
	_rowIndices.reserve(nnz);
	_colIndices.reserve(nnz);
	_values.reserve(nnz);

	const MatrixMap &dokValues = other.values();
	MatrixMap::const_iterator row;
	for(row = dokValues.begin(); row != dokValues.end(); ++row) {
		const ColumnMap &columns = row->second;

		ColumnMap::const_iterator column;
		for(column = columns.begin(); column != columns.end(); ++column) {
			if (column->second != 0) {
				_rowIndices.push_back(row->first);
				_colIndices.push_back(column->first);
				_values.push_back(column->second);
			}
		}
	}
}

SparseIJVMatrix::SparseIJVMatrix(const SparseCSRMatrix &other): Matrix(other.type(), other.rows(), other.columns())
{
	MKL_INT nnz = other.nonZeroValues();
	_rowIndices.resize(nnz);
	_colIndices.resize(nnz);
	_values.resize(nnz);

	std::cout << "NONZERO: " << nnz << "\n";

	MKL_INT job[6] = {
		0, 			// CSR to IJV
		0,			// zero based indexing
		0,			// zero based indexing
		0,			// without any meaning
		nnz,		// non-zero values
		3,			// fill all output arrays
	};

	MKL_INT info;

	mkl_dcsrcoo(
		job, &_rows,
		const_cast<double*>(other.values()), const_cast<MKL_INT*>(other.columnIndices()), const_cast<MKL_INT*>(other.rowPtrs()),
		&nnz, values(), rowIndices(), columnIndices(),
		&info);
}

void SparseIJVMatrix::makeTransposition()
{
	_rowIndices.swap(_colIndices);
}
