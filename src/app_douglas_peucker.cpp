// Copyright (C) 2021 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Douglas-Peucker algorithm : polyline simplification.

#include <cstdio> // snprintf
#include <vector>

#include "algorithm_app.h"
#include "random.h"

namespace
{
struct DouglasPeuckerAlgorithm
{
  struct Segment
  {
    int a, b;
  };

  static std::vector<Vec2> generateInput()
  {
    std::vector<Vec2> points;
    const int N = int(randomFloat(3, 15));
    const float length = 40.0f;

    for(int i = 0; i < N; ++i)
    {
      Vec2 pos;
      pos.x = -length / 2 + length * i / N;
      pos.y = randomFloat(-10, 10);
      points.push_back(pos);
    }

    return points;
  }

  static std::vector<Segment> execute(std::vector<Vec2> input)
  {
    std::vector<Segment> result;
    // TODO
    gVisualizer->step();
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

      const int next_idx = (idx + 1);
      if (next_idx < input.size())
        drawer->line(input[idx], input[next_idx]);
    }
  }

  static void drawOutput(IDrawer* drawer, const std::vector<Vec2>& input, const std::vector<Segment>& output)
  {
    for(auto& segment : output)
      drawer->line(input[segment.a], input[segment.b], Green);
  }
};

const int reg = registerApp("DouglasPeucker", [] () -> IApp* { return new AlgorithmApp<DouglasPeuckerAlgorithm>; });
}
