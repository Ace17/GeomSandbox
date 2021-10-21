// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Triangulation

#include "app.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
struct Edge
{
  int a, b;
};

/*
 * Bowyer-Watson algorithm
 * C++ implementation of http://paulbourke.net/papers/triangulate .
 **/
namespace BowyerWatson
{
constexpr double eps = 1e-4;

struct Point
{
  float x = 0;
  float y = 0;
  int index = -1;

  operator Vec2 () const
  {
    return { x, y };
  }

  bool operator == (const Point& other) const
  {
    return other.x == x && other.y == y;
  }
};

struct Edge
{
  Point p0, p1;

  bool operator == (const Edge& other) const
  {
    return (other.p0 == p0 && other.p1 == p1) ||
           (other.p0 == p1 && other.p1 == p0);
  }
};

struct Circle
{
  Vec2 center;
  float squaredRadius;

  bool isInside(Vec2 pt) const
  {
    const auto d = pt - center;
    const auto squaredDist = d.x * d.x + d.y * d.y;

    return (squaredDist - squaredRadius) <= eps;
  }
};

struct Triangle
{
  Point p0, p1, p2;
  Edge e0, e1, e2;
  Circle circle;

  Triangle(const Point& _p0, const Point& _p1, const Point& _p2)
    : p0{_p0},
    p1{_p1},
    p2{_p2},
    e0{_p0, _p1},
    e1{_p1, _p2},
    e2{_p0, _p2},
    circle{}
  {
    const auto ax = p1.x - p0.x;
    const auto ay = p1.y - p0.y;
    const auto bx = p2.x - p0.x;
    const auto by = p2.y - p0.y;

    const auto m = p1.x * p1.x - p0.x * p0.x + p1.y * p1.y - p0.y * p0.y;
    const auto u = p2.x * p2.x - p0.x * p0.x + p2.y * p2.y - p0.y * p0.y;
    const auto s = 1. / (2. * (ax * by - ay * bx));

    circle.center.x = ((p2.y - p0.y) * m + (p0.y - p1.y) * u) * s;
    circle.center.y = ((p0.x - p2.x) * m + (p1.x - p0.x) * u) * s;

    const auto dx = p0.x - circle.center.x;
    const auto dy = p0.y - circle.center.y;
    circle.squaredRadius = dx * dx + dy * dy;
  }
};

Triangle createEnclosingTriangle(span<const Point> points)
{
  auto xmin = points[0].x;
  auto xmax = xmin;
  auto ymin = points[0].y;
  auto ymax = ymin;

  for(auto const& pt : points)
  {
    xmin = std::min(xmin, pt.x);
    xmax = std::max(xmax, pt.x);
    ymin = std::min(ymin, pt.y);
    ymax = std::max(ymax, pt.y);
  }

  const auto dx = xmax - xmin;
  const auto dy = ymax - ymin;
  const auto dmax = std::max(dx, dy);
  const auto midx = (xmin + xmax) / 2.0f;
  const auto midy = (ymin + ymax) / 2.0f;

  const auto p0 = Point{ midx - 20 * dmax, midy - dmax };
  const auto p1 = Point{ midx, midy + 20 * dmax };
  const auto p2 = Point{ midx + 20 * dmax, midy - dmax };

  return Triangle{ p0, p1, p2 };
}

std::vector<::Edge> triangulate(span<const Point> points)
{
  if(points.len < 3)
    return {};

  std::vector<Triangle> triangles;

  // Init Delaunay triangulation with an enclosing triangle
  const Triangle enclosingTriangle = createEnclosingTriangle(points);
  triangles.emplace_back(enclosingTriangle);

  for(auto const& pt : points)
  {
    std::vector<Edge> edges;
    std::vector<Triangle> tmps;

    for(auto const& tri : triangles)
    {
      if(tri.circle.isInside(pt))
      {
        edges.push_back(tri.e0);
        edges.push_back(tri.e1);
        edges.push_back(tri.e2);
      }
      else
      {
        tmps.push_back(tri);
      }
    }

    /* Delete duplicate edges. */
    std::vector<bool> remove(edges.size(), false);

    for(auto it1 = edges.begin(); it1 != edges.end(); ++it1)
    {
      for(auto it2 = std::next(it1); it2 != edges.end(); ++it2)
      {
        if(*it1 == *it2)
        {
          remove[std::distance(edges.begin(), it1)] = true;
          remove[std::distance(edges.begin(), it2)] = true;
        }
      }
    }

    edges.erase(
      std::remove_if(edges.begin(), edges.end(),
                     [&] (auto const& e) { return remove[&e - &edges[0]]; }),
      edges.end());

    /* Update triangulation. */
    for(auto const& e : edges)
    {
      tmps.push_back({ e.p0, e.p1, { pt.x, pt.y, pt.index } });
    }

    triangles = tmps;
  }

  /* Remove original super triangle. */
  triangles.erase(
    std::remove_if(triangles.begin(), triangles.end(),
                   [&] (auto const& tri) {
        const auto p0 = enclosingTriangle.p0;
        const auto p1 = enclosingTriangle.p1;
        const auto p2 = enclosingTriangle.p2;
        return (tri.p0 == p0 || tri.p1 == p0 || tri.p2 == p0) ||
        (tri.p0 == p1 || tri.p1 == p1 || tri.p2 == p1) ||
        (tri.p0 == p2 || tri.p1 == p2 || tri.p2 == p2);
      }),
    triangles.end());

  std::vector<::Edge> r;

  for(auto const& tri : triangles)
  {
    r.push_back({ tri.e0.p0.index, tri.e0.p1.index });
    r.push_back({ tri.e1.p0.index, tri.e1.p1.index });
    r.push_back({ tri.e2.p0.index, tri.e2.p1.index });
  }

  return r;
}
} /* namespace BowyerWatson */

