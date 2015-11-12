
#include "temperature.h"

namespace assembler {

template <class TInput>
void Temperature<TInput>::inertia(std::vector<double> &inertia)
{
	inertia.resize(1, 0);
}

template <class TInput>
void Temperature<TInput>::C(DenseMatrix &C)
{
	C.resize(3, 3);
	C(0, 0) = C(1, 1) = C(2, 2) = 1;
}

}