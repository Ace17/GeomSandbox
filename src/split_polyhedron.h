#pragma once

#include "core/geom.h"

#include "polyhedron.h"

struct Plane3
{
  Vec3 normal;
  float dist;
};

void splitPolyhedronAgainstPlane(PolyhedronFL poly, Plane3 plane, PolyhedronFL& front, PolyhedronFL& back);
