// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Continuous collision detection using the separating axis test.
//
// A convex shape (the "mover") is swept along a vector 'delta',
// until it hits another (static) convex shape (the "obstacle").
//
// To do this, the minkowski sum of both shapes is constructed
// (in an implicit form, see CombinedShape).
// A ray is then casted along the vector 'delta'. This ray represents the
// displacement of the origin of the "mover" shape, which stops when it hits
// the boundary of the minkowski sum of "mover" and "obstacle".
//|                                                                            |
//|    _____                                                                   |
//|   |     |"delta"                                                           |
//|   |  x--|------->   .                                                      |
//|   |_____|          / \                                                     |
//|                   /   \                                                    |
//|   "mover"        /     \                                                   |
//|                 /   x   \                                                  |
//|                /_________\                                                 |
//|                                                                            |
//|                "obstacle"                                                  |
//|                                                                            |
//|                                                                            |
//|                   _____                                                    |
//|         "delta" /|     |\                                                  |
//|      x---------->|  .  | \                                                 |
//|               /  |_/_\_|  \                                                |
//|              /    /   \    \                                               |
//|             /____/     \____\                                              |
//|             |   / | x | \   |                                              |
//|             |  /__|___|__\  |                                              |
//|             |_____|___|_____|                                              |
//|                                                                            |
//|    minkowsi sum of "mover" and "obstacle"                                  |

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
  virtual ProjectionOnAxis projectOnAxis(Vec3 axis) const = 0;
  virtual std::vector<Vec3> axesToTest() const = 0;
  virtual std::vector<Vec3> edgesToCombine() const = 0;
};

struct RaycastResult
{
  float fraction = 1.0;
  Vec3 normal{};
};

// Cast a ray from 'pos' to 'pos+delta', stops when colliding with 'obstacle'.
// The return value represents the allowed move, as a fraction of the desired move (delta).
// (The final position is then 'pos + return_value * delta')
RaycastResult raycast(Vec3 pos, Vec3 delta, const IShape* obstacle)
{
  auto axes = obstacle->axesToTest();

  // axes.push_back(rotateLeft(normalize(delta)));

  RaycastResult r;
  r.fraction = 0;

  float leaveFraction = 1;

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

      float fMax = (projectedObstacle.max - startPos) / (targetPos - startPos);
      if(startPos <= projectedObstacle.max && targetPos > projectedObstacle.max)
      {
        // trace is leaving the shape on this axis
        if(fMax < leaveFraction)
          leaveFraction = fMax;
      }
    }
  }

  if(leaveFraction < r.fraction)
    return {};

  return r;
}

struct AffineTransformShape : IShape
{
  ProjectionOnAxis projectOnAxis(Vec3 axis) const override
  {
    Vec3 scaledAxis = {axis.x * scale.x, axis.y * scale.y, axis.z * scale.z};
    ProjectionOnAxis r = sub->projectOnAxis(scaledAxis);

    const auto transOnAxis = axis * translate;

    r.min += transOnAxis;
    r.max += transOnAxis;

    assert(r.min <= r.max);

    return r;
  }

  std::vector<Vec3> axesToTest() const override
  {
    // axis are normal vectors: transform them using inverse(transposed(transform))
    std::vector<Vec3> r = sub->axesToTest();
    for(auto& v : r)
    {
      v.x /= scale.x;
      v.y /= scale.y;
      v.z /= scale.z;
      v = normalize(v);
    }
    return r;
  }

  std::vector<Vec3> edgesToCombine() const override
  {
    // edges are normal vectors: transform them using inverse(transposed(transform))
    std::vector<Vec3> r = sub->edgesToCombine();
    for(auto& v : r)
    {
      v.x /= scale.x;
      v.y /= scale.y;
      v.z /= scale.z;
      v = normalize(v);
    }
    return r;
  }

  const IShape* sub;
  Vec3 translate = {};
  Vec3 scale = {1, 1, 1};
};

// an [-1;+1]x[-1;+1]x[-1;+1] AABB
struct BoxShape : IShape
{
  ProjectionOnAxis projectOnAxis(Vec3 axis) const override
  {
    ProjectionOnAxis r;

    const float projectedExtent = fabs(axis.x) + fabs(axis.y) + fabs(axis.z);

    r.min = -projectedExtent;
    r.max = +projectedExtent;

    return r;
  }

