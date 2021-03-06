
#include "wparmetis.h"
#include "parmetis.h"

#include "../../basis/logging/logging.h"
#include "../../basis/utilities/communication.h"

using namespace espreso;

eslocal ParMETIS::call(
			METHOD method,
			MPISubset &subset,
			std::vector<eslocal> &eframes, std::vector<eslocal> &eneighbors,
			std::vector<eslocal> &partition)
{
	eslocal edgecut;

	if (subset.within.size == 1) {
		std::vector<eslocal> edistribution = Communication::getDistribution<eslocal>(partition.size(), subset.across);
		edgecut = ParMETIS::call(method, subset,
					edistribution.data(), eframes.data(), eneighbors.data(),
					0, NULL, 0, NULL, NULL,
					partition.data());
	} else {
		MPIType type = MPITools::getType<eslocal>();
		std::vector<eslocal> gframes, gneighbors, gpartition;
		std::vector<size_t> offsets;

		Communication::gatherUnknownSize(eframes, gframes, subset.within);
		Communication::gatherUnknownSize(eneighbors, gneighbors, subset.within);
		Communication::gatherUnknownSize(partition, gpartition, offsets, subset.within);

		std::vector<int> disp(subset.within.size), count(subset.within.size);

		if (subset.within.rank == 0) {
			for (size_t i = 1, j = 1, offset = 0; j < gframes.size(); i++, j++) {
				if (gframes[j] == 0) {
					offset += gframes[j++ - 1];
				}
				gframes[i] = gframes[j] + offset;
			}
			gframes.resize(gframes.size() - subset.within.size + 1);

			disp = std::vector<int>(offsets.begin(), offsets.end());
			for (size_t i = 1; i < offsets.size(); i++) {
				count[i - 1] = offsets[i] - offsets[i - 1];
			}
			count.back() = gpartition.size() - offsets.back();

			std::vector<eslocal> edistribution = Communication::getDistribution<eslocal>(gpartition.size(), subset.across);

			edgecut = ParMETIS::call(method, subset,
					edistribution.data(), gframes.data(), gneighbors.data(),
					0, NULL, 0, NULL, NULL,
					gpartition.data());
		}

		MPI_Scatterv(gpartition.data(), count.data(), disp.data(), type.type, partition.data(), partition.size(), type.type, 0, subset.within.communicator);
		MPI_Bcast(&edgecut, 1, type.type, 0, subset.within.communicator);
	}

	return edgecut;
}

eslocal ParMETIS::call(
			ParMETIS::METHOD method,
			MPISubset &subset,
			eslocal *edistribution,
			eslocal *eframes, eslocal *eneighbors,
			eslocal dimensions, double *coordinates,
			eslocal verticesWeightCount, eslocal *verticesWeights, eslocal *edgeWeights,
			eslocal *partition)
{
	verticesWeightCount = std::max((eslocal)1, verticesWeightCount);

	eslocal wgtflag = 0;
	eslocal numflag = 0;
	eslocal parts = subset.origin.size;
	std::vector<double> partFraction(verticesWeightCount * parts, 1.0 / parts);
	std::vector<double> unbalanceTolerance(verticesWeightCount, 1.02);
	eslocal options[4] = { 0, 0, 0, PARMETIS_PSR_UNCOUPLED };
	double itr = 1e6;
	eslocal edgecut;

	if (verticesWeights != NULL) {
		wgtflag += 2;
	}
	if (edgeWeights != NULL) {
		wgtflag += 1;
	}

	switch (method) {

	case ParMETIS::METHOD::ParMETIS_V3_PartKway:
		if (coordinates != NULL) {
			if (METIS_OK != ParMETIS_V3_PartGeomKway(
					edistribution,
					eframes, eneighbors,
					verticesWeights, edgeWeights,
					&wgtflag, &numflag, &dimensions, coordinates, &verticesWeightCount,
					&parts, partFraction.data(), unbalanceTolerance.data(),
					options,
					&edgecut, partition,
					&subset.across.communicator)) {

				ESINFO(ERROR) << "PARMETIS_ERROR while partitiate mesh to MPI processes by KWay utilizing coordinates.";
			}
		} else {
			if (METIS_OK != ParMETIS_V3_PartKway(
					edistribution,
					eframes, eneighbors,
					verticesWeights, edgeWeights,
					&wgtflag, &numflag, &verticesWeightCount,
					&parts, partFraction.data(), unbalanceTolerance.data(),
					options,
					&edgecut, partition,
					&subset.across.communicator)) {

				ESINFO(ERROR) << "PARMETIS_ERROR while partitiate mesh to MPI processes by KWay utilizing coordinates.";
			}
		}
		break;

	case ParMETIS::METHOD::ParMETIS_V3_RefineKway:
		if (METIS_OK != ParMETIS_V3_RefineKway(
				edistribution,
				eframes, eneighbors,
				verticesWeights, edgeWeights,
				&wgtflag, &numflag, &verticesWeightCount,
				&parts, partFraction.data(), unbalanceTolerance.data(),
				options,
				&edgecut, partition,
				&subset.across.communicator)) {

			ESINFO(ERROR) << "PARMETIS_ERROR while refine mesh partition to MPI processes by KWay.";
		}
		break;

	case ParMETIS::METHOD::ParMETIS_V3_AdaptiveRepart:
		if (METIS_OK != ParMETIS_V3_AdaptiveRepart(
				edistribution,
				eframes, eneighbors,
				verticesWeights, NULL, edgeWeights,
				&wgtflag, &numflag, &verticesWeightCount,
				&parts, partFraction.data(), unbalanceTolerance.data(), &itr,
				options,
				&edgecut, partition,
				&subset.across.communicator)) {

			ESINFO(ERROR) << "PARMETIS_ERROR while adaptive repartition mesh to MPI processes by KWay.";
		}
		break;

	case ParMETIS::METHOD::ParMETIS_V3_PartGeom:
		if (coordinates == NULL) {
			ESINFO(ERROR) << "PARMETIS_ERROR:: PartGeom needs coordinates.";
		}
		if (METIS_OK != ParMETIS_V3_PartGeom(
				edistribution,
				&dimensions, coordinates,
				partition,
				&subset.across.communicator)) {

			ESINFO(ERROR) << "PARMETIS_ERROR while refine mesh partition to MPI processes by KWay.";
		}

	}

	return edgecut;
}


