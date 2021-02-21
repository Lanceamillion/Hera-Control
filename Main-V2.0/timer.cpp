#include "timer.h"


Timer::Timer ( long unsigned int (*t)() ) {
  timingFunc = t;
}

void Timer::begin () {
  start = timingFunc();
}

void Timer::begin (int _start) {
  start = _start;
}

void Timer::reset () {
  start = timingFunc();
}

int Timer::elapsed () {
  return timingFunc() - start;
}

void Timer::operator +=(int x) {
  start -= x;
}

void Timer::operator -=(int x) {
  start += x;
}

int Timer::operator ==(int compare) {
  return elapsed() == compare;
}

int Timer::operator <(int compare) {
  return elapsed() < compare;
}

int Timer::operator >(int compare) {
  return elapsed() > compare;
}

int Timer::operator >=(int compare) {
  return elapsed() >= compare;
}

int Timer::operator <=(int compare) {
  return elapsed() <= compare;
}