float randomFloat(float min, float max)
{
  return (rand() / float(RAND_MAX)) * (max - min) + min;
}

Vec2 randomPos()
{
  Vec2 r;
  r.x = randomFloat(-20, 20);
  r.y = randomFloat(-10, 10);
  return r;
}

float det2d(Vec2 a, Vec2 b)
{
  return a.x * b.y - a.y * b.x;
}

std::vector<Edge> triangulate(span<const Vec2> points)
{
  using namespace BowyerWatson;
  std::vector<Point> input;

  int i = 0;

  for(auto point : points)
    input.push_back({ point.x, point.y, i++ });

  auto r = BowyerWatson::triangulate(input);

  fprintf(stderr, "Triangulated, %d edges\n", (int)r.size());

  return r;
}

[[maybe_unused]]
std::vector<Edge> triangulateMine(span<const Vec2> points)
{
  std::vector<Edge> r;
  std::map<int, int> hull;

  if(points.len < 3)
    return {};

  // bootstrap triangulation with first triangle
  {
    int i0 = 0;
    int i1 = 1;
    int i2 = 2;

    // make the triangle CCW if needed
    if(det2d(points[i1] - points[i0], points[i2] - points[i0]) < 0)
      std::swap(i1, i2);

    r.push_back({ i0, i1 });
    r.push_back({ i1, i2 });
    r.push_back({ i2, i0 });

    hull[i0] = i1;
    hull[i1] = i2;
    hull[i2] = i0;
  }

  for(int idx = 3; idx < (int)points.len; ++idx)
  {
    auto p = points[idx];

    int hullFirst = hull.begin()->first;
    int hullCurr = hullFirst;

    do
    {
      const int hullNext = hull[hullCurr];

      const auto a = points[hullCurr];
      const auto b = points[hullNext];

      if(det2d(p - a, b - a) > 0)
      {
        r.push_back(Edge{ hullCurr, idx });
        r.push_back(Edge{ idx, hullNext });

        hull[idx] = hullNext;
        hull[hullCurr] = idx;
      }

      hullCurr = hullNext;
    }
    while (hullCurr != hullFirst);
  }

  return r;
}

struct TriangulateApp : IApp
{
  TriangulateApp()
  {
    const int N = 15;
    m_points.resize(N);

    for(auto& p : m_points)
      p = randomPos();

    auto byCoordinates = [] (Vec2 a, Vec2 b)
      {
        if(a.x != b.x)
          return a.x < b.x;

        if(a.y != b.y)
          return a.y < b.y;

        return true;
      };

    std::sort(m_points.begin(), m_points.end(), byCoordinates);
  }

  void draw(IDrawer* drawer) override
  {
    int idx = 0;

    for(auto& p : m_points)
    {
      drawer->rect(p - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
      char buffer[16];
      sprintf(buffer, "%d", idx);
      drawer->text(p + Vec2(0.3, 0), buffer, Red);
      idx++;
    }

    for(auto& edge : m_edges)
    {
      drawer->line(m_points[edge.a], m_points[edge.b], Green);
    }
  }

  void keydown(Key key) override
  {
    if(key == Key::Space)
      m_edges = triangulate({ m_points.size(), m_points.data() });
  }

  std::vector<Vec2> m_points;
  std::vector<Edge> m_edges;
};
const int registered = registerApp("triangulate", [] () -> IApp* { return new TriangulateApp; });
}

