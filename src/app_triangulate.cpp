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

#include <cassert>
#include <climits> // RAND_MAX
#include <cstdio>  // fprintf
#include <cstdlib> // rand
#include <memory>
#include <vector>

struct Edge
{
  int a, b;
};

std::vector<Edge> triangulate_BowyerWatson(span<const Vec2> points);
std::vector<Edge> triangulate_Flip(span<const Vec2> points);

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

struct TriangulateApp : IApp
{
  TriangulateApp()
  {
    const int N = 15;
    m_points.resize(N);

    for(auto& p : m_points)
      p = randomPos();
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
    if(0)
      m_edges = triangulate_BowyerWatson({ m_points.size(), m_points.data() });
    else
      m_edges = triangulate_Flip({ m_points.size(), m_points.data() });

    fprintf(stderr, "Triangulated, %d edges\n", (int)m_edges.size());

    // clear visualization
    m_visu.m_lines.clear();
  }

  static void staticTriangulateFromFiber(void* userParam)
  {
    ((TriangulateApp*)userParam)->triangulateFromFiber();
  }

  void keydown(Key key) override
  {
    if(key == Key::Space || key == Key::Return)
    {
      visu = &m_visu;

      if(!m_fiber)
        m_fiber = std::make_unique<Fiber>(staticTriangulateFromFiber, this);

      if(key == Key::Return)
      {
        while(!m_fiber->finished())
          m_fiber->resume();
      }
      else
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

    void line(Vec2 a, Vec2 b) override { m_lines.push_back({ a, b }); }

    void begin() override { m_lines.clear(); }

    void end() override { Fiber::yield(); }
  };

  Visualizer m_visu;
};
const int registered =
  registerApp("triangulate", [] () -> IApp* { return new TriangulateApp; });
} // namespace

