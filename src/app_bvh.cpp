// Copyright (C) 2024 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Compute a bounding volume hierarchy from a list of triangles

#include "core/algorithm_app.h"
#include "core/geom.h"
#include "core/sandbox.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

#include "random.h"

namespace
{

const Color colors[] = {
      {0, 1, 0, 1},
      {0, 1, 1, 1},
      {1, 0, 0, 1},
      {1, 0, 1, 1},
      {1, 1, 0, 1},
      {1, 1, 1, 1},
      {0.5, 0.5, 1, 1},
      {0.5, 1, 0.5, 1},
      {0.5, 1, 1, 1},
      {1, 0.5, 0.5, 1},
      {1, 0.5, 1, 1},
};

struct AABB
{
  Vec2 mins, maxs;
};

struct Triangle
{
  Vec2 a, b, c;
  Vec2 center;
};

struct Input
{
  std::vector<Triangle> shapes;
};

struct Node
{
  AABB boundaries;
  std::shared_ptr<Node> children[2]; // non-leaf node
  std::vector<int> triangles; // leaf node
};

void addPoint(AABB& box, Vec2 point)
{
  box.mins.x = std::min(box.mins.x, point.x);
  box.mins.y = std::min(box.mins.y, point.y);
  box.maxs.x = std::max(box.maxs.x, point.x);
  box.maxs.y = std::max(box.maxs.y, point.y);
}

AABB computeBoundingBox(const std::vector<Triangle> allTriangles, const std::vector<int> triangles)
{
  AABB r;
  r.mins = r.maxs = allTriangles[triangles[0]].a;

  for(auto& tri : triangles)
  {
    addPoint(r, allTriangles[tri].a);
    addPoint(r, allTriangles[tri].b);
    addPoint(r, allTriangles[tri].c);
  }

  return r;
}

void subdivide(const std::vector<Triangle>& allTriangles, Node* node)
{
  if(node->triangles.size() <= 2)
    return;

  Vec2 size = node->boundaries.maxs - node->boundaries.mins;
  Vec2 cuttingNormal;

  assert(size.x > 0);
  assert(size.y > 0);

  if(size.x > size.y)
    cuttingNormal = Vec2(1, 0);
  else
    cuttingNormal = Vec2(0, 1);

  auto byDistanceToCuttingPlane = [&](int i, int j)
  { return allTriangles[i].a * cuttingNormal < allTriangles[j].a * cuttingNormal; };

  std::sort(node->triangles.begin(), node->triangles.end(), byDistanceToCuttingPlane);

  const int middle = node->triangles.size() / 2;

  {
    auto linePos = allTriangles[node->triangles[middle]].a;
    auto cuttingDir = rotateLeft(cuttingNormal);
    sandbox_line(linePos - cuttingDir * 100, linePos + cuttingDir * 100, Green);
    sandbox_rect(node->boundaries.mins, node->boundaries.maxs - node->boundaries.mins, Red);
    sandbox_breakpoint();
  }

  node->children[0] = std::make_unique<Node>();
  node->children[0]->triangles.assign(node->triangles.begin(), node->triangles.begin() + middle);
  node->children[0]->boundaries = computeBoundingBox(allTriangles, node->children[0]->triangles);

  node->children[1] = std::make_unique<Node>();
  node->children[1]->triangles.assign(node->triangles.begin() + middle, node->triangles.end());
  node->children[1]->boundaries = computeBoundingBox(allTriangles, node->children[1]->triangles);

  {
    auto a = node->children[0].get();
    auto b = node->children[1].get();
    sandbox_rect(a->boundaries.mins, a->boundaries.maxs - a->boundaries.mins, Green);
    sandbox_rect(b->boundaries.mins, b->boundaries.maxs - b->boundaries.mins, Green);
    sandbox_breakpoint();
  }

  subdivide(allTriangles, node->children[0].get());
  subdivide(allTriangles, node->children[1].get());
}

struct Output
{
  std::shared_ptr<Node> rootNode;
};

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

void drawNode(const Node* node, int depth = 0)
{
  sandbox_rect(node->boundaries.mins, node->boundaries.maxs - node->boundaries.mins, colors[depth % 11]);

  if(node->children[0])
    drawNode(node->children[0].get(), depth + 1);

  if(node->children[1])
    drawNode(node->children[1].get(), depth + 1);
}

struct BoundingVolumeHierarchy
{
  static Input generateInput()
  {
    Input r;

    for(int k = 0; k < 20; ++k)
    {
      Triangle t;

      do
      {
        t.a = randomPos(Vec2(-10, -10), Vec2(10, 10));
        t.b = t.a + randomPos(Vec2(0, 0), Vec2(3, 3));
        t.c = t.a + randomPos(Vec2(0, 0), Vec2(3, 3));
      } while(det2d(t.b - t.a, t.c - t.a) < 0.5);

      r.shapes.push_back(t);
    }

    return r;
  }

  static Output execute(Input input)
  {
    Output r;

    r.rootNode = std::make_unique<Node>();
    for(int i = 0; i < (int)input.shapes.size(); ++i)
      r.rootNode->triangles.push_back(i);
    r.rootNode->boundaries = computeBoundingBox(input.shapes, r.rootNode->triangles);

    subdivide(input.shapes, r.rootNode.get());
    return r;
  }

  static void display(const Input& input, Output output)
  {
    for(auto& tri : input.shapes)
    {
      sandbox_line(tri.a, tri.b, Blue);
      sandbox_line(tri.b, tri.c, Blue);
      sandbox_line(tri.c, tri.a, Blue);
    }

    if(output.rootNode)
      drawNode(output.rootNode.get());
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<BoundingVolumeHierarchy>>()); }
const int registered = registerApp("BoundingVolumeHierarchy", &create);
}
