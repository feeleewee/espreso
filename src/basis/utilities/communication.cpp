
#include "communication.h"

#include "../../mesh/store/statisticsstore.h"

namespace espreso {

template<typename Ttype>
static void _sizeToOffsets(void *in, void *out, int *len, MPI_Datatype *datatype)
{
	*(static_cast<Ttype*>(out)) += *(static_cast<Ttype*>(in));
}

static void _mergeStatistics(void *in, void *out, int *len, MPI_Datatype *datatype)
{
	int size = *len / sizeof(Statistics);
	for (int i = 0; i < size; i++) {
		(static_cast<Statistics*>(out) + i)->min = std::min((static_cast<Statistics*>(out) + i)->min, (static_cast<Statistics*>(in) + i)->min);
		(static_cast<Statistics*>(out) + i)->max = std::max((static_cast<Statistics*>(out) + i)->max, (static_cast<Statistics*>(in) + i)->max);
		(static_cast<Statistics*>(out) + i)->avg += (static_cast<Statistics*>(in) + i)->avg;
		(static_cast<Statistics*>(out) + i)->norm += (static_cast<Statistics*>(in) + i)->norm;
	}
}

template<typename Ttype>
static void _max(void *in, void *out, int *len, MPI_Datatype *datatype)
{
	*(static_cast<Ttype*>(out)) = std::max(*(static_cast<Ttype*>(in)), *(static_cast<Ttype*>(out)));
}

template<typename Ttype>
static void _min(void *in, void *out, int *len, MPI_Datatype *datatype)
{
	*(static_cast<Ttype*>(out)) = std::min(*(static_cast<Ttype*>(in)), *(static_cast<Ttype*>(out)));
}

template<typename Ttype>
static void _sum(void *in, void *out, int *len, MPI_Datatype *datatype)
{
	*(static_cast<Ttype*>(out)) += *(static_cast<Ttype*>(in));
}

MPITools::Operations::Operations()
{
	MPI_Op_create(_sizeToOffsets<eslocal>, 1, &sizeToOffsetsEslocal);
	MPI_Op_create(_sizeToOffsets<size_t>, 1, &sizeToOffsetsSize_t);
	MPI_Op_create(_mergeStatistics, 1, &mergeStatistics);
	MPI_Op_create(_max<eslocal>, 1, &max);
	MPI_Op_create(_min<eslocal>, 1, &min);
	MPI_Op_create(_sum<eslocal>, 1, &sum);
}

}

