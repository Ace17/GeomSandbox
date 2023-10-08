// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Clip a polyline against a AABB

#include "core/algorithm_app.h"
#include "core/sandbox.h"

#include <cmath>
#include <cstdio> // snprintf
#include <vector>

#include "random.h"

namespace
{
struct ClipPolylineAlgorithm
{
  struct Segment
  {
    int a, b;
  };

  struct Input
  {
    std::vector<Vec2> line;
    Vec2 boxMin, boxMax;
  };

  struct Output
  {
  };

  static Input generateInput()
  {
    Input r;
    const int N = rand() % 10 + 5;

    for(int i = 0; i < N; ++i)
      r.line.push_back(randomPos(Vec2(-30, -16), Vec2(30, 16)));

    for(int i = 1; i < N; ++i)
    {
      auto d = r.line[i] - r.line[i - 1];
      r.line[i] -= d * 0.25;
      r.line[i - 1] += d * 0.25;
    }

    r.boxMin = randomPos(Vec2(-25, -15), Vec2(25, 15));
    r.boxMax = randomPos(Vec2(-25, -15), Vec2(25, 15));

    auto order = [](float& a, float& b)
    {
      if(a > b)
        std::swap(a, b);
    };

    order(r.boxMin.x, r.boxMax.x);
    order(r.boxMin.y, r.boxMax.y);

    return r;
  }

  static int execute(Input input)
  {
    const int N = input.line.size();
    std::vector<Vec2> poly;
    for(int i = 0; i < N; ++i)
    {
      poly.push_back(input.line[i]);

      for(int idx = 0; idx < int(poly.size()) - 1; ++idx)
        sandbox_line(poly[idx], poly[idx + 1], Green);

      sandbox_circle(input.line[i], 0.3, Green);
      sandbox_breakpoint();
    }
    return 0;
  }

  static void display(Input input, int output)
  {
    (void)input;
    (void)output;

    for(int idx = 0; idx < int(input.line.size()) - 1; ++idx)
    {
      // char buffer[16];
      // snprintf(buffer, sizeof buffer, "%d", idx);
      // sandbox_rect(input[idx] - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
      // sandbox_text(input[idx] + Vec2(0.3, 0), buffer, Red);

      sandbox_line(input.line[idx], input.line[idx + 1]);
    }

    sandbox_rect(input.boxMin, input.boxMax - input.boxMin);

    //  for(auto& segment : output)
    //    sandbox_line(input[segment.a], input[segment.b], Green);
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<ClipPolylineAlgorithm>>()); }

const int reg = registerApp("App.ClipPolyline", &create);
}
