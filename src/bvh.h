#pragma once

#include "core/geom.h"

#include <vector>

#include "bounding_box.h"

// output
struct BvhNode
{
  BoundingBox boundaries;
  int children[2]; // non-leaf node
  std::vector<int> objects; // leaf node
};

std::vector<BvhNode> computeBoundingVolumeHierarchy(span<const BoundingBox> objects);
