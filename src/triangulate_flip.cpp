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

struct HalfEdge
{
  int point;
  int next;
  int twin = -1;
};

std::vector<HalfEdge> convertToHalfEdge(span<const Vec2> points, span<const Triangle> triangles)
{
  std::vector<HalfEdge> he;
  std::map<std::pair<int, int>, int> pointToEdge;

  auto findHalfEdge = [&] (int a, int b)
    {
      auto i = pointToEdge.find({ a, b });

      if(i == pointToEdge.end())
        return -1;

      return i->second;
    };

  for(auto& t : triangles)
  {
    const auto e0 = (int)he.size() + 0;
    const auto e1 = (int)he.size() + 1;
    const auto e2 = (int)he.size() + 2;
    he.push_back({ t.a, e1 });
    he.push_back({ t.b, e2 });
    he.push_back({ t.c, e0 });

    // search for a twin for e0 (=the edge that links [t.a -> t.b])
    he[e0].twin = findHalfEdge(t.b, t.a);
    he[e1].twin = findHalfEdge(t.c, t.b);
    he[e2].twin = findHalfEdge(t.a, t.c);

    pointToEdge[{ t.a, t.b }] = e0;
    pointToEdge[{ t.b, t.c }] = e1;
    pointToEdge[{ t.c, t.a }] = e2;
  }

  return he;
}

std::vector<Edge> flipTriangulation(span<const Vec2> points, span<HalfEdge> he)
{
  std::vector<int> stack;
  stack.reserve(he.len);

  for(int i = 0; i < (int)he.len; ++i)
    if(i > he[i].twin) // of both twins, only push one
      stack.push_back(i);

  while(!stack.empty())
  {
    // |              .
    // |             /|\
    // |         L1 / | \ R2
    // |           /  |  \
    // |          /   |   \
    // |          \  E|   /
    // |           \  |  /
    // |         L2 \ | / R1
    // |             \|/
    // |              .
    const auto E = stack.back();
    stack.pop_back();

    const auto L1 = he[E].next;
    const auto L2 = he[L1].next;

    const auto R1 = he[he[E].twin].next;
    const auto R2 = he[L1].next;

    {
      visu->begin();

      for(auto edge : he)
        visu->line(points[edge.point], points[he[edge.next].point]);

      visu->end();
    }
  }

  std::vector<Edge> r;

  return r;
}
} // namespace

std::vector<Edge> triangulate_Flip(span<const Vec2> points)
{
  auto triangles = createBasicTriangulation(points);
  auto he = convertToHalfEdge(points, triangles);

  return flipTriangulation(points, he);
}

