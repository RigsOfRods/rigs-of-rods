#ifndef TIMER_H__
#define TIMER_H__

// boost timer is awful, measures cpu time on linux only...
// thus we have to hack together some cross platform timer :(

#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif

class Timer
{
protected:
#ifdef WIN32
	LARGE_INTEGER start;
#else
	struct timeval start;
#endif

public:
	Timer()
	{
		restart();
	}

	double elapsed()
	{
#ifdef WIN32
		LARGE_INTEGER tick, ticksPerSecond;
		QueryPerformanceFrequency(&ticksPerSecond);
		QueryPerformanceCounter(&tick);
		return ((double)tick.QuadPart - (double)start.QuadPart) / (double)ticksPerSecond.QuadPart;
#else
		struct timeval now;
		gettimeofday(&now, NULL);
		return (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec)/1000000.0;
#endif
	}

	void restart()
	{
#ifdef WIN32
		QueryPerformanceCounter(&start);		
#else
		gettimeofday(&start, NULL);
#endif
	}
};

#endif //TIMER_H__