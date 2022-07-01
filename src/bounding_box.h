#pragma once

#include "core/geom.h"

struct BoundingBox
{
  BoundingBox()
  {
    min.x = min.y = 1.0 / 0.0;
    max.x = max.y = -1.0 / 0.0;
  }

  void add(Vec2 p)
  {
    min.x = std::min(min.x, p.x);
    max.x = std::max(max.x, p.x);
    min.y = std::min(min.y, p.y);
    max.y = std::max(max.y, p.y);
  }

  Vec2 min, max;
};
