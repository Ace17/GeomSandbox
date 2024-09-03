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
};

struct Input
{
  std::vector<Triangle> shapes;
};

struct Node
{
  AABB boundaries;
  int children[2]; // non-leaf node
  std::vector<int> triangles; // leaf node
};

void addPoint(AABB& box, Vec2 point)
{
  box.mins.x = std::min(box.mins.x, point.x);
  box.mins.y = std::min(box.mins.y, point.y);
  box.maxs.x = std::max(box.maxs.x, point.x);
  box.maxs.y = std::max(box.maxs.y, point.y);
}

AABB computeBoundingBox(span<const Triangle> allTriangles, span<const int> triangles)
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

void subdivide(int nodeIdx, span<const Triangle> allTriangles, std::vector<Node>& nodes)
{
  auto node = &nodes[nodeIdx];
  Vec2 size = node->boundaries.maxs - node->boundaries.mins;
  Vec2 cuttingNormal;

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

  node->children[0] = nodes.size();
  nodes.push_back({});
  auto& child0 = nodes.back();
  child0.triangles.assign(node->triangles.begin(), node->triangles.begin() + middle);
  child0.boundaries = computeBoundingBox(allTriangles, child0.triangles);

  node->children[1] = nodes.size();
  nodes.push_back({});
  auto& child1 = nodes.back();
  child1.triangles.assign(node->triangles.begin() + middle, node->triangles.end());
  child1.boundaries = computeBoundingBox(allTriangles, child1.triangles);

  node->triangles.clear();

  {
    auto a = &nodes[node->children[0]];
    auto b = &nodes[node->children[1]];
    sandbox_rect(a->boundaries.mins, a->boundaries.maxs - a->boundaries.mins, Green);
    sandbox_rect(b->boundaries.mins, b->boundaries.maxs - b->boundaries.mins, Green);
    sandbox_breakpoint();
  }
}

struct Output
{
  std::vector<Node> nodes;
};

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

void drawNode(int curr, span<const Node> allNodes, int depth = 0)
{
  auto* node = &allNodes[curr];
  sandbox_rect(node->boundaries.mins, node->boundaries.maxs - node->boundaries.mins, colors[depth % 11]);

  if(node->children[0])
    drawNode(node->children[0], allNodes, depth + 1);

  if(node->children[1])
    drawNode(node->children[1], allNodes, depth + 1);
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
        t.a = randomPos(Vec2(-20, -10), Vec2(20, 10));
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

    r.nodes.reserve(input.shapes.size() * 2);

    r.nodes.push_back({});
    auto& rootNode = r.nodes.back();
    for(int i = 0; i < (int)input.shapes.size(); ++i)
      rootNode.triangles.push_back(i);
    rootNode.boundaries = computeBoundingBox(input.shapes, rootNode.triangles);

    std::vector<int> stack;
    stack.push_back(0);

    while(stack.size())
    {
      auto curr = stack.back();
      stack.pop_back();

      if(r.nodes[curr].triangles.size() <= 2)
        continue;

      subdivide(curr, input.shapes, r.nodes);

      stack.push_back(r.nodes[curr].children[1]);
      stack.push_back(r.nodes[curr].children[0]);
    }

    r.nodes.shrink_to_fit();
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

    if(output.nodes.size())
      drawNode(0, output.nodes);
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<BoundingVolumeHierarchy>>()); }
const int registered = registerApp("BoundingVolumeHierarchy", &create);
}
