// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Continuous collision detection using the separating axis test.

#include <cassert>
#include <cmath>
#include <vector>

#include "app.h"
#include "drawer.h"
#include "geom.h"
#include "random.h"

namespace
{

float abs(float a) { return a >= 0 ? a : -a; }
float magnitude(Vec2 v) { return sqrt(v * v); }
Vec2 normalize(Vec2 v) { return v * (1.0 / magnitude(v)); }
Vec2 rotateLeft(Vec2 v) { return Vec2(-v.y, v.x); }

struct ProjectionOnAxis
{
  float center;
  float min, max;
};

struct IShape
{
  virtual ProjectionOnAxis projectOnAxis(Vec2 axis) const = 0;
  virtual std::vector<Vec2> axesToTest() const = 0;
};

// Cast a ray from 'pos' to 'pos+delta', stops when colliding with 'obstacle'.
// The return value represents the allowed move, as a fraction of the desired move (delta).
// (The final position is then 'pos + return_value * delta')
float raycast(Vec2 pos, Vec2 delta, IShape* obstacle)
{
  auto axes = obstacle->axesToTest();

  axes.push_back(rotateLeft(normalize(delta)));

  float fraction = 0;

  for(auto axis : axes)
  {
    // make the move always increase the position along the axis
    if(axis * delta < 0)
      axis = axis * -1;

    // compute projections on the axis
    const float startPos = pos * axis;
    const float targetPos = (pos + delta) * axis;
    const auto projectedObstacle = obstacle->projectOnAxis(axis);

    assert(startPos <= targetPos || fabs(startPos - targetPos) < 0.0001);

    if(targetPos < projectedObstacle.min)
      return 1; // all the axis-projected move is before the obstacle

    if(startPos >= projectedObstacle.max)
      return 1; //  all the axis-projected move is after the obstacle

    if(fabs(startPos - targetPos) > 0.00001)
    {
      float f = (projectedObstacle.min - startPos) / (targetPos - startPos);
      if(f > fraction)
        fraction = f;
    }
  }

  return fraction;
}

struct BoxShape : IShape
{
  ProjectionOnAxis projectOnAxis(Vec2 axis) const override
  {
    ProjectionOnAxis r;

    const float projectedExtent = fabs(halfSize.x * axis.x) + fabs(halfSize.y * axis.y);

    r.center = center * axis;
    r.min = r.center - projectedExtent;
    r.max = r.center + projectedExtent;

    return r;
  }

  std::vector<Vec2> axesToTest() const override { return {{1, 0}, {0, 1}}; }

  Vec2 center;
  Vec2 halfSize;
};

struct CombinedShape : IShape
{
  IShape* shapeA;
  IShape* shapeB;

  ProjectionOnAxis projectOnAxis(Vec2 axis) const override
  {
    ProjectionOnAxis projA = shapeA->projectOnAxis(axis);
    ProjectionOnAxis projB = shapeB->projectOnAxis(axis);

    ProjectionOnAxis r;
    r.center = projA.center;
    r.min = projA.min - (projB.center - projB.min);
    r.max = projA.max + (projB.max - projB.center);

    return r;
  }

  std::vector<Vec2> axesToTest() const override
  {
    std::vector<Vec2> r;
    for(auto& axis : shapeA->axesToTest())
      r.push_back(axis);
    for(auto& axis : shapeB->axesToTest())
      r.push_back(axis);
    return r;
  }
};

struct SeparatingAxisTestApp : IApp
{
  SeparatingAxisTestApp()
  {
    boxHalfSize = randomPos({2, 2}, {5, 5});
    boxStart = randomPos({-20, -10}, {20, 10});
    boxTarget = randomPos({-20, -10}, {20, 10});

    obstacleBoxPos = randomPos({-5, -5}, {5, 5});
    obstacleBoxHalfSize = randomPos({2, 2}, {5, 5});

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    drawBox(drawer, boxStart, boxHalfSize, Green, "start");
    drawBox(drawer, boxTarget, boxHalfSize, Red, "target");
    drawBox(drawer, boxFinish, boxHalfSize, LightBlue, "finish");
    drawBox(drawer, obstacleBoxPos, obstacleBoxHalfSize, Yellow, "obstacle");

    {
      const auto hs = boxHalfSize * 0.95;
      auto& box = currentSelection == 0 ? boxStart : boxTarget;
      drawer->rect(box - hs, hs * 2, White);
    }

    drawer->line(boxStart, boxTarget, White);
  }

  void drawBox(IDrawer* drawer, Vec2 pos, Vec2 halfSize, Color color, const char* name)
  {
    drawer->rect(pos - halfSize, halfSize * 2, color);
    drawer->line(pos - Vec2{1, 0}, pos + Vec2{1, 0}, color);
    drawer->line(pos - Vec2{0, 1}, pos + Vec2{0, 1}, color);
    drawer->text(pos, name, color);
  }

  void keydown(Key key) override
  {
    auto& box = currentSelection == 0 ? boxStart : boxTarget;

    switch(key)
    {
    case Key::Left:
      box.x -= 1;
      break;
    case Key::Right:
      box.x += 1;
      break;
    case Key::Up:
      box.y += 1;
      break;
    case Key::Down:
      box.y -= 1;
      break;
    case Key::Space:
      currentSelection = 1 - currentSelection;
      break;
    }

    compute();
  }

  void compute()
  {
    const auto delta = Vec2(boxTarget - boxStart);

    BoxShape moverShape{};
    moverShape.halfSize = boxHalfSize;

    BoxShape obstacleShape{};
    obstacleShape.halfSize = obstacleBoxHalfSize;
    obstacleShape.center = obstacleBoxPos;

    // instead of casting a shape (mover) against another shape (obstacle),
    // we cast a ray against the minkowski sum of both shapes.
    CombinedShape combinedShape;
    combinedShape.shapeA = &obstacleShape;
    combinedShape.shapeB = &moverShape;

    const auto fraction = raycast(boxStart, delta, &combinedShape);
    boxFinish = boxStart + delta * fraction;
  }

  Vec2 boxHalfSize;

  Vec2 boxStart; // the starting position
  Vec2 boxTarget; // the target position
  Vec2 boxFinish; // the position we actually reach (=target if we don't hit the obstacle)

  Vec2 obstacleBoxPos;
  Vec2 obstacleBoxHalfSize;

  int currentSelection = 0;
};

const int registered = registerApp("SAT", []() -> IApp* { return new SeparatingAxisTestApp; });
}
