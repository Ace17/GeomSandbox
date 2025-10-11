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
Vec2 rotateRight(Vec2 v) { return Vec2(v.y, -v.x); }

struct Face
{
  Vec2 points[2]; // local ref
  Vec2 normal;
};

struct ConvexPolygon
{
  std::vector<Face> faces;
  Vec2 pos; // world ref
};

void addFace(ConvexPolygon& polygon, Vec2 a, Vec2 b)
{
  Face f;
  f.points[0] = a;
  f.points[1] = b;
  f.normal = rotateRight(normalize(b - a));
  polygon.faces.push_back(f);
}

ConvexPolygon createSquare()
{
  const Vec2 a = {-1.5, -1.5};
  const Vec2 b = {+1.5, -1.5};
  const Vec2 c = {+1.5, +1.5};
  const Vec2 d = {-1.5, +1.5};

  ConvexPolygon r{};

  addFace(r, a, b);
  addFace(r, b, c);
  addFace(r, c, d);
  addFace(r, d, a);

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
    Vec2 a;
    a.x = cos((i + 0) * M_PI * 2 / N + phase) * radiusX;
    a.y = sin((i + 0) * M_PI * 2 / N + phase) * radiusY;

    Vec2 b;
    b.x = cos((i + 1) * M_PI * 2 / N + phase) * radiusX;
    b.y = sin((i + 1) * M_PI * 2 / N + phase) * radiusY;

    addFace(r, a, b);
  }

  r.pos = randomPos({-10, -5}, {+10, 5});

  return r;
}

struct Collision
{
  float depth;
  Vec2 normal;
  Vec2 contactPoint;
};

struct Interval
{
  float min, max;
};

Interval projectPolygon(const ConvexPolygon& polygon, Vec2 axis)
{
  Interval r;
  r.max = r.min = dotProduct(polygon.pos + polygon.faces[0].points[0], axis);
  for(auto& face : polygon.faces)
  {
    for(auto& v : face.points)
    {
      const float projV = dotProduct(polygon.pos + v, axis);
      r.min = std::min(r.min, projV);
      r.max = std::max(r.max, projV);
    }
  }
  return r;
}

Vec2 findContactPoint(const ConvexPolygon& a, const ConvexPolygon& b, Vec2 normal)
{
  const ConvexPolygon* referent = &a;
  const ConvexPolygon* incident = &b;

  float best = 0;
  float sign = 1;

  // find the face most parallel to the normal: this is the "reference face"
  for(auto& face : a.faces)
  {
    const float value = dotProduct(face.normal, normal);
    if(value > best)
    {
      best = value;
    }
  }

  {
    for(auto& face : b.faces)
    {
      const float value = dotProduct(face.normal, -normal);
      if(value > best)
      {
        best = value;
        sign = -1;
        referent = &b;
        incident = &a;
      }
    }
  }

  (void)referent;

  Vec2 pos;
  // find the deepest point of 'incident' which is inside 'referent'.
  float maxDepth = -1.0 / 0.0;
  for(auto& face : incident->faces)
  {
    for(auto& v : face.points)
    {
      float depth = dotProduct(-normal, incident->pos + v) * sign;
      if(depth > maxDepth)
      {
        maxDepth = depth;
        pos = incident->pos + v;
      }
    }
  }

  return pos;
}

Collision collidePolygons(const ConvexPolygon& a, const ConvexPolygon& b)
{
  Collision r;

  r.depth = 1.0 / 0.0;

  std::vector<Vec2> axisToTest;

  for(auto& face : a.faces)
    axisToTest.push_back(face.normal);

  for(auto& face : b.faces)
    axisToTest.push_back(face.normal);

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

  r.contactPoint = findContactPoint(a, b, r.normal);
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

    const auto contactPoint = collision.contactPoint;
    drawer->line(contactPoint, contactPoint + collision.depth * collision.normal, Red);

    {
      Vec2 pos{-10, -10};
      drawCross(drawer, pos, Red);
      drawer->line(pos, pos + collision.normal, Red);

      char buf[256];
      sprintf(buf, "depth=%.3f", collision.depth);
      drawer->text(pos + Vec2{0, -1}, buf);
    }

    const auto separatingPlaneP0 = collision.contactPoint - rotateRight(collision.normal) * 100;
    const auto separatingPlaneP1 = collision.contactPoint + rotateRight(collision.normal) * 100;
    drawer->line(separatingPlaneP0, separatingPlaneP1, Yellow);
  }

  void drawPolygon(IDrawer* drawer, const ConvexPolygon& polygon, Color color, const char* name)
  {
    drawCross(drawer, polygon.pos, color);
    drawer->text(polygon.pos + Vec2{.7, .7}, name, color);

    for(auto& face : polygon.faces)
    {
      auto v0 = face.points[0];
      auto v1 = face.points[1];
      drawer->line(polygon.pos + v0, polygon.pos + v1, color);

      auto p0 = (polygon.pos + v0 + polygon.pos + v1) * 0.5;
      auto p1 = p0 + face.normal * 0.5;
      drawer->line(p0, p1, Red);
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

const int registered = registerApp("CollisionDetection/SAT/Discrete", []() -> IApp* { return new DiscreteSatApp; });
}
