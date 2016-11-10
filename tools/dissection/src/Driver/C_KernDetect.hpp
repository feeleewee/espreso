/*! \file   C_KernDetect.hpp
    \brief  Kernel detection algorithm : symm <= DOI: 10.1002/nme.4729 / unsymm
    \author Atsushi Suzuki, Laboratoire Jacques-Louis Lions
    \date   Jun. 20th 2014
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

#ifndef _DRIVER_C_KERNDETECT
#define _DRIVER_C_KERNDETECT

#include "Compiler/arithmetic.hpp"
#include "Compiler/blas.hpp"
#include "Driver/C_BlasRoutines.hpp"

template<typename T, typename U>
void copy_matrix_permute_(const int lda, int n,
			  U *b, T *a, int *permute);

template<typename T, typename U>
void copy_matrix_permute_complex_(const int lda, int n,
				  complex<U> *b, complex<T> *a, int *permute);
 

template<typename T, typename U>
bool check_kern(const int n0, const int lda, const int n, T *a_ini,
		int *permute,
		const int dim_augkern, const U &eps,
		const U &eps_param, const bool flag_sym, U *errors);

//template<typename T, typename U, typename W, typename Y = W>
template<typename T, typename U, typename W, typename Y>
bool check_kern_(const int n0, const int lda, const int n, T *a_ini,
		int *permute,
		const int dim_augkern, const U &eps,
		const U &eps_param, const bool flag_sym, U *errors);

template<typename T, typename U>
U check_matrixerr(const int lda, const int n,
		  T *a,
		  const int dim_augkern, const int k,
		  int *permute,
		  const U &eps,
		  const bool flag_sym);

template<typename T, typename U, typename W>
U check_matrixerr_(const int lda, const int n,
		   T *a,
		   const int dim_augkern, const int k,
		   int *permute,
		   const U &eps,
		   const bool flag_sym);

template<typename T, typename U>
void verify_kernels(const int n0, const int n, const T *a_ini,
		    const bool isSym, const double eps, U *errors);

template<typename T, typename U>
int hqr_pivot(const int n, T *a, int *permute);

template<typename T>
void HouseholderVector(int n, T *x, T *v, T *gamma);

template<typename T, typename U>
void HouseholderVector_complex(int n, T *x, T *v, T *gamma);

template<typename T>
void HouseholderReflection(int n, T *a, int lda, T *v, T *w, const T &gamma);

template<typename T, typename U>
bool ComputeDimKernel(int *n0, bool *flag_2x2, const T *a_, const int n, 
		      const bool sym_flag,
		      const int dim_augkern,
		      const U eps_machine,
		      const double eps_piv,
		      const bool verbose,
		      FILE *fp);

template<typename T, typename U>
bool VerifyDimKernel(int *nn0_,
		     int n_dim, T* a_fact,
		     vector<int>& kernel_dim,
		     const bool sym_flag,
		     const int dim_augkern,
		     const U eps_machine,
		     const bool verbose,
		     FILE *fp);

#endif
