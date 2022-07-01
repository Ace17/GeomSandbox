#pragma once

#include "core/geom.h"

#include <vector>

struct Face
{
  int a, b;
};

struct Polygon2f
{
  std::vector<Vec2> vertices;
  std::vector<Face> faces;

  Vec2 normal(int faceIdx) const;

  float faceLength(int faceIdx) const;
};
