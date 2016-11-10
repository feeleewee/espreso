/*! \file   elapsed_time.cpp
    \brief  time esurment functions
    \author Atsushi Suzuki, Laboratoire Jacques-Louis Lions
    \date   Jun. 4th 2013
    \date   Jul. 12th 2015
    \date   Feb. 29th 2016
*/
#include "elapsed_time.hpp"
#ifdef CLOCK_GETTIME
#ifdef _MSC_VER // added by Yann Collette

// From http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows

LARGE_INTEGER getFILETIMEoffset()
{
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return (t);
}

int clock_gettime(int X, struct timeval *tv)
{
    LARGE_INTEGER           t;
    FILETIME            	f;
    double                  microseconds;
    static LARGE_INTEGER    offset;
    static double           frequencyToMicroseconds;
    static int              initialized = 0;
    static BOOL             usePerformanceCounter = 0;

    if (!initialized) {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter) {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
        } else {
            offset = getFILETIMEoffset();
            frequencyToMicroseconds = 10.;
        }
    }
    if (usePerformanceCounter) QueryPerformanceCounter(&t);
    else {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart = microseconds;
    tv->tv_sec = t.QuadPart / 1000000;
    tv->tv_usec = t.QuadPart % 1000000;
    return (0);
}

void get_realtime(elapsed_t *tm)
{
  clock_gettime(0, (timeval *)tm);
}

double convert_time(elapsed_t time1, elapsed_t time0)
{
  return ((double)time1.tv_sec - (double)time0.tv_sec + 
	  ((double)time1.tv_usec - (double)time0.tv_usec) / 1.0e+6);
}

int convert_sec(elapsed_t t)
{
  return (int)t.tv_sec;
}

int convert_microsec(elapsed_t t)
{
  return (int)(t.tv_usec);
}
#else // _MSC_VER
void get_realtime(elapsed_t *tm)
{
  //clock_gettime(CLOCK_REALTIME, tm);
  clock_gettime(CLOCK_MONOTONIC, tm);
}

double convert_time(elapsed_t time1, elapsed_t time0)
{
  double t;
  t = ((double)time1.tv_sec - 
       (double)time0.tv_sec +
       ((double)time1.tv_nsec - 
	(double)time0.tv_nsec) / 1.0e+9);
  return t;
}

int convert_sec(elapsed_t t)
{
  return (int)t.tv_sec;
}

int convert_microsec(elapsed_t t)
{
  return (int)(t.tv_nsec / 1.0e+3);
}
#endif // _MSC_VER
#else  /* #ifdef CLOCK_GETTIME */
#ifdef GETRUSAGE
void get_realtime(elapsed_t *tm)
{
  getrusage(RUSAGE_SELF, tm);
}

double convert_time(elapsed_t time1, elapsed_t time0)
{
  double t;
  t = ((double)time1.ru_utime.tv_sec - 
       (double)time0.ru_utime.tv_sec +
       (double)time1.ru_stime.tv_sec - 
       (double)time0.ru_stime.tv_sec +
       ((double)time1.ru_utime.tv_usec - 
	(double)time0.ru_utime.tv_usec +
	(double)time1.ru_stime.tv_usec - 
	(double)time0.ru_stime.tv_usec) / 1.0e+6);
  return t;
}

int convert_sec(elapsed_t t)
{
  int t0 = (int)(t.ru_utime.tv_sec + t.ru_stime.tv_sec);
  int t1 = (int)(t.ru_utime.tv_usec + t.ru_stime.tv_usec);
  return (int)(t0 + t1 / 1000000);
}

int convert_microsec(elapsed_t t)
{
  int t1 = (int)(t.ru_utime.tv_usec + t.ru_stime.tv_usec);
  return (int)(t1 % 1000000);
}

#else
#ifdef CLOCK

void get_realtime(elapsed_t *tm)
{
  *tm = clock();
}

double convert_time(elapsed_t time1, elapsed_t time0)
{
  double t;
  t = (time1 - time0) / CLOCKS_PER_SEC;
  return t;
}

int convert_sec(elapsed_t t)
{
  return (int)(t / CLOCKS_PER_SEC);
}

int convert_microsec(elapsed_t t)
{
  return (int)((t * 1.0e+3) / CLOCKS_PER_SEC);
}

#else
void get_realtime(elapsed_t *tm)
{
  gettimeofday(tm, (struct timezone *)0);
}

double convert_time(elapsed_t time1, elapsed_t time0)
{
  double t;
  t = ((double)time1.tv_sec - 
       (double)time0.tv_sec +
       ((double)time1.tv_usec - 
	(double)time0.tv_usec) / 1.0e+6);
  return t;
}


int convert_sec(elapsed_t t)
{
  return (int)t.tv_sec;
}

int convert_microsec(elapsed_t t)
{
  return (int)t.tv_usec;
}
#endif /* #ifdef CLOCK */
#endif /* #ifdef GETRUSAGE */
#endif /* #ifdef CLOCK_GETTIME */
