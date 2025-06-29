#pragma once

#include "core/geom.h"

#include <vector>

struct Face3
{
  std::vector<int> indices;
};

struct Polyhedron3f
{
  std::vector<Vec3> vertices;
  std::vector<Face3> faces;

  Vec2 normal(int faceIdx) const;

  float faceLength(int faceIdx) const;
};
