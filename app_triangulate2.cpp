// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Triangulation

#include "app.h"
#include "fiber.h"

#include <algorithm> // sort
#include <cassert>
#include <climits> // RAND_MAX
#include <cmath>
#include <cstdio>
#include <cstdlib> // rand
#include <memory>
#include <vector>

namespace
{
static float magnitude(Vec2 v) { return sqrt(v * v); }
static Vec2 normalize(Vec2 v) { return v * (1.0 / magnitude(v)); }
static Vec2 rotateLeft(Vec2 v) { return Vec2(-v.y, v.x); }
template<typename T>
T lerp(T a, T b, float alpha) { return a * (1.0f - alpha) + b * alpha; }

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

struct Edge
{
  int a, b;
};

float det2d(Vec2 a, Vec2 b)
{
  return a.x * b.y - a.y * b.x;
}

struct HalfEdge
{
  int point;
  int nextEdge;
  int twin;
};

int nextEdgeOnHull(span<const HalfEdge> he, int edge)
{
  const int watchdog = edge;
  assert(he[edge].twin == -1);
  edge = he[edge].nextEdge;

  while(he[edge].twin != -1)
  {
    edge = he[he[edge].twin].nextEdge;
    assert(edge != watchdog); // infinite loop
  }

  return edge;
}

const int unittests_run = [] ()
  {
    // simple
    {
      std::vector<HalfEdge> he =
      {
        { 0, 1, -1 },
        { 1, 2, -1 },
        { 2, 4, -1 },
      };

      assert(1 == nextEdgeOnHull(he, 0));
      assert(2 == nextEdgeOnHull(he, 1));
    }

    // internal edge
    {
      // |          P3
      // |           |
      // |           |
      // |         E1|E2
      // |           |
      // |      E0   |  E3
      // |  P0 ---- P1 ----- P2
      //
      std::vector<HalfEdge> he =
      {
        /* E0 */ { 0, 1, -1 },
        /* E1 */ { 1, 1000, 2 },
        /* E2 */ { 3, 3, 1 },
        /* E3 */ { 1, 1001, -1 },
      };

      assert(3 == nextEdgeOnHull(he, 0));
    }

    return 0;
  }();

struct IVisualizer
{
  virtual ~IVisualizer() = default;

  virtual void begin() = 0;
  virtual void end() = 0;

