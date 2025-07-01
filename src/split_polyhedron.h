#pragma once

#include "core/geom.h"

#include "polyhedron.h"

struct Plane3
{
  Vec3 normal;
  float dist;
};

void splitPolyhedronAgainstPlane(const Polyhedron3f& poly, Plane3 plane, Polyhedron3f& front, Polyhedron3f& back);
