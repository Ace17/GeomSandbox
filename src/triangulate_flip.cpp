// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Triangulation, Flip algorithm

#include "geom.h"
#include "triangulate.h"
#include "visualizer.h"
#include <algorithm>
#include <map>
#include <vector>

namespace
{
float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

void printHull(std::map<int, int> hull, span<const Vec2> points, int head)
{
  visu->begin();

  int curr = head;

  do
  {
    int next = hull[curr];
    visu->line(points[curr], points[next]);
    curr = next;
  }
  while (curr != head);

  visu->line(points[head] + Vec2(-1, -1), points[head] + Vec2(+1, +1));
  visu->line(points[head] + Vec2(-1, +1), points[head] + Vec2(+1, -1));

  visu->end();
}

struct Triangle
{
  int a, b, c;
};

std::vector<int> sortPointsFromLeftToRight(span<const Vec2> points)
{
  std::vector<int> order(points.len);

  for(int i = 0; i < points.len; ++i)
    order[i] = i;

  auto byCoordinates = [&] (int ia, int ib) {
      auto a = points[ia];
      auto b = points[ib];

      if(a.x != b.x)
        return a.x < b.x;

      if(a.y != b.y)
        return a.y < b.y;

      return true;
    };

  std::sort(order.begin(), order.end(), byCoordinates);

  return order;
}

std::vector<Triangle> createBasicTriangulation(span<const Vec2> points)
{
  const auto order = sortPointsFromLeftToRight(points);
  span<const int> queue = order;

  std::vector<Triangle> triangles;
  std::map<int, int> hull;

  if(points.len < 3)
    return {};

  // bootstrap triangulation with first triangle
  {
    int i0 = queue.pop();
    int i1 = queue.pop();
    int i2 = queue.pop();

    // make the triangle CCW if needed
    if(det2d(points[i1] - points[i0], points[i2] - points[i0]) < 0)
      std::swap(i1, i2);

    triangles.push_back({ i0, i1, i2 });

    hull[i0] = i1;
    hull[i1] = i2;
    hull[i2] = i0;
  }

  int hullHead = hull.begin()->first;

  printHull(hull, points, hullHead);

  while(queue.len > 0)
  {
    const int idx = queue[0];
    const auto p = points[idx];

    // recompute hullHead so its on the left of the hull
    while(1)
    {
      auto a = points[hullHead];
      auto b = points[hull[hullHead]];

      if(det2d(p - a, b - a) <= 0)
        break;

      hullHead = hull[hullHead];
    }

    const int hullFirst = hullHead;
    int hullCurr = hullFirst;

    do
    {
      const int hullNext = hull[hullCurr];

      const auto a = points[hullCurr];
      const auto b = points[hullNext];

      if(det2d(p - a, b - a) > 0.001)
      {
        triangles.push_back({ hullCurr, idx, hullNext });

        hull[idx] = hullNext;
        hull[hullCurr] = idx;
        hullHead = idx;

        printHull(hull, points, hullHead);
      }

      hullCurr = hullNext;
    }
    while (hullCurr != hullFirst);

    queue += 1;
  }

  return triangles;
}
} // namespace

std::vector<Edge> triangulate_Flip(span<const Vec2> points)
{
  auto triangles = createBasicTriangulation(points);

  std::vector<Edge> r;

  for(auto& t : triangles)
  {
    r.push_back({ t.a, t.b });
    r.push_back({ t.b, t.c });
    r.push_back({ t.c, t.a });
  }

  return r;
}

