// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Random polygon generation

#include "app.h"
#include "fiber.h"
#include "visualizer.h"
#include "random.h"

#include <cassert>
#include <cmath>
#include <cstdio> // sprintf
#include <memory>
#include <vector>

namespace
{

Vec2 randomPos()
{
  Vec2 r;

  auto radius = randomFloat(0, 20);
  auto angle = randomFloat(0, 3.1415 * 2);
  r.x = cos(angle) * radius;
  r.y = sin(angle) * radius;
  return r;
}

struct PolygonApp : IApp
{
  PolygonApp()
  {
    const int N = 15;
    m_points.resize(N);

    for(auto& p : m_points)
      p = randomPos();
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

    for(auto& line : m_visu.m_lines)
      drawer->line(line.a, line.b, Yellow);
  }

  void runFromFiber()
  {
    // clear visualization
    m_visu.m_lines.clear();
  }

  static void staticRun(void* userParam)
  {
    ((PolygonApp*)userParam)->runFromFiber();
  }

  void keydown(Key key) override
  {
    if(key == Key::Space || key == Key::Return)
    {
      gVisualizer = &m_visu;

      if(!m_fiber)
        m_fiber = std::make_unique<Fiber>(staticRun, this);

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

    void step() override
    {
      Fiber::yield();
      m_lines.clear();
    }
  };

  Visualizer m_visu;
};
const int registered = registerApp("polygon", [] () -> IApp* { return new PolygonApp; });
}

