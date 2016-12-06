
#ifndef SRC_CONFIG_READER_H_
#define SRC_CONFIG_READER_H_

#include "tokenizer.h"

namespace espreso {

struct GlobalConfiguration;
struct Configuration;

class Reader {

public:
	static void read(Configuration &configuration, const std::string &file) { _read(configuration, file, {}); }
	static void read(Configuration &configuration, int* argc, char ***argv) { _read(configuration, argc, argv); }

	static void set(const GlobalConfiguration &configuration);

	static void print(const Configuration &configuration);
	static void store(const Configuration &configuration);

private:
	static void _read(Configuration &configuration, const std::string &file, const std::vector<std::string> &args);
	static void _read(Configuration &configuration, int* argc, char ***argv);
};

}



#endif /* SRC_CONFIG_READER_H_ */
