// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Segment/Segment intersection

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cmath>

#include "random.h"

namespace
{
float sqr(float v) { return v * v; }

float sqrMagnitude(Vec2 a) { return dotProduct(a, a); }

bool segmentsIntersect(Vec2 u0, Vec2 u1, Vec2 v0, Vec2 v1, float toleranceRadius, Vec2& where)
{
  // raycast of u0->u1 against the "wall" v0v1
  const Vec2 tangent = normalize(v1 - v0);
  const Vec2 normal = rotateLeft(tangent);

  // project everybody on this normal
  const float pos0 = dotProduct(normal, u0);
  const float pos1 = dotProduct(normal, u1);
  const float posWall = dotProduct(normal, v0);

  if(pos0 > posWall + toleranceRadius && pos1 > posWall + toleranceRadius)
    return false;

  if(pos0 < posWall - toleranceRadius && pos1 < posWall - toleranceRadius)
    return false;

  float fraction;

  // parallel segments
  if(fabs(pos1 - pos0) < toleranceRadius)
  {
    if(fabs(pos0 - posWall) > toleranceRadius)
      return false; // not colinear enough

    const float uMin = std::min(dotProduct(tangent, u0), dotProduct(tangent, u1));
    const float uMax = std::max(dotProduct(tangent, u0), dotProduct(tangent, u1));

    const float vMin = std::min(dotProduct(tangent, v0), dotProduct(tangent, v1));
    const float vMax = std::max(dotProduct(tangent, v0), dotProduct(tangent, v1));

    if(uMax < vMin - toleranceRadius || uMin > vMax + toleranceRadius)
      return false; // no common point

    // now, there are 2 possibilites:
    // 1) uMin is less than vMin-toleranceRadius
    // 2) uMin is between vMin-toleranceRadius and vMax+toleranceRadius
    if(uMin < vMin - toleranceRadius)
    {
      fraction = (vMin - uMin) / (uMax - uMin); // common point at vMin
    }
    else
    {
      fraction = 0; // uMin is between vMin and vMax.
                    // choose uMin as our common point.
    }
  }
  else
  {
    fraction = (posWall - pos0) / (pos1 - pos0);
  }

  if(fraction < -toleranceRadius || fraction > 1 + toleranceRadius)
    return false;

  // intersection of u0u1 with the line v0v1
  where = u0 + fraction * (u1 - u0);

  // check if the intersection belongs to the segment v0v1
  if(dotProduct(where - v0, tangent) < -toleranceRadius)
    return false;

  if(dotProduct(where - v1, tangent) > +toleranceRadius)
    return false;

  // we exclude the circle centered on u1, with a radius of toleranceRadius.
  if(sqrMagnitude(where - u1) < sqr(toleranceRadius))
    return false;

  // we exclude the circle centered on v1, with a radius of toleranceRadius.
  if(sqrMagnitude(where - v1) < sqr(toleranceRadius))
    return false;

  return true;
}

Vec2 randomPosition()
{
  Vec2 r;
  r.x = randomFloat(-10, 10);
  r.y = randomFloat(-10, 10);
  return r;
}

struct SegmentIntersectionApp : IApp
{
  SegmentIntersectionApp()
  {
    for(auto& p : points)
      p = randomPosition();

    points[0] = {-5, 0.2};
    points[1] = {+5, 0.2};
    points[2] = {-10, 0};
    points[3] = {+10, 0};

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    drawer->line(points[0], points[1], m_doIntersect ? Red : White);
    drawer->line(points[2], points[3], m_doIntersect ? Red : White);

    for(auto p : points)
      drawer->circle(p, m_toleranceRadius, White);

    if(m_doIntersect)
    {
      drawer->line(m_where - Vec2(0.4, 0), m_where + Vec2(0.4, 0), Green);
      drawer->line(m_where - Vec2(0, 0.4), m_where + Vec2(0, 0.4), Green);
    }

    // draw selected point
    drawer->circle(points[m_cur], 0.3, Yellow);
  }

  void processEvent(InputEvent inputEvent) override
  {
    if(inputEvent.pressed)
      keydown(inputEvent.key);
  }

  void keydown(Key key)
  {
    const auto speed = 0.1;

    switch(key)
    {
    case Key::Left:
      points[m_cur].x -= speed;
      break;
    case Key::Right:
      points[m_cur].x += speed;
      break;
    case Key::Up:
      points[m_cur].y += speed;
      break;
    case Key::Down:
      points[m_cur].y -= speed;
      break;
    case Key::PageUp:
      m_toleranceRadius *= 1.01;
      break;
    case Key::PageDown:
      m_toleranceRadius *= 1.0 / 1.01;
      break;
    case Key::Space:
      m_cur = (m_cur + 1) % 4;
      break;
    default:
      break;
    }

    compute();
  }

  void compute()
  {
    m_doIntersect = segmentsIntersect(points[0], points[1], points[2], points[3], m_toleranceRadius, m_where);
  }

  int m_cur = 0;
  Vec2 points[4];
  Vec2 m_where{};
  float m_toleranceRadius = 0.5;
  bool m_doIntersect = false;
};

const int registered = registerApp("Intersection/Segments", []() -> IApp* { return new SegmentIntersectionApp; });
}
