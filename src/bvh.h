#pragma once

#include "core/geom.h"

#include <vector>

#include "bounding_box.h"

// input
struct Triangle
{
  Vec2 a, b, c;
};

// output
struct BvhNode
{
  BoundingBox boundaries;
  int children[2]; // non-leaf node
  std::vector<int> triangles; // leaf node
};

std::vector<BvhNode> computeBoundingVolumeHierarchy(span<const Triangle> triangles);
