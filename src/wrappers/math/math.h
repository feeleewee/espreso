
#ifndef SRC_WRAPPERS_MATH_MATH_H_
#define SRC_WRAPPERS_MATH_MATH_H_

namespace espreso {

struct MATH {

	static void setNumberOfThreads(int numberOfThreads);

	static void upCSRMatVecProduct(eslocal rows, eslocal cols, eslocal *mRows, eslocal *mCols, float *mVals, float *vVals, float *result);
	static void upCSRMatVecProduct(eslocal rows, eslocal cols, eslocal *mRows, eslocal *mCols, double *mVals, double *vVals, double *result);

	static void vecScale(eslocal size, float alpha, float *vVals);
	static void vecScale(eslocal size, double alpha, double *vVals);

	static double vecNorm(eslocal size, float *vVals);
	static double vecNorm(eslocal size, double *vVals);

	static eslocal vecNormMaxIndex(eslocal size, float *vVals);
	static eslocal vecNormMaxIndex(eslocal size, double *vVals);

	// C = alpha * A * B + beta * C
	static void DenseMatDenseMatRowMajorProduct(
			double alpha, bool transposeA, eslocal aRows, eslocal aCols, double* aVals,
			bool transposeB, eslocal bCols, double* bVals,
			double beta, double* cVals);

	struct SOLVER {

		static void GMRESUpCRSMat(
				eslocal rows, eslocal cols, eslocal *mRows, eslocal *mCols, double *mVals,
				double *rhsVals, double *results,
				double tolerance, eslocal maxIterations, eslocal &itercount);

		static void GMRESDenseRowMajorMat(
				eslocal rows, eslocal cols, double *mVals,
				double *rhsVals, double *results,
				double tolerance, eslocal maxIterations, eslocal &itercount);

		static void GMRESUpperSymetricColumnMajorMat(
				eslocal cols, double *mVals,
				double *rhsVals, double *results,
				double tolerance, eslocal maxIterations, eslocal &itercount);

		static eslocal directUpperSymetricIndefiniteColumnMajor(
				eslocal cols, double *m_packed_values,
				eslocal nrhs, double *rhsVals);
	};
};

}



#endif /* SRC_WRAPPERS_MATH_MATH_H_ */
