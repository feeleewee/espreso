
#include "timeeval.h"

using namespace espreso;

TimeEvent::TimeEvent(std::string name)
{
	eventName   = name;
	name_length = 50;
	val_length  = 12;
	reset();
}

void TimeEvent::reset() {
	eventCount = 0;
	eventTime.clear();

	avgTime = 0.0;
	sumTime = 0.0;
	minTime = 0.0;
	maxTime = 0.0;
	stdDev  = 0.0;

	g_avgTime = 0.0;
	g_minTime = 0.0;
	g_maxTime = 0.0;
	g_stdDev  = 0.0;
	g_sumTime = 0.0;
}

void TimeEvent::start(double time) {
#ifdef TM_BLOCK_START
	startWithBarrier(time);
#else
	startWithoutBarrier(time);
#endif
}

void TimeEvent::startWithBarrier(double time) {
	MPI_Barrier(MPI_COMM_WORLD);
	startWithoutBarrier(time);
}

void TimeEvent::startWithoutBarrier(double time) {
	eventTime.push_back(time);
}

void TimeEvent::start() {
#ifdef TM_BLOCK_START
	startWithBarrier();
#else
	startWithoutBarrier();
#endif
}

void TimeEvent::startWithBarrier() {
	MPI_Barrier(MPI_COMM_WORLD);
	startWithoutBarrier();
}

void TimeEvent::startWithoutBarrier() {
	eventTime.push_back(time());
}


void TimeEvent::end() {
#ifdef TM_BLOCK_END
	endWithBarrier();
#else
	endWithoutBarrier();
#endif
}

void TimeEvent::endWithBarrier() {
	MPI_Barrier(MPI_COMM_WORLD);
	endWithoutBarrier();
}

void TimeEvent::endWithoutBarrier() {
	eventTime.back() = time() - eventTime.back();
	eventCount++;
}

void TimeEvent::end(double time) {
#ifdef TM_BLOCK_END
	endWithBarrier(time);
#else
	endWithoutBarrier(time);
#endif
}

void TimeEvent::endWithBarrier(double time) {
	MPI_Barrier(MPI_COMM_WORLD);
	endWithoutBarrier(time);
}

void TimeEvent::endWithoutBarrier(double time) {
	eventTime.back() = time - eventTime.back();
	eventCount++;
}


void TimeEvent::evaluate() {
	sumTime = 0;
	avgTime = 0;
	minTime = eventCount ? eventTime[0] : 0;
	maxTime = 0;
	stdDev  = 0;

	for (eslocal i = 0; i < eventCount; i++) {
		double d_time = eventTime[i];
		sumTime += d_time;
		if (d_time < minTime) {
			minTime = d_time;
		}
		if (d_time > maxTime) {
			maxTime = d_time;
		}
	}

	avgTime = sumTime / eventCount;

	double E = 0;
	for (eslocal i = 0; i < eventCount; i++) {
		E += (eventTime[i] - avgTime) * (eventTime[i] - avgTime);
	}

	if (eventCount * E) {
		stdDev = sqrt(1 / eventCount * E);
	} else {
		stdDev = 0;
	}
}

