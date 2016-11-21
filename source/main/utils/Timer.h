// license: do whatever you want to do with it ;)
#pragma once
#ifndef TIMER_H__
#define TIMER_H__

#include "RoRPrerequisites.h"

// boost timer is awful, measures cpu time on linux only...
// thus we have to hack together some cross platform timer :(

#ifndef _WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif // _WIN32

class PrecisionTimer : public ZeroedMemoryAllocator
{
protected:
#ifdef _WIN32
    LARGE_INTEGER start;
#else
    struct timeval start;
#endif // _WIN32

public:
    PrecisionTimer()
    {
        restart();
    }

    double elapsed()
    {
#ifdef _WIN32
        LARGE_INTEGER tick, ticksPerSecond;
        QueryPerformanceFrequency(&ticksPerSecond);
        QueryPerformanceCounter(&tick);
        return ((double)tick.QuadPart - (double)start.QuadPart) / (double)ticksPerSecond.QuadPart;
#else
        struct timeval now;
        gettimeofday(&now, NULL);
        return (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec)/1000000.0;
#endif // _WIN32
    }

    void restart()
    {
#ifdef _WIN32
        QueryPerformanceCounter(&start);
#else
        gettimeofday(&start, NULL);
#endif // _WIN32
    }
};

#endif //TIMER_H__
