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

struct RaycastResult
{
  float fraction = 1.0;
  Vec2 normal{};
};

// Cast a ray from 'pos' to 'pos+delta', stops when colliding with 'obstacle'.
// The return value represents the allowed move, as a fraction of the desired move (delta).
// (The final position is then 'pos + return_value * delta')
RaycastResult raycast(Vec2 pos, Vec2 delta, IShape* obstacle)
{
  auto axes = obstacle->axesToTest();

  axes.push_back(rotateLeft(normalize(delta)));

  RaycastResult r;
  r.fraction = 0;

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
      return {}; // all the axis-projected move is before the obstacle

    if(startPos >= projectedObstacle.max)
      return {}; //  all the axis-projected move is after the obstacle

    if(fabs(startPos - targetPos) > 0.00001)
    {
      float f = (projectedObstacle.min - startPos) / (targetPos - startPos);
      if(f > r.fraction)
      {
        r.fraction = f;
        r.normal = -axis;
      }
    }
  }

  return r;
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

struct PolygonShape : IShape
{
  Vec2 center;
  std::vector<Vec2> vertices;

  ProjectionOnAxis projectOnAxis(Vec2 axis) const override
  {
    ProjectionOnAxis r;

    r.center = center * axis;

    r.min = vertices[0] * axis;
    r.max = vertices[0] * axis;

    for(auto& v : vertices)
    {
      r.min = std::min<float>(r.min, v * axis);
      r.max = std::max<float>(r.max, v * axis);
    }

    assert(r.min <= r.max);

    return r;
  }

  std::vector<Vec2> axesToTest() const override
  {
    std::vector<Vec2> r;
    for(int i = 0; i < (int)vertices.size(); ++i)
    {
      auto v0 = vertices[i];
      auto v1 = vertices[(i + 1) % vertices.size()];
      auto delta = v1 - v0;
      r.push_back(rotateLeft(normalize(delta)));
    }
    return r;
  }
};

// Minkowski sum of two shapes, centered on shapeA
// (shapeB's position is ignored).
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
    boxHalfSize = randomPos({1, 1}, {3, 3});
    boxStart = randomPos({-20, -10}, {20, 10});
    boxTarget = randomPos({-20, -10}, {20, 10});

    {
      obstacleBox.halfSize = randomPos({2, 2}, {5, 5});
      obstacleBox.center = randomPos({5, -5}, {15, 5});
    }

    {
      obstaclePolygon.center = randomPos({-25, -5}, {-5, 5});

      const int N = randomInt(3, 12);
      const float radiusX = randomFloat(2, 5);
      const float radiusY = randomFloat(2, 5);
      const float phase = randomFloat(0, 2 * M_PI);
      for(int i = 0; i < N; ++i)
      {
        Vec2 v;
        v.x = cos(i * M_PI * 2 / N + phase) * radiusX;
        v.y = sin(i * M_PI * 2 / N + phase) * radiusY;
        obstaclePolygon.vertices.push_back(obstaclePolygon.center + v);
      }
    }

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    // draw selection
    {
      const auto hs = boxHalfSize * 0.95;
      auto& box = currentSelection == 0 ? boxStart : boxTarget;
      drawer->rect(box - hs, hs * 2, White);
    }

    // draw trajectory
    drawer->line(boxStart, boxTarget, White);

    // draw obstacles
    drawBox(drawer, obstacleBox.center, obstacleBox.halfSize, Yellow, "obstacle");

    for(int i = 0; i < (int)obstaclePolygon.vertices.size(); ++i)
    {
      auto v0 = obstaclePolygon.vertices[i];
      auto v1 = obstaclePolygon.vertices[(i + 1) % obstaclePolygon.vertices.size()];
      drawer->line(v0, v1, Yellow);
    }
    drawer->text(obstaclePolygon.center, "obstacle", Yellow);
    drawCross(drawer, obstaclePolygon.center, Yellow);

    // draw start, target, and finish positions
    drawBox(drawer, boxStart, boxHalfSize, Green, "start");
    drawBox(drawer, boxTarget, boxHalfSize, Red, "target");
    drawBox(drawer, boxFinish, boxHalfSize, LightBlue, "finish");

    drawer->line(boxFinish, boxFinish + collisionNormal * 5.0, LightBlue);
  }

  void drawBox(IDrawer* drawer, Vec2 pos, Vec2 halfSize, Color color, const char* name)
  {
    drawer->rect(pos - halfSize, halfSize * 2, color);
    drawCross(drawer, pos, color);
    drawer->text(pos, name, color);
  }

  void drawCross(IDrawer* drawer, Vec2 pos, Color color)
  {
    drawer->line(pos - Vec2{1, 0}, pos + Vec2{1, 0}, color);
    drawer->line(pos - Vec2{0, 1}, pos + Vec2{0, 1}, color);
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

    IShape* obstacleShapes[] = {&obstaclePolygon, &obstacleBox};

    RaycastResult minRaycast;
    minRaycast.fraction = 1;

    for(auto obstacleShape : obstacleShapes)
    {
      // instead of sweeping a shape (mover) against another shape (obstacle),
      // we cast a ray against the minkowski sum of both shapes.
      CombinedShape combinedShape;
      combinedShape.shapeA = obstacleShape;
      combinedShape.shapeB = &moverShape;

      const auto result = raycast(boxStart, delta, &combinedShape);
      if(result.fraction < minRaycast.fraction)
        minRaycast = result;
    }

    boxFinish = boxStart + delta * minRaycast.fraction;
    collisionNormal = minRaycast.normal;
  }

  Vec2 boxHalfSize;

  Vec2 boxStart; // the starting position
  Vec2 boxTarget; // the target position
  Vec2 boxFinish; // the position we actually reach (=target if we don't hit the obstacle
  Vec2 collisionNormal{};

  BoxShape obstacleBox{};
  PolygonShape obstaclePolygon{};

  int currentSelection = 0;
};

const int registered = registerApp("SAT", []() -> IApp* { return new SeparatingAxisTestApp; });
}