  virtual void line(Vec2 a, Vec2 b) = 0;
};

std::vector<Edge> triangulate(span<const Vec2> points, IVisualizer* vis)
{
  std::vector<HalfEdge> he;
  std::vector<int> pointToEdge(points.len, -1);

  if(points.len < 3)
    return {};

  auto addHalfEdge = [&] (int p0, int nextEdge) -> int
    {
      const int edge = (int)he.size();
      pointToEdge[p0] = edge;
      he.push_back({ p0, -1, -1 });
      he[edge].nextEdge = nextEdge;
      return edge;
    };

  // bootstrap triangulation with first triangle
  {
    // make the triangle CCW if needed
    if(det2d(points[1] - points[0], points[2] - points[0]) > 0)
    {
      addHalfEdge(/*P*/ 0, /*E*/ 1);
      addHalfEdge(/*P*/ 1, /*E*/ 2);
      addHalfEdge(/*P*/ 2, /*E*/ 0);
    }
    else
    {
      addHalfEdge(/*P*/ 0, /*E*/ 1);
      addHalfEdge(/*P*/ 2, /*E*/ 2);
      addHalfEdge(/*P*/ 1, /*E*/ 0);
    }
  }

  int hullHead = 0;

  auto printHull = [&] ()
    {
      int k = 0;
      int edge = hullHead;
      fprintf(stderr, "[");

      do
      {
        fprintf(stderr, "E%d (P%d) ", edge, he[edge].point);
        edge = nextEdgeOnHull(he, edge);

        if(++k > 10)
        {
          fprintf(stderr, "... ");
          break;
        }
      }
      while (edge != hullHead);

      fprintf(stderr, "]\n");
    };

  auto drawHull = [&] ()
    {
      int k = 0;
      int edge = hullHead;

      do
      {
        const int prev = edge;
        edge = nextEdgeOnHull(he, edge);

        vis->line(points[he[edge].point], points[he[prev].point]);

        if(++k > 10)
        {
          break;
        }
      }
      while (edge != hullHead);
    };

  vis->begin();
  drawHull();
  vis->end();

  for(int idx = 3; idx < (int)points.len; ++idx)
  {
    auto p = points[idx];
    fprintf(stderr, "--- insertion of point P%d ---\n", idx);

    int hullCurr = hullHead;
    const int loopPoint = he[hullHead].point;

    do
    {
      const auto currHe = he[hullCurr];
      int hullNext = nextEdgeOnHull(he, hullCurr);
      const auto nextHe = he[hullNext];

      const auto a = points[currHe.point];
      const auto b = points[nextHe.point];

      fprintf(stderr, "   Considering edge E%d [P%d P%d]\n", hullCurr, currHe.point, nextHe.point);

      assert(currHe.twin == -1); // make sure we stay on the hull

      if(det2d(p - a, b - a) > 0)
      {
        fprintf(stderr, "   Linking point\n");
        const int e0 = (int)he.size() + 0;
        const int e1 = (int)he.size() + 1;
        const int e2 = (int)he.size() + 2;

        he.push_back({});
        he.push_back({});
        he.push_back({});

        auto& edge0 = he[e0];
        auto& edge1 = he[e1];
        auto& edge2 = he[e2];

        // The edge we're about to link is going to become an internal edge.
        // If this is the edge we use as the head of the hull,
        // reassign the head of the hull to one of the two external edges
        // we're about to add.
        if(hullCurr == hullHead)
          hullHead = e1;

        // First, the edge that is shared with the current triangulation
        edge0.twin = hullCurr;
        he[hullCurr].twin = e0;
        edge0.point = he[hullNext].point;
        edge0.nextEdge = e1;

        // Then, the edges that will become part of the hull
        edge1.point = he[hullCurr].point;
        edge1.twin = -1;
        edge1.nextEdge = e2;

        edge2.point = idx;
        edge2.twin = -1;
        edge2.nextEdge = e0;

        printHull();

        vis->begin();
        drawHull();
        vis->end();
      }

      hullCurr = hullNext;
    }
    while (he[hullCurr].point != loopPoint);
  }

  std::vector<Edge> r;

  for(auto& e : he)
    r.push_back({ e.point, he[e.nextEdge].point });

  return r;
}

struct TriangulateApp : IApp
{
  TriangulateApp()
  {
    const int N = 7;
    m_points.resize(N);

    for(auto& p : m_points)
      p = randomPos();

    // sort points from left to right
    {
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
  }

  void draw(IDrawer* drawer) override
  {
    int idx = 0;

    for(auto& p : m_points)
    {
      drawer->rect(p - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
      char buffer[16];
      sprintf(buffer, "P%d", idx);
      drawer->text(p + Vec2(0.3, 0), buffer, Red);
      idx++;
    }

    idx = 0;

    for(auto& edge : m_edges)
    {
      auto center = (m_points[edge.a] + m_points[edge.b]) * 0.5;
      auto N = normalize(m_points[edge.b] - m_points[edge.a]);
      auto T = rotateLeft(N);

      drawer->line(m_points[edge.a], m_points[edge.b], Green);

      for(int i = 0; i < 10; ++i)
      {
        auto pos = lerp(m_points[edge.a], m_points[edge.b], i / 10.0f);
        drawer->line(pos, pos - N * 0.25 + T * 0.25, Green);
      }

      char buffer[16];
      sprintf(buffer, "E%d", idx);
      drawer->text(center + T * 1.0, buffer, Green);
      idx++;
    }

    for(auto& line : m_visu.m_lines)
      drawer->line(line.a, line.b, Yellow);
  }

  void keydown(Key key) override
  {
    if(key == Key::Space)
    {
      if(!m_fiber)
        m_fiber = std::make_unique<Fiber>(&staticTriangulateFromFiber, this);

      m_fiber->resume();
    }
  }

  static void staticTriangulateFromFiber(void* userParam)
  {
    auto pThis = (TriangulateApp*)userParam;
    pThis->triangulateFromFiber();
  }

  void triangulateFromFiber()
  {
    m_edges = triangulate({ m_points.size(), m_points.data() }, &m_visu);
  }

  struct Visualizer : IVisualizer
  {
    struct VisualLine
    {
      Vec2 a, b;
    };

    std::vector<VisualLine> m_lines;

    void line(Vec2 a, Vec2 b) override
    {
      m_lines.push_back({ a, b });
    }

    void begin() override
    {
      m_lines.clear();
    }

    void end() override
    {
      Fiber::yield();
    }
  };

  Visualizer m_visu;
  std::unique_ptr<Fiber> m_fiber;
  std::vector<Vec2> m_points;
  std::vector<Edge> m_edges;
};
const int registered = registerApp("triangulate2", [] () -> IApp* { return new TriangulateApp; });
}

