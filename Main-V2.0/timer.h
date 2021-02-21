#ifndef TIMER_H
#define TIMER_H

class Timer {
  public:
    long unsigned int (*timingFunc)();
    Timer ( long unsigned int (*t)() );
    void begin ();
    void begin (int _start);
    void reset ();
    int elapsed ();

    void operator +=(int x);
    void operator -=(int x);
    int operator ==(int compare);
    int operator <(int compare);
    int operator >(int compare);
    int operator >=(int compare);
    int operator <=(int compare);
  private:
    int start;
};

#endif
