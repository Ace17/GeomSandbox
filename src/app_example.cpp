// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Example algorithm: split a convex polygon into triangles

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <cassert>
#include <cmath>
#include <cstdio> // snprintf
#include <vector>

#include "random.h"

namespace
{

struct Segment
{
  int a, b;
};

std::vector<Vec2> generateConvexPolygon(int /*seed*/)
{
  std::vector<Vec2> points;
  const int N = int(randomFloat(10, 35));
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

std::vector<Segment> triangulateConvexPolygon(std::vector<Vec2> input)
{
  std::vector<Segment> result;

  for(int i = 2; i + 1 < (int)input.size(); ++i)
  {
    result.push_back({0, i});

    sandbox_line(input[0], input[i]);
    sandbox_breakpoint();
  }

  return result;
}

void display(span<const Vec2> input, span<const Segment> output)
{
  for(int idx = 0; idx < (int)input.len; ++idx)
  {
    char buffer[16];
    snprintf(buffer, sizeof buffer, "%d", idx);
    sandbox_rect(input[idx], {}, White, Vec2(8, 8));
    sandbox_text(input[idx], buffer, Red, Vec2(8, 0));

    const int next_idx = (idx + 1) % input.len;
    sandbox_line(input[idx], input[next_idx]);
  }

  for(auto& segment : output)
    sandbox_line(input[segment.a], input[segment.b], Green);
}

const TestCase<std::vector<Vec2>, span<const Segment>> allTestCases[] = {
      {
            "Basic",
            {
                  {-10, -10},
                  {+10, -10},
                  {+10, +10},
                  {-10, +10},
            },
            [](const span<const Segment>& result) { assert(result.len == 1); },
      },
      {
            "Intermediate",
            {
                  {0, 0},
                  {+10, 0},
                  {+9, 1},
                  {+7, 2},
                  {+4, 3},
                  {0, 3.5},
                  {-4, 3},
                  {-7, 2},
                  {-9, 1},
                  {-10, 0},
            },
            [](const span<const Segment>& result) { assert(result.len == 7); },
      },
};

BEGIN_ALGO("Demo/Example", triangulateConvexPolygon)
WITH_INPUTGEN(generateConvexPolygon)
WITH_TESTCASES(allTestCases)
WITH_DISPLAY(display)
END_ALGO
}
