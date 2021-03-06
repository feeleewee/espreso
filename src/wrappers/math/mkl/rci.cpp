
#include "../math.h"

#include "../../../basis/logging/logging.h"

#include "mkl.h"

#include <vector>

using namespace espreso;

enum class SOLVER_INTERNAL_TYPE {
	DCSRGEMV,
	DGEMV,
	DSPMV,
};

void GMRESolverInternal(SOLVER_INTERNAL_TYPE type,
						eslocal rows, eslocal cols, eslocal *mRows, eslocal *mCols, double *mVals,
						double *rhsVals, double *results,
						double tolerance, eslocal maxIterations, eslocal &itercount)
{
	//---------------------------------------------------------------------------
		// Define arrays for the coefficient matrix
		// Compressed sparse row storage is used for sparse representation
		//---------------------------------------------------------------------------
		eslocal size = 128;

		//---------------------------------------------------------------------------
		// Allocate storage for the ?par parameters and the solution/rhs/residual vectors
		//---------------------------------------------------------------------------
		eslocal ipar[size];
		double dpar[size];
		std::vector<double> tmp(cols * (2 * cols + 1) + (cols * (cols + 9)) / 2 + 1);

		//---------------------------------------------------------------------------
		// Some additional variables to use with the RCI (P)FGMRES solver
		//---------------------------------------------------------------------------
		eslocal RCI_request, ivar;
		ivar = cols;

		char uplo = 'U';
		char transa = 'N';

		double alpha = 1.0;
		double beta = 0.0;

		eslocal incx = 1;
		eslocal incy = 1;

		//---------------------------------------------------------------------------
		// Initialize the initial guess
		//---------------------------------------------------------------------------
		for (eslocal i = 0; i < cols; i++) {
			results[i] = 0.0;
		}

		//---------------------------------------------------------------------------
		// Initialize the solver
		//---------------------------------------------------------------------------
		dfgmres_init(&ivar, results, rhsVals, &RCI_request, ipar, dpar, tmp.data());
		if (RCI_request != 0) {
			ESINFO(ERROR) << "Something wrong happens while 'dfgmres_init' in solver.";
		}

		//---------------------------------------------------------------------------
		// Set the desired parameters:
		// https://software.intel.com/en-us/node/521710
		//---------------------------------------------------------------------------
		ipar[4] = maxIterations;
		dpar[0] = tolerance;

		ipar[7] = 1;
		ipar[8] = 1;
		ipar[9] = 0;
		ipar[10] = 0;
		ipar[11] = 1;

		//---------------------------------------------------------------------------
		// Check the correctness and consistency of the newly set parameters
		//---------------------------------------------------------------------------
		dfgmres_check(&ivar, results, rhsVals, &RCI_request, ipar, dpar,tmp.data());
		if (RCI_request != 0) {
			ESINFO(ERROR) << "Something wrong happens while 'dfgmres_check' in solver.";
		}

		//---------------------------------------------------------------------------
		// Compute the solution by RCI (P)FGMRES solver with preconditioning
		// Reverse Communication starts here
		//---------------------------------------------------------------------------
		while (true) {
			dfgmres(&ivar, results, rhsVals, &RCI_request, ipar, dpar, tmp.data());
			//---------------------------------------------------------------------------
			// If RCI_request=0, then the solution was found with the required precision
			//---------------------------------------------------------------------------
			//std::cout<<"RCI "<<RCI_request<<std::endl;

			if (RCI_request == 0) break;
			//---------------------------------------------------------------------------
			// If RCI_request=1, then compute the vector A*tmp[ipar[21]-1]
			// and put the result in vector tmp[ipar[22]-1]
			//---------------------------------------------------------------------------
			if (RCI_request == 1) {
				switch (type) {
					case SOLVER_INTERNAL_TYPE::DSPMV: {
						dspmv(&uplo, &cols, &alpha, mVals, tmp.data() + ipar[21] - 1, &incx, &beta, tmp.data() + ipar[22] - 1, &incy);
					}break;

					case SOLVER_INTERNAL_TYPE::DGEMV: {
						dgemv(&transa, &rows, &cols, &alpha, mVals, &rows, tmp.data() + ipar[21] - 1, &incx, &beta, tmp.data() + ipar[22] - 1, &incy );
					}

					case SOLVER_INTERNAL_TYPE::DCSRGEMV: {
						mkl_dcsrgemv(&transa, &ivar, mVals, mRows, mCols, tmp.data() + ipar[21] - 1, tmp.data() + ipar[22] - 1);
					}

					default:
						ESINFO(GLOBAL_ERROR) << "ESPRESO internal error: no such internal solver.";
				}
				continue;
			}

			break;
		}
		//---------------------------------------------------------------------------
		// Reverse Communication ends here
		// Get the current iteration number and the FGMRES solution (DO NOT FORGET to
		// call dfgmres_get routine as computed_solution is still containing
		// the initial guess!). Request to dfgmres_get to put the solution
		// into vector computed_solution[N] via ipar[12]
		//---------------------------------------------------------------------------
		ipar[12] = 0;
		dfgmres_get(&ivar, results, rhsVals, &RCI_request, ipar, dpar, tmp.data(), &itercount);
		//---------------------------------------------------------------------------
		// Print solution vector: computed_solution[N] and the number of iterations: itercount
		//---------------------------------------------------------------------------
		if (RCI_request != 0) {
			ESINFO(ERROR) << "Something wrong happens while 'dfgmres_get' in solver.";
		}

}

void MATH::SOLVER::GMRESUpCRSMat(
		eslocal rows, eslocal cols, eslocal *mRows, eslocal *mCols, double *mVals,
		double *rhsVals, double *results,
		double tolerance, eslocal maxIterations, eslocal &itercount)
{
	GMRESolverInternal(SOLVER_INTERNAL_TYPE::DCSRGEMV,
			rows, cols, mRows, mCols, mVals,
			rhsVals, results,
			tolerance, maxIterations, itercount);

}

void MATH::SOLVER::GMRESDenseRowMajorMat(
		eslocal rows, eslocal cols, double *mVals,
		double *rhsVals, double *results,
		double tolerance, eslocal maxIterations, eslocal &itercount)
{
	GMRESolverInternal(SOLVER_INTERNAL_TYPE::DGEMV,
							rows, cols, NULL, NULL, mVals,
							rhsVals, results,
							tolerance, maxIterations, itercount);
}


void MATH::SOLVER::GMRESUpperSymetricColumnMajorMat(
		eslocal cols, double *mVals,
		double *rhsVals, double *results,
		double tolerance, eslocal maxIterations, eslocal &itercount)
{
	GMRESolverInternal(SOLVER_INTERNAL_TYPE::DSPMV,
							cols, cols, NULL, NULL, mVals,
							rhsVals, results,
							tolerance, maxIterations, itercount);
}

eslocal MATH::SOLVER::directUpperSymetricIndefiniteColumnMajor(
		eslocal cols, double *m_packed_values,
		eslocal nrhs, double *rhsVals)
{
	char U = 'U';
	eslocal info;
	std::vector<eslocal>  m_ipiv(cols);

	dsptrf(&U, &cols, &m_packed_values[0], &m_ipiv[0], &info);

	if (info == 0 ) {
		dsptrs(&U, &cols, &nrhs, &m_packed_values[0], &m_ipiv[0], &rhsVals[0], &cols, &info);
	}
	return info;
}


