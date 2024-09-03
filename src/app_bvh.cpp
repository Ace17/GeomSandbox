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

#include <memory>
#include <vector>

#include "bvh.h"
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

struct Input
{
  std::vector<Triangle> shapes;
};

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

  static Output execute(Input input) { return {computeBoundingVolumeHierarchy(input.shapes)}; }

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
