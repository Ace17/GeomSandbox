// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Triangulation

#include "app.h"
#include "fiber.h"
#include "visualizer.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <vector>

struct Edge
{
  int a, b;
};

std::vector<Edge> triangulateMine_BowyerWatson(span<const Vec2> points);

struct NullVisualizer : IVisualizer
{
  void begin() override {};
  void end() override {};
  void line(Vec2, Vec2) override {};
};

static NullVisualizer nullVisualizer;
IVisualizer* visu = &nullVisualizer;

namespace
{
// reorder 'elements' so that it can be split into two parts:
// - [0 .. r[ : criteria is false
// - [r .. N[ : criteria is true
template<typename T, typename Lambda>
int split(span<T> elements, Lambda predicate)
{
  int result = elements.len;

  for(int i = 0; i < result;)
  {
    if(predicate(elements[i]))
      std::swap(elements[i], elements[--result]);
    else
      ++i;
  }

  return result;
}

const int unittests_run = [] ()
  {
    // no-op
    {
      auto isGreaterThan10 = [] (int val) { return val > 10; };
      std::vector<int> elements = { 4, 5, 3, 4, 5, 3 };
      int r = split<int>(elements, isGreaterThan10);
      assert(6 == r);
    }

    // simple
    {
      auto isEven = [] (int val) { return val % 2 == 0; };
      std::vector<int> elements = { 1, 2, 3, 4, 5, 6, 7 };
      int r = split<int>(elements, isEven);
      assert(4 == r);
      assert(!isEven(elements[0]));
      assert(!isEven(elements[1]));
      assert(!isEven(elements[2]));
      assert(!isEven(elements[3]));
      assert(isEven(elements[4]));
      assert(isEven(elements[5]));
      assert(isEven(elements[6]));
    }

    return 0;
  }();

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

    if(0)
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

    for(auto& edge : m_edges)
      drawer->line(m_points[edge.a], m_points[edge.b], Green);

    for(auto& p : m_points)
    {
      drawer->rect(p - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
      char buffer[16];
      sprintf(buffer, "%d", idx);
      drawer->text(p + Vec2(0.3, 0), buffer, Red);
      idx++;
    }

    for(auto& line : m_visu.m_lines)
      drawer->line(line.a, line.b, Yellow);
  }

  void triangulateFromFiber()
  {
    m_edges = triangulateMine_BowyerWatson({ m_points.size(), m_points.data() });
    fprintf(stderr, "Triangulated, %d edges\n", (int)m_edges.size());
  }

  static void staticTriangulateFromFiber(void* userParam)
  {
    ((TriangulateApp*)userParam)->triangulateFromFiber();
  }

  void keydown(Key key) override
  {
    if(key == Key::Space)
    {
      visu = &m_visu;

      if(!m_fiber)
        m_fiber = std::make_unique<Fiber>(staticTriangulateFromFiber, this);

      m_fiber->resume();
    }
  }

  std::vector<Vec2> m_points;
  std::vector<Edge> m_edges;
  std::unique_ptr<Fiber> m_fiber;

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
};
const int registered = registerApp("triangulate", [] () -> IApp* { return new TriangulateApp; });
}

