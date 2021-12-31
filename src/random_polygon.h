#pragma once

#include <vector>

#include "geom.h"

class IVisualizer;

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

Polygon2f createRandomPolygon2f(IVisualizer* visualizer);
