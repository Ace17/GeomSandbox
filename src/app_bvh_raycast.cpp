// Copyright (C) 2024 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Raycast against a BVH

#include "core/algorithm_app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

#include "bvh.h"
#include "random.h"

namespace
{

std::vector<short> status_bvh;

bool intersectsAABB(Vec2 a, Vec2 b, BoundingBox aabb)
{
  auto delta = b - a;

  float tmin = 0;
  float tmax = 1;

  float tx1 = (aabb.min.x - a.x) / delta.x;
  float tx2 = (aabb.max.x - a.x) / delta.x;
  tmin = max(tmin, min(tx1, tx2));
  tmax = min(tmax, max(tx1, tx2));

  float ty1 = (aabb.min.y - a.y) / delta.y;
  float ty2 = (aabb.max.y - a.y) / delta.y;
  tmin = max(tmin, min(ty1, ty2));
  tmax = min(tmax, max(ty1, ty2));

  return tmax >= tmin;
}

float raycast(Vec2 a, Vec2 b, span<const BvhNode> bvh)
{
  status_bvh.clear();
  status_bvh.resize(bvh.len);

  std::vector<int> stack;
  stack.reserve(bvh.len);

  stack.push_back(0);

  while(stack.size())
  {
    const int curr = stack.back();
    stack.pop_back();

    status_bvh[curr] = 2; // tested
    if(!intersectsAABB(a, b, bvh[curr].boundaries))
      continue;

    status_bvh[curr] = 1; // tested, intersection found

    if(bvh[curr].children[0])
      stack.push_back(bvh[curr].children[0]);

    if(bvh[curr].children[1])
      stack.push_back(bvh[curr].children[1]);
  }

  return 1;
}

struct BvhRaycastApp : IApp
{
  BvhRaycastApp()
  {
    rayStart = randomPos({-20, -10}, {20, 10});
    rayTarget = randomPos({-20, -10}, {20, 10});

    static auto det2d = [](Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; };

    for(int k = 0; k < 20; ++k)
    {
      Triangle t;

      do
      {
        t.a = randomPos(Vec2(-20, -10), Vec2(20, 10));
        t.b = t.a + randomPos(Vec2(0, 0), Vec2(3, 3));
        t.c = t.a + randomPos(Vec2(0, 0), Vec2(3, 3));
      } while(det2d(t.b - t.a, t.c - t.a) < 0.5);

      shapes.push_back(t);
    }

    bvh = computeBoundingVolumeHierarchy(shapes);

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    // draw triangles
    for(auto& tri : shapes)
    {
      drawer->line(tri.a, tri.b, Blue);
      drawer->line(tri.b, tri.c, Blue);
      drawer->line(tri.c, tri.a, Blue);
    }

    // draw BVH
    for(auto& node : bvh)
      drawer->rect(node.boundaries.min, node.boundaries.max - node.boundaries.min, Gray);

    for(auto& node : bvh)
    {
      auto i = int(&node - bvh.data());
      if(status_bvh[i] != 0)
      {
        auto margin = Vec2(0.1, 0.1);
        drawer->rect(node.boundaries.min + margin, node.boundaries.max - node.boundaries.min - 2 * margin, status_bvh[i] == 1 ? Yellow : White);

        if(status_bvh[i] == 1)
        {
          for(auto& index : node.triangles)
          {
            auto& tri = shapes[index];
            drawer->line(tri.a, tri.b, Yellow);
            drawer->line(tri.b, tri.c, Yellow);
            drawer->line(tri.c, tri.a, Yellow);
          }
        }
      }
    }

    // draw selection
    {
      const auto hs = Vec2(0.3, 0.3);
      auto& box = currentSelection == 0 ? rayStart : rayTarget;
      drawer->rect(box - hs, hs * 2, White);
    }

    // draw trajectory
    drawer->line(rayStart, rayTarget, White);

    // draw start, target, and finish positions
    drawer->text(rayStart, "start", Green);
    drawCross(drawer, rayStart, Green);

    drawer->text(rayTarget, "target", Green);
    drawCross(drawer, rayTarget, Green);
  }

  void drawCross(IDrawer* drawer, Vec2 pos, Color color)
  {
    drawer->line(pos - Vec2{1, 0}, pos + Vec2{1, 0}, color);
    drawer->line(pos - Vec2{0, 1}, pos + Vec2{0, 1}, color);
  }

  void processEvent(InputEvent inputEvent) override
  {
    if(inputEvent.pressed)
      keydown(inputEvent.key);
  }

  void keydown(Key key)
  {
    auto& point = currentSelection == 0 ? rayStart : rayTarget;

    switch(key)
    {
    case Key::Left:
      point.x -= 1;
      break;
    case Key::Right:
      point.x += 1;
      break;
    case Key::Up:
      point.y += 1;
      break;
    case Key::Down:
      point.y -= 1;
      break;
    case Key::Space:
      currentSelection = 1 - currentSelection;
      break;
    default:
      break;
    }

    compute();
  }

  void compute() { rayRatio = raycast(rayStart, rayTarget, bvh); }

  std::vector<Triangle> shapes;
  std::vector<BvhNode> bvh;

  Vec2 rayStart; // the starting position
  Vec2 rayTarget; // the target position
  float rayRatio; // the amount of move we can do

  int currentSelection = 0;
};

IApp* create() { return new BvhRaycastApp; }
const int registered = registerApp("Bvh.Raycast", &create);
}
