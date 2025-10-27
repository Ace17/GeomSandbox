// Copyright (C) 2024 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Raycast against a BVH

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cmath>
#include <cstdio>
#include <vector>

#include "bvh.h"
#include "random.h"

namespace
{

struct Circle
{
  Vec2 center;
  float radius;
};

std::vector<short> status_bvh;
int status_counter;

void order(float& a, float& b)
{
  if(a > b)
    std::swap(a, b);
}

float raytraceThroughAABB(Vec2 a, Vec2 b, BoundingBox aabb)
{
  auto delta = b - a;

  float enter = 0;
  float leave = 1;

  float timeEnterOnX = (aabb.min.x - a.x) / delta.x;
  float timeLeaveOnX = (aabb.max.x - a.x) / delta.x;
  order(timeEnterOnX, timeLeaveOnX);

  enter = max(enter, timeEnterOnX);
  leave = min(leave, timeLeaveOnX);

  float timeEnterOnY = (aabb.min.y - a.y) / delta.y;
  float timeLeaveOnY = (aabb.max.y - a.y) / delta.y;
  order(timeEnterOnY, timeLeaveOnY);

  enter = max(enter, timeEnterOnY);
  leave = min(leave, timeLeaveOnY);

  if(leave < enter)
    return 1; // not hit

  return enter;
}

float raycastThroughOneCircle(Vec2 A, Vec2 B, Circle circle)
{
  const auto C = circle.center;
  const auto r = circle.radius;

  const auto a = dotProduct(B - A, B - A);
  const auto b = -2 * dotProduct(B - A, C - A);
  const auto c = dotProduct(C - A, C - A) - r * r;

  const auto delta = b * b - 4 * a * c;

  if(delta < 0)
    return 1;

  const auto sqrtDelta = sqrt(delta);

  float ratio = 1;

  const auto t0 = (-b - sqrtDelta) / (2 * a);
  if(t0 >= 0 && t0 < 1)
    ratio = min(ratio, t0);

  const auto t1 = (-b + sqrtDelta) / (2 * a);
  if(t1 >= 0 && t1 < 1)
    ratio = min(ratio, t1);

  return ratio;
}

float raycast(Vec2 a, Vec2 b, span<const BvhNode> bvh, span<const Circle> shapes)
{
  status_bvh.clear();
  status_bvh.resize(bvh.len);
  status_counter = 0;

  std::vector<int> stack;
  stack.reserve(bvh.len);

  stack.push_back(0);

  float minRatio = 1.0f;

  while(stack.size())
  {
    const int curr = stack.back();
    stack.pop_back();

    for(auto obj : bvh[curr].objects)
    {
      float ratio = raycastThroughOneCircle(a, b, shapes[obj]);
      if(ratio < minRatio)
        minRatio = ratio;
      ++status_counter;
    }

    status_bvh[curr] = 1; // tested, intersection found

    const int child0 = bvh[curr].children[0];
    const int child1 = bvh[curr].children[1];

    float t0 = 1;
    if(child0)
    {
      status_bvh[child0] = 2; // tested
      t0 = raytraceThroughAABB(a, b, bvh[child0].boundaries);
    }

    float t1 = 1;
    if(child1)
    {
      status_bvh[child1] = 2; // tested
      t1 = raytraceThroughAABB(a, b, bvh[child1].boundaries);
    }

    if(t0 < minRatio)
      stack.push_back(child0);

    if(t1 < minRatio)
      stack.push_back(child1);

    // optimization: reorder children, so the one with the lower hit time
    // gets explored first
    if(t0 < minRatio && t1 < minRatio)
      if(t0 < t1)
        std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
  }

  return minRatio;
}

struct BvhRaycastApp : IApp
{
  BvhRaycastApp()
  {
    rayStart = randomPos({-20, -10}, {20, 10});
    rayTarget = randomPos({-20, -10}, {20, 10});

    for(int k = 0; k < 20; ++k)
    {
      Circle c;

      c.center = randomPos(Vec2(-20, -10), Vec2(20, 10));
      c.radius = randomFloat(0.5, 3);

      shapes.push_back(c);
    }

    {
      std::vector<BoundingBox> boxes;
      boxes.reserve(shapes.size());
      for(auto& obj : shapes)
      {
        BoundingBox box;
        box.add(obj.center - Vec2(obj.radius, obj.radius));
        box.add(obj.center + Vec2(obj.radius, obj.radius));
        boxes.push_back(box);
      }
      bvh = computeBoundingVolumeHierarchy(boxes);
    }

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    // draw shapes
    for(auto& c : shapes)
      drawer->circle(c.center, c.radius, Blue);

    // draw BVH
    for(auto& node : bvh)
      drawer->rect(node.boundaries.min, node.boundaries.max - node.boundaries.min, Gray);

    for(auto& node : bvh)
    {
      auto i = int(&node - bvh.data());
      if(status_bvh[i] != 0)
      {
        auto margin = Vec2(0.1, 0.1);
        drawer->rect(node.boundaries.min + margin, node.boundaries.max - node.boundaries.min - 2 * margin,
              status_bvh[i] == 1 ? Yellow : White);

        if(status_bvh[i] == 1)
        {
          for(auto index : node.objects)
          {
            auto& tri = shapes[index];
            drawer->circle(tri.center, tri.radius, Yellow);
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

    {
      auto targetColor = rayRatio != 1.0 ? Red : Green;
      drawer->text(rayTarget, "target", targetColor);
      drawCross(drawer, rayTarget, targetColor);
    }

    if(rayRatio != 1.0)
    {
      const auto rayFinish = rayStart + rayRatio * (rayTarget - rayStart);
      drawer->text(rayFinish, "finish", Green);
      drawCross(drawer, rayFinish, Green);
    }

    {
      char buf[256];
      sprintf(buf, "%d intersection tests", status_counter);
      drawer->text({}, buf, White);
    }
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

  void compute() { rayRatio = raycast(rayStart, rayTarget, bvh, shapes); }

  std::vector<Circle> shapes;
  std::vector<BvhNode> bvh;

  Vec2 rayStart; // the starting position
  Vec2 rayTarget; // the target position
  float rayRatio; // the amount of move we can do

  int currentSelection = 0;
};

IApp* create() { return new BvhRaycastApp; }
const int registered = registerApp("SpatialPartitioning/Bvh/Raycast", &create);
}
