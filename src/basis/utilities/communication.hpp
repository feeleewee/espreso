
#include "communication.h"

#include <algorithm>

namespace espreso {

template <typename Ttype>
bool Communication::exchangeKnownSize(const std::vector<std::vector<Ttype> > &sBuffer, std::vector<std::vector<Ttype> > &rBuffer, const std::vector<int> &neighbours)
{
	std::vector<MPI_Request> req(2 * neighbours.size());
	for (size_t n = 0; n < neighbours.size(); n++) {
		// bullxmpi violate MPI standard (cast away constness)
		MPI_Isend(const_cast<Ttype*>(sBuffer[n].data()), sizeof(Ttype) * sBuffer[n].size(), MPI_BYTE, neighbours[n], 0, environment->MPICommunicator, req.data() + 2 * n);
	}

	for (size_t n = 0; n < neighbours.size(); n++) {
		// bullxmpi violate MPI standard (cast away constness)
		MPI_Irecv(const_cast<Ttype*>(rBuffer[n].data()), sizeof(Ttype) * rBuffer[n].size(), MPI_BYTE, neighbours[n], 0, environment->MPICommunicator, req.data() + 2 * n + 1);
	}

	MPI_Waitall(2 * neighbours.size(), req.data(), MPI_STATUSES_IGNORE);
	return true;
}


template <typename Ttype>
bool Communication::exchangeUnknownSize(const std::vector<std::vector<Ttype> > &sBuffer, std::vector<std::vector<Ttype> > &rBuffer, const std::vector<int> &neighbours)
{
	auto n2i = [ & ] (size_t neighbour) {
		return std::lower_bound(neighbours.begin(), neighbours.end(), neighbour) - neighbours.begin();
	};

	std::vector<MPI_Request> req(neighbours.size());
	for (size_t n = 0; n < neighbours.size(); n++) {
		// bullxmpi violate MPI standard (cast away constness)
		MPI_Isend(const_cast<Ttype*>(sBuffer[n].data()), sizeof(Ttype) * sBuffer[n].size(), MPI_BYTE, neighbours[n], 0, environment->MPICommunicator, req.data() + n);
	}

	int flag;
	size_t counter = 0;
	MPI_Status status;
	while (counter < neighbours.size()) {
		MPI_Iprobe(MPI_ANY_SOURCE, 0, environment->MPICommunicator, &flag, &status);
		if (flag) {
			int count;
			MPI_Get_count(&status, MPI_BYTE, &count);
			rBuffer[n2i(status.MPI_SOURCE)].resize(count / sizeof(Ttype));
			MPI_Recv(rBuffer[n2i(status.MPI_SOURCE)].data(), count, MPI_BYTE, status.MPI_SOURCE, 0, environment->MPICommunicator, MPI_STATUS_IGNORE);
			counter++;
		}
	}

	MPI_Waitall(neighbours.size(), req.data(), MPI_STATUSES_IGNORE);
	MPI_Barrier(environment->MPICommunicator); // MPI_Iprobe(ANY_SOURCE) can be problem when calling this function more times
	return true;
}

template <typename Ttype>
bool Communication::exchangeUnknownSize(const std::vector<Ttype> &sBuffer, std::vector<std::vector<Ttype> > &rBuffer, const std::vector<int> &neighbours)
{
	auto n2i = [ & ] (size_t neighbour) {
		return std::lower_bound(neighbours.begin(), neighbours.end(), neighbour) - neighbours.begin();
	};

	std::vector<MPI_Request> req(neighbours.size());
	for (size_t n = 0; n < neighbours.size(); n++) {
		// bullxmpi violate MPI standard (cast away constness)
		MPI_Isend(const_cast<Ttype*>(sBuffer.data()), sizeof(Ttype) * sBuffer.size(), MPI_BYTE, neighbours[n], 0, environment->MPICommunicator, req.data() + n);
	}

	int flag;
	size_t counter = 0;
	MPI_Status status;
	while (counter < neighbours.size()) {
		MPI_Iprobe(MPI_ANY_SOURCE, 0, environment->MPICommunicator, &flag, &status);
		if (flag) {
			int count;
			MPI_Get_count(&status, MPI_BYTE, &count);
			rBuffer[n2i(status.MPI_SOURCE)].resize(count / sizeof(Ttype));
			MPI_Recv(rBuffer[n2i(status.MPI_SOURCE)].data(), count, MPI_BYTE, status.MPI_SOURCE, 0, environment->MPICommunicator, MPI_STATUS_IGNORE);
			counter++;
		}
	}

	MPI_Waitall(neighbours.size(), req.data(), MPI_STATUSES_IGNORE);
	MPI_Barrier(environment->MPICommunicator); // MPI_Iprobe(ANY_SOURCE) can be problem when calling this function more times
	return true;
}

template <typename Ttype>
bool Communication::receiveLowerKnownSize(const std::vector<std::vector<Ttype> > &sBuffer, std::vector<std::vector<Ttype> > &rBuffer, const std::vector<int> &neighbours)
{
	std::vector<MPI_Request> req(neighbours.size());
	for (size_t n = 0; n < neighbours.size(); n++) {
		if (neighbours[n] > environment->MPIrank) {
			// bullxmpi violate MPI standard (cast away constness)
			MPI_Isend(const_cast<Ttype*>(sBuffer[n].data()), sizeof(Ttype) * sBuffer[n].size(), MPI_BYTE, neighbours[n], 1, environment->MPICommunicator, req.data() + n);
		}
		if (neighbours[n] < environment->MPIrank) {
			MPI_Irecv(rBuffer[n].data(), sizeof(Ttype) * rBuffer[n].size(), MPI_BYTE, neighbours[n], 1, environment->MPICommunicator, req.data() + n);
		}
	}

	MPI_Waitall(neighbours.size(), req.data(), MPI_STATUSES_IGNORE);
	return true;
}

template <typename Ttype>
bool Communication::receiveUpperKnownSize(const std::vector<std::vector<Ttype> > &sBuffer, std::vector<std::vector<Ttype> > &rBuffer, const std::vector<int> &neighbours)
{
	std::vector<MPI_Request> req(neighbours.size());
	for (size_t n = 0; n < neighbours.size(); n++) {
		if (neighbours[n] < environment->MPIrank) {
			// bullxmpi violate MPI standard (cast away constness)
			MPI_Isend(const_cast<Ttype*>(sBuffer[n].data()), sizeof(Ttype) * sBuffer[n].size(), MPI_BYTE, neighbours[n], 1, environment->MPICommunicator, req.data() + n);
		}
		if (neighbours[n] > environment->MPIrank) {
			MPI_Irecv(rBuffer[n].data(), sizeof(Ttype) * rBuffer[n].size(), MPI_BYTE, neighbours[n], 1, environment->MPICommunicator, req.data() + n);
		}
	}

	MPI_Waitall(neighbours.size(), req.data(), MPI_STATUSES_IGNORE);
	return true;
}

template <typename Ttype>
bool Communication::receiveUpperUnknownSize(const std::vector<std::vector<Ttype> > &sBuffer, std::vector<std::vector<Ttype> > &rBuffer, const std::vector<int> &neighbours)
{
	auto n2i = [ & ] (size_t neighbour) {
		return std::lower_bound(neighbours.begin(), neighbours.end(), neighbour) - neighbours.begin();
	};

	size_t rSize = 0;
	std::vector<MPI_Request> req(neighbours.size());
	for (size_t n = 0; n < neighbours.size() && neighbours[n] < environment->MPIrank; n++) {
		// bullxmpi violate MPI standard (cast away constness)
		MPI_Isend(const_cast<Ttype*>(sBuffer[n].data()), sizeof(Ttype) * sBuffer[n].size(), MPI_BYTE, neighbours[n], 0, environment->MPICommunicator, req.data() + rSize++);
	}

	int flag;
	size_t counter = std::lower_bound(neighbours.begin(), neighbours.end(), environment->MPIrank) - neighbours.begin();
	MPI_Status status;
	while (counter < neighbours.size()) {
		MPI_Iprobe(MPI_ANY_SOURCE, 0, environment->MPICommunicator, &flag, &status);
		if (flag) {
			int count;
			MPI_Get_count(&status, MPI_BYTE, &count);
			rBuffer[n2i(status.MPI_SOURCE)].resize(count / sizeof(Ttype));
			MPI_Recv(rBuffer[n2i(status.MPI_SOURCE)].data(), count, MPI_BYTE, status.MPI_SOURCE, 0, environment->MPICommunicator, MPI_STATUS_IGNORE);
			counter++;
		}
	}

	MPI_Waitall(rSize, req.data(), MPI_STATUSES_IGNORE);
	MPI_Barrier(environment->MPICommunicator); // MPI_Iprobe(ANY_SOURCE) can be problem when calling this function more times
	return true;
}

template <typename Ttype>
bool Communication::gatherUnknownSize(const std::vector<Ttype> &sBuffer, std::vector<Ttype> &rBuffer)
{
	std::vector<size_t> offsets;
	return gatherUnknownSize(sBuffer, rBuffer, offsets);
}

template <typename Ttype>
bool Communication::gatherUnknownSize(const std::vector<Ttype> &sBuffer, std::vector<Ttype> &rBuffer, std::vector<size_t> &offsets)
{
	int size = sizeof(Ttype) * sBuffer.size();
	std::vector<int> rSizes(environment->MPIsize), rOffsets(environment->MPIsize);
	MPI_Gather(&size, sizeof(int), MPI_BYTE, rSizes.data(), sizeof(int), MPI_BYTE, 0, environment->MPICommunicator);

	if (!environment->MPIrank) {
		size = 0;
		for (size_t i = 0; i < rSizes.size(); i++) {
			rOffsets[i] = size;
			size += rSizes[i];
		}
		rBuffer.resize(size / sizeof(Ttype));
	}

	// bullxmpi violate MPI standard (cast away constness)
	MPI_Gatherv(const_cast<Ttype*>(sBuffer.data()), sBuffer.size() * sizeof(Ttype), MPI_BYTE, rBuffer.data(), rSizes.data(), rOffsets.data(), MPI_BYTE, 0, environment->MPICommunicator);

	offsets.resize(environment->MPIsize);
	for (size_t i = 0; i < rOffsets.size(); i++) {
		offsets[i] = rOffsets[i] / sizeof(Ttype);
	}
	return true;
}

template <typename Ttype>
bool Communication::broadcastUnknownSize(std::vector<Ttype> &buffer)
{
	int size = buffer.size();
	MPI_Bcast(&size, sizeof(int), MPI_BYTE, 0, environment->MPICommunicator);
	buffer.resize(size);
	MPI_Bcast(buffer.data(), sizeof(Ttype) * size, MPI_BYTE, 0, environment->MPICommunicator);
	return true;
}

template <typename Ttype>
Ttype Communication::exscan(Ttype &value, MPI_Op &operation)
{
	Ttype size = value;
	if (environment->MPIsize == 1) {
		value = 0;
		return size;
	}

	MPI_Exscan(&size, &value, sizeof(Ttype), MPI_BYTE, operation, environment->MPICommunicator);

	size = value + size;
	MPI_Bcast(&size, sizeof(Ttype), MPI_BYTE, environment->MPIsize - 1, environment->MPICommunicator);
	if (environment->MPIrank == 0) {
		value = 0;
	}
	MPI_Barrier(environment->MPICommunicator);

	return size;
}

template <typename Ttype>
bool Communication::sendVariousTargets(const std::vector<std::vector<Ttype> > &sBuffer, std::vector<std::vector<Ttype> > &rBuffer, const std::vector<int> &targets, std::vector<int> &sources)
{
	std::vector<int> smsgcounter(environment->MPIsize);
	std::vector<int> rmsgcounter(environment->MPIsize);
	for (size_t n = 0; n < targets.size(); n++) {
		smsgcounter[targets[n]] = 1;
	}

	MPI_Allreduce(smsgcounter.data(), rmsgcounter.data(), environment->MPIsize, MPI_INT, MPI_SUM, environment->MPICommunicator);

	std::vector<MPI_Request> req(targets.size());
	for (size_t t = 0; t < targets.size(); t++) {
		MPI_Isend(const_cast<Ttype*>(sBuffer[t].data()), sizeof(Ttype) * sBuffer[t].size(), MPI_BYTE, targets[t], 0, environment->MPICommunicator, req.data() + t);
	}

	int flag;
	int counter = 0;
	MPI_Status status;
	sources.clear();
	std::vector<std::vector<Ttype> > tmpBuffer;
	tmpBuffer.reserve(rmsgcounter[environment->MPIrank]);
	while (counter < rmsgcounter[environment->MPIrank]) {
		MPI_Iprobe(MPI_ANY_SOURCE, 0, environment->MPICommunicator, &flag, &status);
		if (flag) {
			int count;
			MPI_Get_count(&status, MPI_BYTE, &count);
			tmpBuffer.push_back(std::vector<Ttype>(count / sizeof(Ttype)));
			MPI_Recv(tmpBuffer.back().data(), count, MPI_BYTE, status.MPI_SOURCE, 0, environment->MPICommunicator, MPI_STATUS_IGNORE);
			sources.push_back(status.MPI_SOURCE);
			counter++;
		}
	}

	std::vector<int> permutation(sources.size());
	for (size_t i = 0; i < sources.size(); i++) {
		permutation[i] = i;
	}
	std::sort(permutation.begin(), permutation.end(), [&] (int i, int j) { return sources[i] < sources[j]; });
	rBuffer.resize(tmpBuffer.size());
	for (size_t i = 0; i < permutation.size(); i++) {
		rBuffer[i].swap(tmpBuffer[permutation[i]]);
	}

	std::sort(sources.begin(), sources.end());

	MPI_Waitall(targets.size(), req.data(), MPI_STATUSES_IGNORE);
	MPI_Barrier(environment->MPICommunicator); // MPI_Iprobe(ANY_SOURCE) can be problem when calling this function more times
	return true;
}

inline void Communication::serialize(std::function<void(void)> fnc)
{
	for (int r = 0; r < environment->MPIsize; ++r) {
		if (r == environment->MPIrank) {
			fnc();
		}
		MPI_Barrier(environment->MPICommunicator);
	}
	MPI_Barrier(environment->MPICommunicator);
}

}



