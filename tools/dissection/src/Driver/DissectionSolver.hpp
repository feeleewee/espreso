/*! \file DissectionSolver.hpp
    \brief task mangemanet of dissection algorithm
    \author Atsushi Suzuki, Laboratoire Jacques-Louis Lions
    \date   Mar. 30th 2012
    \date   Jul. 12th 2015
    \date   Feb. 29th 2016
*/

// This file is part of Dissection
// 
// Dissection is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Dissection is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dissection.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _DISSECTION_SOLVER_
#define _DISSECTION_SOLVER_
#include <cassert>
#include <vector>

#include "Compiler/OptionCompiler.hpp"
#include "Driver/C_threads_tasks.hpp"
#include "Driver/DissectionMatrix.hpp"
#include "Driver/DissectionQueue.hpp"
#include "Driver/TridiagBlockMatrix.hpp"
#include "Driver/TridiagQueue.hpp"
#include "Algebra/SparseMatrix.hpp"
#include "Driver/DissectionDefault.hpp"

// #include <map>
#include <vector>
#include <list>
#include "Splitters/BisectionTree.hpp"

#ifdef BLAS_MKL
#include "mkl_lapack.h"
#include "mkl_types.h"
#endif

# include <cstdlib>
#include <time.h>

using std::vector;
using std::list;

// example on usage of template
// T : complex<quadruple>, U : quadruple, W : complex<duoble>, Z : double
// T : complex<quadruple>, U : quadruple, W = T, Z = U
// T : quadruple,  U = T,  W = T,  Z = U
template<typename T, typename U = T, typename W = T, typename Z = U, typename X = T, typename Y = U>
class DissectionSolver
{
public:
  DissectionSolver(int num_threads, bool verbose, int called, FILE *fp) :
    _precDiag(NULL), _verbose(verbose), _num_threads(num_threads),
    _status_factorized(false), _called(called), _fp(fp)
  {  
  }

  ~DissectionSolver()
  {
    Destroy();
  }
  
  void SaveCSRMatrix(const int called,
		     const W *coefs);
  void SaveMMMatrix(const int called,
		    const W *coefs);

  void Destroy(void);

  void NumericFree(void);

  void SymbolicFact(const int dim,
		    const int *ptRows,
		    const int *indCols,
		    const bool sym,
		    const bool upper,
		    const bool isWhole = false,
		    const int decomposer = SCOTCH_DECOMPOSER,
		    const int nbLevels = (-1),
		    const int minNodes = MINNODES);

  void NumericFact(const int called,
		   W *coefs,
		   const int scaling = KKT_SCALING, //useful for the Stokes eqs.
		   const double eps_pivot = EPS_PIVOT,
		   const bool kernel_detection_all = false,
		   const int dim_augkern = DIM_AUG_KERN);

  bool getFactorized(void) { return _status_factorized; };

  void CopyQueueFwBw(DissectionSolver<X, Y, T, U> &qdslv, T *coefs);

  void GetNullPivotIndices(int *pivots);

  void GetKernelVectors(T *kern_basis);
  void GetTransKernelVectors(T *kernt_basis);
  void GetMatrixScaling(Z *weight);

  void ProjectionImageSingle(T *x, string name = string(""));
  void ProjectionKernelOrthSingle(T *x, bool isTrans);
  void ProjectionImageMulti(T *x, int nrhs);

  void SpMV(const T *x, T *y);
  void SpMtV(const T *x, T *y);

  void SolveScaled(T *x, int nrhs, bool isTrans);
  void QueueFwBw(T **x, int *nrhs);

  void SolveSingle(T *x, bool projection, bool isTrans, bool isScaling);
  void SolveMulti(T *x, int nrhs, bool projection, bool isTrans,
		  bool isScaling);

  void SolveSingleDebug(T *x, T* y);

  void BuildSingCoefs(T *DSsingCoefs, 
		      SparseMatrix<T> *DCsCoefs,
		      T *DBsCoefs,
		      vector<int> &singIdx);
  
  void BuildKernels(vector<int> &singIdx_, 
		    int n2,
		    SchurMatrix<T> &Schur,
		    KernelMatrix<T> &kernel);
  
  void BuildKernelsDetection(int &n0, 
			     vector<int> &singIdx, 
			     vector<vector<int> >& augkern_indexes, 
			     vector<SquareBlockMatrix<T>* >& diags, 
			     const double eps,
			     const int dim_augkern,
			     SchurMatrix<T> &Schur,
			     KernelMatrix<T> &kernel);
  Dissection::Tree** btree() { return _btree; }
  SparseMatrix<T, W, Z>* ptDA() { return _ptDA; }
  int dimension(void) { return _dim; }
  int kern_dimension(void); 
  int ComputeTransposedKernels(void);
  int get_num_threads(void) { return _num_threads; }
  int scaling(void) const { return _scaling; }
  bool verbose(void) const { return _verbose; }
  int num_threads(void) const { return _num_threads; }
  bool isScaled(void) { return (_scaling > 0 ? true : false); }
  int nsing(void) const { return _nsing; }
  void SetNsing(int nsing) { _nsing = nsing; }
  int graph_colors() const { return _graph_colors; }
  
  FILE* get_filedescriptor() { return _fp; }
  Z* addrPrecDiag() { return _precDiag; }
  DissectionQueue<T, U>** getDissectionQueue() { return _dissectionQueue; }
  TridiagQueue<T, U>** getTridiagQueue() { return _tridiagQueue; }

  vector<DissectionMatrix<T, U>* >* getDissectionMatrix() {
    return _dissectionMatrix;
  }
  TridiagBlockMatrix<T, U>** getTridiagBlockMatrix() { return _tridiagMatrix; }

  KernelMatrix<T>* getKernelMatrix() { return _kernel; }
  vector<int>* getSingVal() { return _singIdx; }
  vector<int>& getIndexIsolated() { return _index_isolated; }
  
  DissectionSolver(const DissectionSolver &s) 
  {
    _ptDA = s._ptDA;
    _btree = s._btree;
    _dissectionMatrix = s._dissectionMatrix;
    _dissectionQueue = s._dissectionQueue;
    _kernel = s._kernel;
    _singIdx = s._singIdx;
    _precDiag = s._precDiag;
    _scaling = s._scaling;
    _verbose = s._verbose;
    _num_threads = s._num_threads;
  }

  T fromreal(const U &y);

private:
  SparseMatrix<T, W, Z>*                        _ptDA;
  Dissection::Tree**                           _btree;
  vector<DissectionMatrix<T, U>* >* _dissectionMatrix;
  DissectionQueue<T, U>**            _dissectionQueue;
  TridiagBlockMatrix<T, U>**           _tridiagMatrix;
  TridiagQueue<T, U>**                  _tridiagQueue;
  SchurMatrix<T>*                              _Schur;
  KernelMatrix<T>*                            _kernel;
  vector<int>*                               _singIdx;
  Z*                                        _precDiag;
  vector<int>                         _index_isolated;
  int _scaling;
  bool _verbose;
  int _num_threads;
  bool _status_factorized;
  int _dim;
  int _graph_colors;
  int _nsing;
  int _called;
  FILE *_fp;
  bool _with_btree;
  static const T _one;  // (1.0);
  static const T _zero; // (0.0);
  static const T _none; // (-1.0);
}; // End class DissictionSolver

#endif
