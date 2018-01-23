
#include "parser.h"
#include "blockend.h"

#include "../../../config/ecf/environment.h"

#include <iostream>
#include <algorithm>

using namespace espreso;

eslocal WorkbenchParser::offset = 0;
const char* WorkbenchParser::begin = NULL;
const char* WorkbenchParser::end = NULL;

void WorkbenchParser::fillIndices(const char* header, const char* data)
{
	this->header = offset + header - begin;
	this->first = offset + data - begin;
}

void WorkbenchParser::fillIndices(const char* header, const char* first, const char* last)
{
	fillIndices(header, first);
	this->last = offset + last - begin;
}

const char* WorkbenchParser::getFirst()
{
	if (fRank <= environment->MPIrank && environment->MPIrank <= lRank) {
		if (fRank == environment->MPIrank) {
			return begin + first - offset;
		} else {
			return begin;
		}
	}
	return begin;
}

const char* WorkbenchParser::getLast()
{
	if (fRank <= environment->MPIrank && environment->MPIrank <= lRank) {
		if (lRank == environment->MPIrank) {
			return begin + last - offset;
		} else {
			return end;
		}
	}
	return begin;
}

void WorkbenchParser::fillDistribution(std::vector<BlockEnd> &blocksEnds, std::vector<eslocal> &distribution)
{
	if (last == -1) {
		last = std::lower_bound(blocksEnds.begin(), blocksEnds.end(), first, [] (BlockEnd &b, eslocal first) { return b.first < first; })->first;
	}

	fRank = std::lower_bound(distribution.begin(), distribution.end(), first + 1) - distribution.begin() - 1;
	lRank = std::lower_bound(distribution.begin(), distribution.end(), last + 1) - distribution.begin() - 1;
}

void WorkbenchParser::print(const char* data)
{
	std::cout << first << "(" << fRank << ") -> " << last << "(" << lRank << ")\n";
	if (fRank == environment->MPIrank) {
		const char *p = data + first - offset;
		while (*p != '\n') {
			std::cout << *p++;
		}
		std::cout << "\n";
	}
	if (lRank == environment->MPIrank) {
		const char *p = data + last - offset - 1;
		while (*(p - 1) != '\n') { --p; }
		while (*p != '\n') {
			std::cout << *p++;
		}
		std::cout << "\n";
	}
}


