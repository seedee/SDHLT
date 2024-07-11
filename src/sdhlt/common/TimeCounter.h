#ifndef TIMECOUNTER_H__
#define TIMECOUNTER_H__

#if _MSC_VER >= 1000
#pragma once
#endif

#include "cmdlib.h"

class TimeCounter
{
public:
    void start()
    {
        starttime = I_FloatTime();
    }

    void stop()
    {
        double stop = I_FloatTime();
        accum += stop - starttime;
    }

    double getTotal() const
    {
        return accum;
    }

    void reset()
    {
        memset(this, 0, sizeof(*this));
    }

// Construction
public:
    TimeCounter()
    {
        reset();
    }
    // Default Destructor ok
    // Default Copy Constructor ok
    // Default Copy Operator ok

protected:
    double starttime;
    double accum;
};

#endif//TIMECOUNTER_H__