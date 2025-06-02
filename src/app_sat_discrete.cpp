// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Discrete collision detection using the separating axis test.

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cmath>
#include <cstdio>
#include <vector>

#include "random.h"

namespace
{

struct ConvexPolygon
{
  std::vector<Vec2> vertices; // local ref
  Vec2 pos; // world ref
};

ConvexPolygon createSquare()
{
  ConvexPolygon r{};

  r.vertices.push_back({-1.5, -1.5});
  r.vertices.push_back({+1.5, -1.5});
  r.vertices.push_back({+1.5, +1.5});
  r.vertices.push_back({-1.5, +1.5});

  return r;
}

ConvexPolygon randomConvexPolygon()
{
  ConvexPolygon r;

  const int N = randomInt(3, 12);
  const float radiusX = randomFloat(2, 5);
  const float radiusY = randomFloat(2, 5);
  const float phase = randomFloat(0, 2 * M_PI);
  for(int i = 0; i < N; ++i)
  {
    Vec2 v;
    v.x = cos(i * M_PI * 2 / N + phase) * radiusX;
    v.y = sin(i * M_PI * 2 / N + phase) * radiusY;
    r.vertices.push_back(v);
  }

  r.pos = randomPos({-10, -5}, {+10, 5});

  return r;
}

struct Collision
{
  float depth;
  Vec2 normal;
  Vec2 deepestPoints[2];
};

struct Interval
{
  float min, max;
};

Interval projectPolygon(const ConvexPolygon& polygon, Vec2 axis)
{
  Interval r;
  r.max = r.min = dotProduct(polygon.pos + polygon.vertices[0], axis);
  for(auto& v : polygon.vertices)
  {
    const float projV = dotProduct(polygon.pos + v, axis);
    r.min = std::min(r.min, projV);
    r.max = std::max(r.max, projV);
  }
  return r;
}

Collision collidePolygons(const ConvexPolygon& a, const ConvexPolygon& b)
{
  Collision r;

  r.depth = 1.0 / 0.0;

  std::vector<Vec2> axisToTest;

  auto addAxisForPolygon = [&axisToTest](const ConvexPolygon& polygon)
  {
    const int N = (int)polygon.vertices.size();
    for(int i = 0; i < N; ++i)
    {
      auto v0 = polygon.vertices[(i + 0) % N];
      auto v1 = polygon.vertices[(i + 1) % N];
      auto axis = rotateLeft(normalize(v1 - v0));
      axisToTest.push_back(axis);
    }
  };

  addAxisForPolygon(a);
  addAxisForPolygon(b);

  for(auto axis : axisToTest)
  {
    const Interval projA = projectPolygon(a, axis);
    const Interval projB = projectPolygon(b, axis);

    Collision c{};

    const auto middleA = (projA.min + projA.max) * 0.5f;
    const auto middleB = (projB.min + projB.max) * 0.5f;

    if(middleA < middleB)
    {
      c.depth = projA.max - projB.min;
      c.normal = axis;
    }
    else
    {
      c.depth = projB.max - projA.min;
      c.normal = -axis;
    }

    if(c.depth < r.depth)
      r = c;
  }

  r.deepestPoints[0] = a.vertices[0];
  for(auto p : a.vertices)
  {
    const auto depth = dotProduct(p, r.normal);
    if(depth > dotProduct(r.deepestPoints[0], r.normal))
      r.deepestPoints[0] = p;
  }
  r.deepestPoints[1] = b.vertices[0];
  for(auto p : b.vertices)
  {
    const auto depth = dotProduct(p, r.normal);
    if(depth < dotProduct(r.deepestPoints[1], r.normal))
      r.deepestPoints[1] = p;
  }
  return r;
}

struct DiscreteSatApp : IApp
{
  DiscreteSatApp()
  {
    colliderA = createSquare();
    colliderB = createSquare();
    colliderA = randomConvexPolygon();
    colliderB = randomConvexPolygon();

    colliderA.pos = {-4, 0};
    colliderB.pos = {+4, 0};

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    drawPolygon(drawer, colliderA, White, "A");
    drawPolygon(drawer, colliderB, White, "B");

    const auto deepestPointA = colliderA.pos + collision.deepestPoints[0];
    drawer->line(deepestPointA, deepestPointA - collision.depth * collision.normal, Red);

    const auto deepestPointB = colliderB.pos + collision.deepestPoints[1];
    drawer->line(deepestPointB, deepestPointB + collision.depth * collision.normal, Red);

    {
      Vec2 pos { -10, -10 };
      drawCross(drawer, pos, Red);
      drawer->line(pos, pos + collision.normal, Red);

      char buf[256];
      sprintf(buf, "depth=%.3f", collision.depth);
      drawer->text(pos + Vec2{0, -1}, buf);
    }
  }

  void drawPolygon(IDrawer* drawer, const ConvexPolygon& polygon, Color color, const char* name)
  {
    drawCross(drawer, polygon.pos, color);
    drawer->text(polygon.pos + Vec2{.7, .7}, name, color);
    const int N = (int)polygon.vertices.size();
    for(int i = 0; i < N; ++i)
    {
      auto v0 = polygon.vertices[(i + 0) % N];
      auto v1 = polygon.vertices[(i + 1) % N];
      drawer->line(polygon.pos + v0, polygon.pos + v1, color);
    }
  }

  void drawBox(IDrawer* drawer, Vec2 pos, Vec2 halfSize, Color color, const char* name)
  {
    drawer->rect(pos - halfSize, halfSize * 2, color);
    drawCross(drawer, pos, color);
    drawer->text(pos, name, color);
  }

  void drawCross(IDrawer* drawer, Vec2 pos, Color color)
  {
    drawer->line(pos - Vec2{0.5, 0}, pos + Vec2{0.5, 0}, color);
    drawer->line(pos - Vec2{0, 0.5}, pos + Vec2{0, 0.5}, color);
  }

  void processEvent(InputEvent inputEvent) override
  {
    if(inputEvent.pressed)
      keydown(inputEvent.key);
  }

  void keydown(Key key)
  {
    auto& collider = selection ? colliderA : colliderB;
    switch(key)
    {
    case Key::Left:
      collider.pos.x -= 0.3;
      break;
    case Key::Right:
      collider.pos.x += 0.3;
      break;
    case Key::Up:
      collider.pos.y += 0.3;
      break;
    case Key::Down:
      collider.pos.y -= 0.3;
      break;
    case Key::Space:
      selection = !selection;
      break;
    default:
      break;
    }

    compute();
  }

  void compute() { collision = collidePolygons(colliderA, colliderB); }

  bool selection = false;
  Collision collision{};
  ConvexPolygon colliderA;
  ConvexPolygon colliderB;
};

const int registered = registerApp("App.SAT.Discrete", []() -> IApp* { return new DiscreteSatApp; });
}
