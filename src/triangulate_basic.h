#pragma once

#include "core/geom.h"

#include <vector>

struct HalfEdge
{
  int point;
  int next;
  int twin = -1;
};

std::vector<HalfEdge> createBasicTriangulation(span<const Vec2> points);
