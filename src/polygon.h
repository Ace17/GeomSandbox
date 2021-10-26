#pragma once

#include <array>
#include <vector>

#include "geom.h"

struct Polygon
{
  std::vector<std::array<int, 2>> faces;
  std::vector<Vec2> vertices;

  Vec2 normal(int faceIdx) const;
  float faceLength(int faceIdx) const;
};

std::vector<Vec2> createRandomPolygon(int seed);

