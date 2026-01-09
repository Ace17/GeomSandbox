// Copyright (C) 2026 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// build a basic triangulation by sweeping the input points left-to-right
#include "triangulate_basic.h"

#include "core/geom.h"
#include "core/sandbox.h"

#include <algorithm>
#include <map>
#include <vector>

namespace
{
const bool EnableTrace = true;

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

void printHull(const std::vector<int>& hull, span<const Vec2> points, int head)
{
  int curr = head;

  do
  {
    int next = hull[curr];
    sandbox_line(points[curr], points[next]);
    curr = next;
  } while(curr != head);

  sandbox_line(points[head] + Vec2(-1, -1), points[head] + Vec2(+1, +1));
  sandbox_line(points[head] + Vec2(-1, +1), points[head] + Vec2(+1, -1));

  sandbox_breakpoint();
}

std::vector<int> sortPointsFromLeftToRight(span<const Vec2> points)
{
  std::vector<int> order(points.len);

  for(int i = 0; i < (int)points.len; ++i)
    order[i] = i;

  auto byCoordinates = [&](int ia, int ib)
  {
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

}

std::vector<HalfEdge> createBasicTriangulation(span<const Vec2> points)
{
  std::vector<HalfEdge> he;
  std::map<std::pair<int, int>, int> pointToEdge;

  auto findHalfEdge = [&](int a, int b)
  {
    auto i = pointToEdge.find({a, b});

    if(i == pointToEdge.end())
      return -1;

    return i->second;
  };

  const auto order = sortPointsFromLeftToRight(points);
  span<const int> queue = order;

  std::vector<int> hull(points.len);
  int hullHead = 0;

  if(points.len < 3)
    return {};

  // bootstrap triangulation with first edge
  {
    int i0 = queue.pop();
    int i1 = queue.pop();

    hull[i0] = i1;
    hull[i1] = i0;
    hullHead = i0;
  }

  if(EnableTrace)
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
        {
          const int p0 = hullCurr;
          const int p1 = idx;
          const int p2 = hullNext;

          const auto e0 = (int)he.size() + 0;
          const auto e1 = (int)he.size() + 1;
          const auto e2 = (int)he.size() + 2;
          he.push_back({p0, e1});
          he.push_back({p1, e2});
          he.push_back({p2, e0});

          // search for a twin for e0 (=the edge that links [p0 -> p1])
          he[e0].twin = findHalfEdge(p1, p0);
          he[e1].twin = findHalfEdge(p2, p1);
          he[e2].twin = findHalfEdge(p0, p2);

          if(he[e0].twin != -1)
            he[he[e0].twin].twin = e0;
          if(he[e1].twin != -1)
            he[he[e1].twin].twin = e1;
          if(he[e2].twin != -1)
            he[he[e2].twin].twin = e2;

          pointToEdge[{p0, p1}] = e0;
          pointToEdge[{p1, p2}] = e1;
          pointToEdge[{p2, p0}] = e2;
        }

        hull[idx] = hullNext;
        hull[hullCurr] = idx;
        hullHead = idx;

        if(EnableTrace)
        {
          for(auto edge : he)
            sandbox_line(points[edge.point], points[he[edge.next].point], Gray);
          printHull(hull, points, hullHead);
        }
      }

      hullCurr = hullNext;
    } while(hullCurr != hullFirst);

    queue += 1;
  }

  return he;
}
