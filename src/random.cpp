// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Random generator

#include "random.h"

#include <climits> // RAND_MAX
#include <cstdlib> // rand

float randomFloat(float min, float max) { return (rand() / float(RAND_MAX)) * (max - min) + min; }
int randomInt(int min, int max) { return rand() % (max - min) + min; }

Vec2 randomPos(Vec2 min, Vec2 max)
{
  Vec2 r;
  r.x = randomFloat(min.x, max.x);
  r.y = randomFloat(min.y, max.y);
  return r;
}
