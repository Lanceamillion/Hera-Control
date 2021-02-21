#include "secondsTimer.h"
#include "timer.h"

SecondsTimer::SecondsTimer (Timer *millis) {
  millisTimer = millis;
}

SecondsTimer::SecondsTimer ( long unsigned int (*t)() ) {
  millisTimer = new Timer(t);
}

SecondsTimer::~SecondsTimer () {
  delete millisTimer;
}

void SecondsTimer::begin () {
  seconds = 0;
  millisTimer->begin();
}

void SecondsTimer::begin (int start) {
  seconds = start;
  millisTimer->begin();
}

void SecondsTimer::reset () {
  seconds = 0;
  millisTimer->reset();
}

void SecondsTimer::count () {
  if (*millisTimer > 1000) {
    millisTimer->reset();
    seconds += 1;
  }
}

int SecondsTimer::elapsed () {
  return seconds;
}

void SecondsTimer::operator +=(int x) {
  seconds += x;
}

void SecondsTimer::operator -=(int x) {
  seconds -= x;
}

int SecondsTimer::operator ==(int compare) {
  return seconds == compare;
}

int SecondsTimer::operator <(int compare) {
  return seconds < compare;
}

int SecondsTimer::operator >(int compare) {
  return seconds > compare;
}

int SecondsTimer::operator >=(int compare) {
  return seconds >= compare;
}

int SecondsTimer::operator <=(int compare) {
  return seconds <= compare;
}

