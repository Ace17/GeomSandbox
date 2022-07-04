#pragma once

#include "core/geom.h"

#include <memory>
#include <vector>

struct Polygon2f;

const auto BspEpsilon = 0.001;

struct Hyperplane
{
  Vec2 normal;
  float dist;
};

struct BspFace
{
  Vec2 a, b;
  Vec2 normal;
};

struct BspNode
{
  Hyperplane plane;
  std::unique_ptr<BspNode> posChild;
  std::unique_ptr<BspNode> negChild;
  std::vector<BspFace> coincident;
};

std::unique_ptr<BspNode> createBspTree(const Polygon2f& polygon);
