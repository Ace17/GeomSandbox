#pragma once

#include "geom.h"
#include "polygon.h"

struct Plane
{
  Vec2 normal;
  float dist;
};

void splitPolygonAgainstPlane(const Polygon2f& poly, Plane plane, Polygon2f& front, Polygon2f& back);
