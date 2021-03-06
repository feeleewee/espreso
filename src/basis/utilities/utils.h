
#ifndef BASIS_UTILITIES_UTILS_H_
#define BASIS_UTILITIES_UTILS_H_

#include <stdlib.h>
#include <vector>
#include <string>
#include <ostream>

namespace espreso {

struct Esutils
{

	template<typename Ttype>
	static Ttype getEnv(const std::string &name);

	template<typename Ttype>
	static void setFromEnv(Ttype &value, const std::string &name);

	template<typename Ttype>
	static Ttype sizesToOffsets(std::vector<Ttype> &sizes);

	template<typename Ttype, typename Tpermutation>
	static void permute(std::vector<Ttype> &data, const std::vector<Tpermutation> &permutation, size_t elementsize = 1);

	template<typename Ttype>
	static void threadDistributionToFullDistribution(std::vector<std::vector<Ttype> > &distribution);

	template<typename Ttype>
	static void threadDistributionToFullDistribution(std::vector<Ttype> &data, const std::vector<size_t> &distribution);

	template<typename Ttype>
	static void removeDuplicity(std::vector<Ttype> &data, size_t begin = 0);

	template<typename Ttype>
	static void sortAndRemoveDuplicity(std::vector<Ttype> &data, size_t begin = 0);

	template<typename Ttype>
	static void sortAndRemoveDuplicity(std::vector<std::vector<Ttype> > &data);

	template<typename Ttype>
	static void mergeThreadedUniqueData(std::vector<std::vector<Ttype> > &data);

	template<typename Ttype>
	static void mergeThreadedUniqueData(std::vector<std::vector<std::vector<Ttype> > > &data);

	template<typename Ttype>
	static void inplaceMerge(std::vector<Ttype> &data, const std::vector<size_t> &distribution);

	template<typename Ttype>
	static void inplaceMerge(std::vector<std::vector<Ttype> > &data);

	template<typename Ttype>
	static void sortWithInplaceMerge(std::vector<Ttype> &data, const std::vector<size_t> &distribution);

	template<typename Ttype>
	static void sortWithInplaceMerge(std::vector<std::vector<Ttype> > &data);

	template<typename Ttype>
	static void sortWithUniqueMerge(std::vector<std::vector<Ttype> > &data);

	template<typename Ttype>
	static void mergeAppendedData(std::vector<Ttype> &data, const std::vector<size_t> &distribution);

	template<typename Ttype>
	static typename std::vector<Ttype>::const_iterator max_element(const std::vector<Ttype> &elements);

	static std::string createDirectory(const std::vector<std::string> &path);

	template<typename Ttype>
	static size_t packedSize(const Ttype &data);

	template<typename Ttype>
	static size_t packedSize(const std::vector<Ttype> &data);

	template<typename Ttype>
	static void pack(const Ttype &data, char* &p);

	template<typename Ttype>
	static void pack(const std::vector<Ttype> &data, char* &p);

	template<typename Ttype>
	static void unpack(Ttype &data, const char* &p);

	template<typename Ttype>
	static void unpack(std::vector<Ttype> &data, const char* &p);
};


template<typename T1, typename T2>
std::ostream& operator<< (std::ostream& os, const std::pair<T1, T2> &v)
{
	os << "<" << v.first << ":" << v.second << ">";
	return os;
}

template<typename T>
std::ostream& operator<< (std::ostream& os, const std::vector<T> &v)
{
	for(size_t i = 0; i < v.size(); ++i) {
		os << v[i] << " ";
	}
	os << "\n";
	return os;
}

}

#include "utils.hpp"


#endif /* BASIS_UTILITIES_UTILS_H_ */
