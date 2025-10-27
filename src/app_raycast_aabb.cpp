// Copyright (C) 2024 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Raycast against an AABB

#include "core/app.h"
#include "core/bounding_box.h"
#include "core/drawer.h"
#include "core/geom.h"

#include "random.h"

namespace
{

float g_enter = 0;
float g_leave = 1;

float raycast(Vec2 start, Vec2 target, BoundingBox aabb)
{
  const float tx0 = (aabb.min.x - start.x) / (target.x - start.x);
  const float tx1 = (aabb.max.x - start.x) / (target.x - start.x);

  const float tx_min = std::min(tx0, tx1);
  const float tx_max = std::max(tx0, tx1);

  const float ty0 = (aabb.min.y - start.y) / (target.y - start.y);
  const float ty1 = (aabb.max.y - start.y) / (target.y - start.y);

  const float ty_min = std::min(ty0, ty1);
  const float ty_max = std::max(ty0, ty1);

  const auto enter = std::max(tx_min, ty_min);
  const auto leave = std::min(tx_max, ty_max);

  g_enter = enter;
  g_leave = leave;

  if(enter < 0 && leave < 0)
    return 1; // the box is completely behind the ray

  if(enter > 1 && leave > 1)
    return 1; // the box is completely ahead of the ray

  if(enter < 0 && leave > 1)
    return 1; // the ray is completely inside the box

  if(enter > leave)
    return 1; // the line doesn't cross the box

  if(enter < 0)
    return leave; // the ray starts inside the box, and ends outside

  return enter; // the ray starts outside the box, and ends inside
}

struct RaycastAgainstAABB : IApp
{
  RaycastAgainstAABB()
  {
    rayStart = randomPos({-20, -10}, {20, 10});
    rayTarget = randomPos({-20, -10}, {20, 10});

    aabb.min = randomPos({-10, -10}, {10, 10});
    aabb.max = aabb.min + randomPos({10, 5}, {10, 5});

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    // draw shapes
    drawer->rect(aabb.min, aabb.max - aabb.min, White);

    // draw selection
    {
      const auto hs = Vec2(0.3, 0.3);
      auto& box = currentSelection == 0 ? rayStart : rayTarget;
      drawer->rect(box - hs, hs * 2, White);
    }

    // draw trajectory
    drawer->line(rayStart, rayTarget, rayRatio == 1 ? White : Red);
    drawer->line(rayStart, rayFinish, Green);

    auto delta = rayTarget - rayStart;

    drawer->text(rayStart + delta * g_enter + Vec2{0, 0.5f}, "enter", Yellow);
    drawCross(drawer, rayStart + delta * g_enter, Yellow);

    drawer->text(rayStart + delta * g_leave + Vec2{0, 0.5f}, "leave", Yellow);
    drawCross(drawer, rayStart + delta * g_leave, Yellow);

    // draw start, target, and finish positions
    drawer->text(rayStart, "start", Green);
    drawCross(drawer, rayStart, Green);

    drawer->text(rayTarget, "target", Green);
    drawCross(drawer, rayTarget, Green);

    drawer->text(rayFinish, "finish", Green);
    drawCross(drawer, rayFinish, Green);
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

  void compute()
  {
    rayRatio = raycast(rayStart, rayTarget, aabb);
    rayFinish = rayStart * (1 - rayRatio) + rayTarget * rayRatio;
  }

  BoundingBox aabb;
  Vec2 rayStart; // the starting position
  Vec2 rayTarget; // the target position
  Vec2 rayFinish; // the finish position
  float rayRatio; // the amount of move we can do

  int currentSelection = 0;
};

IApp* create() { return new RaycastAgainstAABB; }
const int registered = registerApp("Raycast/AABB", &create);
}
