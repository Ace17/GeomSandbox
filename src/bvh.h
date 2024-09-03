#pragma once

#include "core/geom.h"

#include <vector>

// input
struct Triangle
{
  Vec2 a, b, c;
};

// output
struct AABB
{
  Vec2 mins, maxs;
};

struct Node
{
  AABB boundaries;
  int children[2]; // non-leaf node
  std::vector<int> triangles; // leaf node
};

std::vector<Node> computeBoundingVolumeHierarchy(span<const Triangle> triangles);
