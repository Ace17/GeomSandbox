#pragma once

#include "core/geom.h"

#include <vector>

struct PolyhedronFacet
{
  std::vector<int> indices;
};

struct PolyhedronFL
{
  std::vector<Vec3> vertices;
  std::vector<PolyhedronFacet> faces;

  Vec2 normal(int faceIdx) const;

  float faceLength(int faceIdx) const;
};