void TimeEvent::evaluateMPI() {
	evaluate();

	MPI_Reduce(&avgTime, &g_avgTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	g_avgTime = g_avgTime / config::env::MPIsize;

	MPI_Reduce(&sumTime, &g_sumTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	g_sumTime = g_sumTime / config::env::MPIsize;

	MPI_Reduce(&minTime, &g_minTime, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Reduce(&maxTime, &g_maxTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
}


void TimeEvent::printStat(double totalTime) {
	evaluate();

	ESLOG(SUMMARY)
		<< std::setw(name_length) << std::left << eventName
		<< " avg.: " << std::fixed << std::setw(val_length) << avgTime
		<< " min.: " << std::setw(val_length) << minTime
		<< " max.: " << std::setw(val_length) << maxTime
		<< " % of avg tot: " << std::setw(val_length)
		<< (totalTime != 0 ? 100.0 * avgTime / totalTime : INFINITY);
}


void TimeEvent::printLastStat(double totalTime) {
	avgTime = eventTime.back();

	ESLOG(SUMMARY)
		<< std::setw(name_length) << std::left << eventName
		<< " avg.: " << std::fixed << std::setw(val_length) << avgTime
		<< " min.: " << std::setw(val_length) << " -- "
		<< " max.: " << std::setw(val_length) << " -- "
		<< " % of avg tot: " << std::setw(val_length)
		<< (totalTime != 0 ? 100.0 * avgTime / totalTime : INFINITY);
}

double TimeEvent::getLastStat(double totalTime) {
	return eventTime.back();
}


void TimeEvent::printStatMPI(double totalTime) {
	evaluateMPI();

	ESLOG(SUMMARY)
		<< std::setw(name_length) << std::left << eventName
		<< " avg.: " << std::setw(val_length) << std::fixed << g_avgTime
		<< " min.: " << std::setw(val_length) << g_minTime
		<< " max.: " << std::setw(val_length) << g_maxTime
		<< " sum.: " << std::setw(val_length) << g_sumTime
		<< " count: " << std::setw(val_length) << eventCount
		<< " % of avg tot: " << std::setw(val_length)
		<< (totalTime != 0 ? 100.0 * g_avgTime / totalTime : INFINITY);
}


void TimeEvent::printLastStatMPI(double totalTime) {
	if (config::info::measureLevel == 0) {
		return;
	}
	double d_time = eventTime.back();

	MPI_Reduce(&d_time, &g_avgTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	g_avgTime = g_avgTime / config::env::MPIsize;

	MPI_Reduce(&d_time, &g_minTime, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Reduce(&d_time, &g_maxTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

	ESLOG(SUMMARY)
		<< std::setw(name_length) << std::left << eventName
		<< " last: " << std::fixed << std::setw(val_length) << g_avgTime
		<< " min.: " << std::setw(val_length) << g_minTime
		<< " max.: " << std::setw(val_length) << g_maxTime
		<< " % of avg tot: " << std::setw(val_length)
		<< (totalTime != 0 ? 100.0 * g_avgTime / totalTime : INFINITY);
}


void TimeEvent::printLastStatMPIPerNode(double totalTime)
{
	double d_time = eventTime.back();
	std::vector<double> d_all_times;

	if(config::env::MPIrank == 0) {
		d_all_times.resize(config::env::MPIsize);
	} else {
		d_all_times.resize(1);
	}

	MPI_Gather(&d_time, 1, MPI_DOUBLE, &d_all_times[0], 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	MPI_Reduce(&d_time, &g_avgTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	g_avgTime= g_avgTime / config::env::MPIsize;

	MPI_Reduce(&d_time, &g_minTime, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Reduce(&d_time, &g_maxTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

	ESLOG(SUMMARY)
		<< std::setw(name_length) << std::left << eventName
		<< " last: " << std::fixed << std::setw(val_length) << g_avgTime
		<< " min.: " << std::setw(val_length) << g_minTime
		<< " max.: " << std::setw(val_length) << g_maxTime
		<< " % of avg tot: " << std::setw(val_length)
		<< (totalTime != 0 ? 100.0 * g_avgTime / totalTime : INFINITY);

	std::stringstream ss;
	for (eslocal i = 0; i < config::env::MPIsize; i++) {
		ss << std::fixed << std::setw(3) << "R: " << std::setw(5) << i << std::setw(15) << d_all_times[i];

		if ((i + 1) % 10 == 0) {
			ESLOG(SUMMARY) << ss.str();
			ss.clear();
		}
	}
	ESLOG(SUMMARY) << ss.str();
}



TimeEval::TimeEval(std::string name):
	evalName(name),
	totalTime(TimeEvent(name + std::string("- Total "))),
	remainingTime(TimeEvent(name + std::string("- Remaining ")))
{
}

void TimeEval::addEvent(TimeEvent &timeEvent) {
	timeEvents.push_back(timeEvent);
}

void TimeEval::printStats() {
	totalTime.evaluate();

	for (eslocal i = 0; i < timeEvents.size(); i++) {
		timeEvents[i].printStat(totalTime.avgTime);
	}

	totalTime.printStat(totalTime.avgTime);
}

void TimeEval::printStatsMPI() {
	double sum_avg_time = 0;

	auto separator = [] (int size, char character) {
		std::stringstream ss;
		for (int i = 0; i < size; i++) {
			ss << character;
		}
		return ss.str();
	};

	int separator_size = 80;

	ESLOG(SUMMARY) << separator(separator_size, '*');
	ESLOG(SUMMARY) << "        " << evalName << "         ";
	ESLOG(SUMMARY) << separator(separator_size, '*');
	totalTime.evaluateMPI();

	for (eslocal i = 0; i < timeEvents.size(); i++) {
		timeEvents[i].printStatMPI(totalTime.g_avgTime);
		sum_avg_time += timeEvents[i].avgTime;
	}

	ESLOG(SUMMARY) << separator(separator_size, '-');

	totalTime.printStatMPI(totalTime.g_avgTime);

	ESLOG(SUMMARY) << separator(separator_size, '*');
}







