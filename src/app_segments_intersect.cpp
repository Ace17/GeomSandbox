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
#include <iterator>
#include <vector>

#include "random.h"

namespace
{

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

bool compare(Vec2 a, Vec2 b)
{
  if(a.x != b.x)
    return a.x < b.x;
  if(a.y != b.y)
    return a.y < b.y;
  return false;
}

bool onSegment(Vec2 u0, Vec2 u1, Vec2 p)
{
  // be invariant to the order of u0 and u1
  if(compare(u0, u1))
    std::swap(u0, u1);

  if(dotProduct(p - u0, p - u1) > 0)
    return false;
  return det2d(p - u0, u1 - u0) == 0;
}
}

bool segmentsIntersect(Vec2 u0, Vec2 u1, Vec2 v0, Vec2 v1, Vec2& where)
{
  // First, check for intersections on endpoints
  // (This also handles the case where the segments are parallel)
  // Beware, by convention:
  // - if both u0 and u1 are in contact with v, we return u0.
  // - if both v0 and v1 are in contact with u, we return v0.
  {
    if(onSegment(u0, u1, v0))
    {
      where = v0;
      return true;
    }
    if(onSegment(v0, v1, u0))
    {
      where = u0;
      return true;
    }
    if(onSegment(u0, u1, v1))
    {
      where = v1;
      return true;
    }
    if(onSegment(v0, v1, u1))
    {
      where = u1;
      return true;
    }
  }

  // Be invariant to the order of the input points
  {
    if(compare(u0, u1))
      std::swap(u0, u1);

    if(compare(v0, v1))
      std::swap(v0, v1);
  }

  // Determine if the infinite lines (u0,u1) and (v0,v1) intersect.
  {
    const int side_v0 = det2d(u1 - u0, v0 - u0) > 0 ? 1 : -1;
    const int side_v1 = det2d(u1 - u0, v1 - u0) > 0 ? 1 : -1;
    if(side_v0 == side_v1)
      return false; // the lines don't cross
  }

  // The infinite lines (u0,u1) and (v0,v1) intersect. Find where.
  const auto normal = rotateLeft(v1 - v0);

  const float pos_u0 = dotProduct(normal, u0);
  const float pos_u1 = dotProduct(normal, u1);
  const float pos_wall = dotProduct(normal, v0);

  const float fraction = (pos_wall - pos_u0) / (pos_u1 - pos_u0);
  where = u0 + (u1 - u0) * fraction;

  if(fraction > 1 || fraction < 0)
    return false; // the intersection is outside [u0,u1]

  return true;
}

struct Crossing
{
  int i;
  Vec2 where;
};

std::vector<Crossing> computeSegmentVsPolylineIntersections(Vec2 u0, Vec2 u1, span<const Vec2> polyline)
{
  std::vector<Crossing> r;

  for(int i = 0; i + 1 < (int)polyline.len; ++i)
  {
    Vec2 where;
    if(segmentsIntersect(u0, u1, polyline[i], polyline[i + 1], where))
    {
      if(!(where == polyline[i + 1]))
        r.push_back({i, where});
    }
  }

  return r;
}

namespace
{
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
    points.resize(10);

    for(auto& p : points)
      p = randomPosition();

    {
      points[0] = {-12.5, 0};
      points[1] = {+10, 0};

      points[2] = {0, -10};
      points[3] = {0, 0};
      points[4] = {0, 10};
    }

    {
      points[0] = {0, -5};
      points[1] = {0, +5};

      points[2] = {0, -10};
      points[3] = {0, +10};
      points[4] = {10, +10};
    }

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    drawer->line(points[0], points[1], Yellow);

    for(int i = 2; i + 1 < (int)std::size(points); ++i)
      drawer->line(points[i], points[i + 1], White);

    for(auto p : points)
      drawer->rect(p, {}, White, {8, 8});

    for(auto intersection : m_intersections)
    {
      drawer->line(intersection.where - Vec2(0.4, 0), intersection.where + Vec2(0.4, 0), Green);
      drawer->line(intersection.where - Vec2(0, 0.4), intersection.where + Vec2(0, 0.4), Green);

      drawer->line(points[2 + intersection.i], points[2 + intersection.i + 1], Red);
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
    const auto speed = 0.125;

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
    case Key::Return:
      std::swap(points[m_cur / 2], points[m_cur / 2 + 1]);
      break;
    case Key::Space:
      m_cur = (m_cur + 1) % std::size(points);
      break;
    default:
      break;
    }

    compute();
  }

  void compute()
  {
    span<const Vec2> polyline(std::size(points) - 2, points.data() + 2);
    m_intersections = computeSegmentVsPolylineIntersections(points[0], points[1], polyline);
  }

  int m_cur = 0;
  std::vector<Vec2> points;
  std::vector<Crossing> m_intersections;
};

const int registered =
      registerApp("Intersection/SegmentVsPolyline", []() -> IApp* { return new SegmentIntersectionApp; });
}
