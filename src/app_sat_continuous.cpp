// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Continuous collision detection using the separating axis test.

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cassert>
#include <cmath>
#include <vector>

#include "random.h"

namespace
{

struct ProjectionOnAxis
{
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
RaycastResult raycast(Vec2 pos, Vec2 delta, const IShape* obstacle)
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

struct AffineTransformShape : IShape
{
  ProjectionOnAxis projectOnAxis(Vec2 axis) const override
  {
    Vec2 scaledAxis = {axis.x * scale.x, axis.y * scale.y};
    ProjectionOnAxis r = sub->projectOnAxis(scaledAxis);

    const auto transOnAxis = axis * translate;

    r.min += transOnAxis;
    r.max += transOnAxis;

    assert(r.min <= r.max);

    return r;
  }

  std::vector<Vec2> axesToTest() const override
  {
    // axis are normal vectors: transform them using inverse(transposed(transform))
    std::vector<Vec2> r = sub->axesToTest();
    for(auto& v : r)
    {
      v.x /= scale.x;
      v.y /= scale.y;
    }
    return r;
  }

  const IShape* sub;
  Vec2 translate = {};
  Vec2 scale = {1, 1};
};

// an [-1;+1]x[-1;+1] AABB
struct BoxShape : IShape
{
  ProjectionOnAxis projectOnAxis(Vec2 axis) const override
  {
    ProjectionOnAxis r;

    const float projectedExtent = fabs(axis.x) + fabs(axis.y);

    r.min = -projectedExtent;
    r.max = +projectedExtent;

    return r;
  }

  std::vector<Vec2> axesToTest() const override { return {{1, 0}, {0, 1}}; }
};

static const BoxShape boxShape;

struct PolygonShape : IShape
{
  std::vector<Vec2> vertices;

  ProjectionOnAxis projectOnAxis(Vec2 axis) const override
  {
    ProjectionOnAxis r;

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

// Minkowski sum of two shapes
struct CombinedShape : IShape
{
  IShape* shapeA;
  IShape* shapeB;

  ProjectionOnAxis projectOnAxis(Vec2 axis) const override
  {
    ProjectionOnAxis projA = shapeA->projectOnAxis(axis);
    ProjectionOnAxis projB = shapeB->projectOnAxis(axis);

    ProjectionOnAxis r;
    r.min = projA.min + projB.min;
    r.max = projA.max + projB.max;

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
      obstacleBoxHalfSize = randomPos({2, 2}, {5, 5});
      obstacleBoxCenter = randomPos({5, -5}, {15, 5});
    }

    {
      const Vec2 center = randomPos({-25, -5}, {-5, 5});

      const int N = randomInt(3, 12);
      const float radiusX = randomFloat(2, 5);
      const float radiusY = randomFloat(2, 5);
      const float phase = randomFloat(0, 2 * M_PI);
      for(int i = 0; i < N; ++i)
      {
        Vec2 v;
        v.x = cos(i * M_PI * 2 / N + phase) * radiusX;
        v.y = sin(i * M_PI * 2 / N + phase) * radiusY;
        obstaclePolygon.vertices.push_back(center + v);
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
    drawBox(drawer, obstacleBoxCenter, obstacleBoxHalfSize, Yellow, "obstacle");

    Vec2 center{};
    for(int i = 0; i < (int)obstaclePolygon.vertices.size(); ++i)
    {
      auto v0 = obstaclePolygon.vertices[i];
      auto v1 = obstaclePolygon.vertices[(i + 1) % obstaclePolygon.vertices.size()];
      drawer->line(v0, v1, Yellow);
      center += v0;
    }
    center = center * (1.0 / obstaclePolygon.vertices.size());
    drawer->text(center, "obstacle", Yellow);
    drawCross(drawer, center, Yellow);

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

  void processEvent(InputEvent inputEvent) override
  {
    if(inputEvent.pressed)
      keydown(inputEvent.key);
  }

  void keydown(Key key)
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
    default:
      break;
    }

    compute();
  }

  void compute()
  {
    const auto delta = Vec2(boxTarget - boxStart);

    AffineTransformShape moverShape{};
    moverShape.sub = &boxShape;
    moverShape.scale = boxHalfSize;

    AffineTransformShape obstacleBoxShape{};
    obstacleBoxShape.sub = &boxShape;
    obstacleBoxShape.scale = obstacleBoxHalfSize;
    obstacleBoxShape.translate = obstacleBoxCenter;

    IShape* obstacleShapes[] = {&obstaclePolygon, &obstacleBoxShape};

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

  Vec2 obstacleBoxCenter;
  Vec2 obstacleBoxHalfSize;
  PolygonShape obstaclePolygon{};

  int currentSelection = 0;
};

const int registered = registerApp("App.SAT.Continuous", []() -> IApp* { return new SeparatingAxisTestApp; });
}
