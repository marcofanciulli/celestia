// SdlTimer.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#include "timer.h"
#include <SDL.h>

class SdlTimer : public Timer
{
public:
    SdlTimer();
    ~SdlTimer();
    double getTime() const;
    void reset();

private:
    Uint64 start;
};


SdlTimer::SdlTimer()
{
    reset();
}

SdlTimer::~SdlTimer()
{
}

double SdlTimer::getTime() const
{
  double result = SDL_GetPerformanceCounter() - start;
  result /= SDL_GetPerformanceFrequency();
  return result;
}

void SdlTimer::reset()
{
  start = SDL_GetPerformanceCounter();
}

Timer* CreateTimer()
{
    return new SdlTimer();
}
