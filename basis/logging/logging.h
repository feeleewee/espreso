
#ifndef BASIS_LOGGING_LOGGING_H_
#define BASIS_LOGGING_LOGGING_H_

#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sys/sysinfo.h>

#include "esconfig.h"
#include "timeeval.h"

#define ESTEST(EVENT) if (!eslog::Test::report(EVENT))    ; else eslog::Test(EVENT).get()
#define ESINFO(EVENT) if (!eslog::Info::report(EVENT))    ; else eslog::Info(EVENT).get()
#define ESTIME(EVENT) if (!eslog::Measure::report(EVENT)) ; else eslog::Measure(EVENT).get()

namespace eslog {

enum ESPRESOTest {
	FAILED,
	PASSED
};

enum TestEvent {
	MANDATORY,
	TEST_LEVEL0,

	SIMPLE,
	TEST_LEVEL1,

	EXPENSIVE,
	TEST_LEVEL2,

	PEDANTIC,
	TEST_LEVEL3,
};

enum InfoEvent {
	ERROR,
	VERBOSE_LEVEL0,

	BASIC,
	VERBOSE_LEVEL1,

	DETAILED,
	VERBOSE_LEVEL2,

	EXHAUSTIVE,
	VERBOSE_LEVEL3
};

enum MeasureEvent {
	MEASURE_LEVEL0,

	SUMMARY,
	CHECKPOINT1,
	MEASURE_LEVEL1,

	CHECKPOINT2,
	MEASURE_LEVEL2,

	CHECKPOINT3,
	MEASURE_LEVEL3
};

class Test
{
public:
	Test& operator<<(const ESPRESOTest &test)
	{
		if (test == FAILED) { error = true; }
		return *this;
	}
	template<typename Ttype>
	Test& operator<<(const Ttype &value)
	{
		os << value;
		return *this;
	}

	Test(TestEvent event): error(false) {};
	~Test();

	Test& get() { return *this; };

	static bool report(TestEvent event) {
		switch (esconfig::info::testingLevel) {
		case 0: return event < TEST_LEVEL0;
		case 1: return event < TEST_LEVEL1;
		case 2: return event < TEST_LEVEL2;
		case 3: return event < TEST_LEVEL3;
		default : return true;
		}
	};

protected:
	std::ostringstream os;
	bool error;
};

class Info
{
public:
	Info(InfoEvent event): event(event) {};
	~Info();

	std::ostringstream& get() { return os; };

	static bool report(InfoEvent event) {
		switch (esconfig::info::verboseLevel) {
		case 0: return event < VERBOSE_LEVEL0;
		case 1: return event < VERBOSE_LEVEL1;
		case 2: return event < VERBOSE_LEVEL2;
		case 3: return event < VERBOSE_LEVEL3;
		default : return true;
		}
	};

protected:
	std::ostringstream os;
	InfoEvent event;
};


class Measure
{
public:
	static double time()
	{
		return omp_get_wtime();
	}

	Measure(MeasureEvent event);
	~Measure();

	static double processMemory();
	static double globalMemory();
	static double availableMemory();

	std::ostringstream& get() { return os; };

	static bool report(MeasureEvent event) {
		switch (esconfig::info::measureLevel) {
		case 0: return event < MEASURE_LEVEL0;
		case 1: return event < MEASURE_LEVEL1;
		case 2: return event < MEASURE_LEVEL2;
		case 3: return event < MEASURE_LEVEL3;
		default : return true;
		}
	};

protected:
	static std::vector<Checkpoint> checkpoints;

	void evaluateCheckpoints();

	std::ostringstream os;
	MeasureEvent event;
};

class Logging {

public:
	static std::string prepareFile(const std::string &name)
	{
		std::stringstream dir, file, mkdir;

		dir << esconfig::info::output << "/" << esconfig::MPIrank << "/";
		file << dir.str() << "/" << name << ".txt";

		mkdir << "mkdir -p " << dir.str();
		system(mkdir.str().c_str());

		return file.str();
	}

	static std::string prepareFile(size_t subdomain, const std::string &name)
	{
		std::stringstream ss;
		ss << name << subdomain;
		return prepareFile(ss.str());
	}
};
}


#endif /* BASIS_LOGGING_LOGGING_H_ */