  std::vector<Vec3> axesToTest() const override { return {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; }
  std::vector<Vec3> edgesToCombine() const override { return {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; }
};

static const BoxShape boxShape;

struct ConvexPolyhedronShape : IShape
{
  std::vector<Vec3> vertices;
  std::vector<int> faces;

  ProjectionOnAxis projectOnAxis(Vec3 axis) const override
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

  std::vector<Vec3> axesToTest() const override
  {
    std::vector<Vec3> r;
    for(int i = 0; i < (int)faces.size(); i += 3)
    {
      auto v0 = vertices[faces[i + 0]];
      auto v1 = vertices[faces[i + 1]];
      auto v2 = vertices[faces[i + 2]];
      r.push_back(normalize(crossProduct(v1 - v0, v2 - v0)));
    }
    return r;
  }

  std::vector<Vec3> edgesToCombine() const override
  {
    std::vector<Vec3> r;
    for(int i = 0; i < (int)faces.size(); i += 3)
    {
      auto v0 = vertices[faces[i + 0]];
      auto v1 = vertices[faces[i + 1]];
      auto v2 = vertices[faces[i + 2]];
      r.push_back(normalize(v1-v0));
      r.push_back(normalize(v2-v1));
      r.push_back(normalize(v0-v2));
    }
    return r;
  }
};

// Minkowski sum of two shapes
struct CombinedShape : IShape
{
  IShape* shapeA;
  IShape* shapeB;

  ProjectionOnAxis projectOnAxis(Vec3 axis) const override
  {
    ProjectionOnAxis projA = shapeA->projectOnAxis(axis);
    ProjectionOnAxis projB = shapeB->projectOnAxis(axis);

    ProjectionOnAxis r;
    r.min = projA.min + projB.min;
    r.max = projA.max + projB.max;

    return r;
  }

  std::vector<Vec3> axesToTest() const override
  {
    std::vector<Vec3> r;
    for(auto& axis : shapeA->axesToTest())
      r.push_back(axis);
    for(auto& axis : shapeB->axesToTest())
      r.push_back(axis);

    for(auto& edgeA : shapeA->edgesToCombine())
    {
      for(auto& edgeB : shapeB->edgesToCombine())
      {
        auto axis = crossProduct(edgeA, edgeB);
        if(axis * axis > 0.001)
          r.push_back(normalize(axis));
      }
    }
    return r;
  }

  std::vector<Vec3> edgesToCombine() const override
  {
    std::vector<Vec3> r;
    for(auto& axis : shapeA->edgesToCombine())
      r.push_back(axis);
    for(auto& axis : shapeB->edgesToCombine())
      r.push_back(axis);

    return r;
  }
};

struct SeparatingAxisTestApp3D : IApp
{
  SeparatingAxisTestApp3D()
  {
    boxHalfSize = randomPos({1, 1, 1}, {3, 3, 3});
    boxStart = randomPos({-20, -10, 0}, {20, 10, 0});
    boxTarget = randomPos({-20, -10, 0}, {20, 10, 0});

    {
      obstacleBoxHalfSize = randomPos({2, 20, 2}, {5, 5, 3});
      obstacleBoxCenter = randomPos({5, -5, 0}, {15, 5, 0});
    }

    {
      const Vec3 center = randomPos({-25, -5, 0}, {-5, 5, 10});

      const int N = 3;//randomInt(3, 12);
      const float radiusX = randomFloat(4, 9);
      const float radiusY = randomFloat(4, 9);
      const float phase = randomFloat(0, 2 * M_PI);
      obstaclePolyhedron.vertices.push_back(center + Vec3(0, 0, randomFloat(3, 10)));
      for(int i = 0; i < N; ++i)
      {
        Vec3 v;
        v.x = cos(i * M_PI * 2 / N + phase) * radiusX;
        v.y = sin(i * M_PI * 2 / N + phase) * radiusY;
        v.z = 0;
        obstaclePolyhedron.vertices.push_back(center + v);

        obstaclePolyhedron.faces.push_back(0);
        obstaclePolyhedron.faces.push_back(1 + (i + 0) % N);
        obstaclePolyhedron.faces.push_back(1 + (i + 1) % N);
      }
        obstaclePolyhedron.faces.push_back(3);
        obstaclePolyhedron.faces.push_back(2);
        obstaclePolyhedron.faces.push_back(1);
    }

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    // draw selection
    //  {
    //    const auto hs = boxHalfSize * 0.95;
    //    auto& box = currentSelection == 0 ? boxStart : boxTarget;
    //    drawer->rect(box - hs, hs * 2, White);
    //  }

    // draw trajectory
    drawer->line(boxStart, boxTarget, White);

    // draw obstacles
    drawBox(drawer, obstacleBoxCenter, obstacleBoxHalfSize, Yellow, "obstacle");

    // draw polyhedron
    {
      for(int i = 0; i < (int)obstaclePolyhedron.faces.size(); i += 3)
      {
        auto v0 = obstaclePolyhedron.vertices[obstaclePolyhedron.faces[i + 0]];
        auto v1 = obstaclePolyhedron.vertices[obstaclePolyhedron.faces[i + 1]];
        auto v2 = obstaclePolyhedron.vertices[obstaclePolyhedron.faces[i + 2]];
        drawer->line(v0, v1, Yellow);
        drawer->line(v1, v2, Yellow);
        drawer->line(v2, v0, Yellow);
      }

      Vec3 center{};
      for(int i = 0; i < (int)obstaclePolyhedron.vertices.size(); ++i)
        center += obstaclePolyhedron.vertices[i];
      center = center * (1.0 / obstaclePolyhedron.vertices.size());
      // drawer->text(center, "obstacle", Yellow);
      drawCross(drawer, center, Yellow);
    }

    // draw start, target, and finish positions
    drawBox(drawer, boxStart, boxHalfSize, Green, "start");
    drawBox(drawer, boxTarget, boxHalfSize, Red, "target");
    drawBox(drawer, boxFinish, boxHalfSize, LightBlue, "finish");

    drawer->line(boxFinish, boxFinish + collisionNormal * 5.0, LightBlue);
  }

  void drawBox(IDrawer* drawer, Vec3 pos, Vec3 halfSize, Color color, const char* name)
  {
    auto v0 = pos - Vec3{-halfSize.x, -halfSize.y, -halfSize.z};
    auto v1 = pos - Vec3{-halfSize.x, +halfSize.y, -halfSize.z};
    auto v2 = pos - Vec3{+halfSize.x, +halfSize.y, -halfSize.z};
    auto v3 = pos - Vec3{+halfSize.x, -halfSize.y, -halfSize.z};

    auto v4 = pos - Vec3{-halfSize.x, -halfSize.y, +halfSize.z};
    auto v5 = pos - Vec3{-halfSize.x, +halfSize.y, +halfSize.z};
    auto v6 = pos - Vec3{+halfSize.x, +halfSize.y, +halfSize.z};
    auto v7 = pos - Vec3{+halfSize.x, -halfSize.y, +halfSize.z};

    drawer->line(v0, v1, color);
    drawer->line(v1, v2, color);
    drawer->line(v2, v3, color);
    drawer->line(v3, v0, color);

    drawer->line(v4, v5, color);
    drawer->line(v5, v6, color);
    drawer->line(v6, v7, color);
    drawer->line(v7, v4, color);

    drawer->line(v0, v4, color);
    drawer->line(v1, v5, color);
    drawer->line(v2, v6, color);
    drawer->line(v3, v7, color);

    drawCross(drawer, pos, color);
    (void)name;
    // drawer->text(pos, name, color);
  }

  void drawCross(IDrawer* drawer, Vec3 pos, Color color)
  {
    drawer->line(pos + Vec3{-1, 0, 0}, pos + Vec3{+1, 0, 0}, color);
    drawer->line(pos + Vec3{0, -1, 0}, pos + Vec3{0, +1, 0}, color);
    drawer->line(pos + Vec3{0, 0, -1}, pos + Vec3{0, 0, +1}, color);
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
    case Key::PageUp:
      box.z += 1;
      break;
    case Key::PageDown:
      box.z -= 1;
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
    const auto delta = boxTarget - boxStart;

    AffineTransformShape moverShape{};
    moverShape.sub = &boxShape;
    moverShape.scale = boxHalfSize;

    AffineTransformShape obstacleBoxShape{};
    obstacleBoxShape.sub = &boxShape;
    obstacleBoxShape.scale = obstacleBoxHalfSize;
    obstacleBoxShape.translate = obstacleBoxCenter;

    IShape* obstacleShapes[] = {&obstaclePolyhedron, &obstacleBoxShape};

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

  Vec3 boxHalfSize;

  Vec3 boxStart; // the starting position
  Vec3 boxTarget; // the target position
  Vec3 boxFinish; // the position we actually reach (=target if we don't hit the obstacle
  Vec3 collisionNormal{};

  Vec3 obstacleBoxCenter;
  Vec3 obstacleBoxHalfSize;
  ConvexPolyhedronShape obstaclePolyhedron{};

  int currentSelection = 0;
};

const int registered =
      registerApp("CollisionDetection/SAT/Continuous3D", []() -> IApp* { return new SeparatingAxisTestApp3D; });
}
