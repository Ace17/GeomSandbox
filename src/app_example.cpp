// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Example algorithm: split a convex polygon into triangles

#include <cmath>
#include <cstdio> // snprintf
#include <vector>

#include "algorithm_app.h"
#include "random.h"

namespace
{
struct ExampleAlgorithm
{
  struct Segment
  {
    int a, b;
  };

  static std::vector<Vec2> generateInput()
  {
    std::vector<Vec2> points;
    const int N = int(randomFloat(3, 15));
    const float radiusX = randomFloat(5, 15);
    const float radiusY = randomFloat(5, 15);

    for(int i = 0; i < N; ++i)
    {
      const float angle = 3.14 * 2 * i / N;
      Vec2 pos;
      pos.x = cos(angle) * radiusX;
      pos.y = sin(angle) * radiusY;
      points.push_back(pos);
    }

    return points;
  }

  static std::vector<Segment> execute(std::vector<Vec2> input)
  {
    std::vector<Segment> result;

    for(int i = 2; i + 1 < (int)input.size(); ++i)
    {
      result.push_back({0, i});

      gVisualizer->line(input[0], input[i]);
      gVisualizer->step();
    }

    return result;
  }

  static void drawInput(IDrawer* drawer, const std::vector<Vec2>& input)
  {
    for(int idx = 0; idx < input.size(); ++idx)
    {
      char buffer[16];
      snprintf(buffer, sizeof buffer, "%d", idx);
      drawer->rect(input[idx] - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
      drawer->text(input[idx] + Vec2(0.3, 0), buffer, Red);

      const int next_idx = (idx + 1) % input.size();
      drawer->line(input[idx], input[next_idx]);
    }
  }

  static void drawOutput(IDrawer* drawer, const std::vector<Vec2>& input, const std::vector<Segment>& output)
  {
    for(auto& segment : output)
      drawer->line(input[segment.a], input[segment.b], Green);
  }
};

const int reg = registerApp("Example", []() -> IApp* { return new AlgorithmApp<ExampleAlgorithm>; });
}