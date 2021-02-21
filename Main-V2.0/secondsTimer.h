#ifndef SECONDS_TIMER_H
#define SECONDS_TIMER_H

#include "timer.h"

class SecondsTimer {
  public:
    SecondsTimer ( long unsigned int (*t)() );
    SecondsTimer (Timer *millis);
    ~SecondsTimer ();
    void begin ();
    void begin (int start);
    void reset ();
    void count ();
    int elapsed ();

    void operator +=(int x);
    void operator -=(int x);
    int operator ==(int compare);
    int operator <(int compare);
    int operator >(int compare);
    int operator >=(int compare);
    int operator <=(int compare);
  private:
    Timer *millisTimer;
    int seconds;
};

#endif
