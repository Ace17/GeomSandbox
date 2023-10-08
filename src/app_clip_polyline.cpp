// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Clip a polyline against a AABB

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <algorithm>
#include <cmath>
#include <cstdio> // snprintf
#include <vector>

#include "random.h"

namespace
{

struct Input
{
  std::vector<Vec2> line;
  Vec2 boxMin, boxMax;
};

using Output = std::vector<std::vector<Vec2>>;

Input generateInput(int /*seed*/)
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

struct RaycastResult
{
  float fraction;
};

struct Segment
{
  float min, max;
};

Segment intersectSegment(Segment a, Segment b)
{
  // Swap if need, so 'a' is the leftmost segment
  if(a.min > b.min)
    std::swap(a, b);

  if(a.max < b.min)
    return {1, 0}; // empty segment

  // b.min is inside [a.min, a.max]
  if(b.max < a.max)
    return b;

  return {b.min, a.max};
}

Vec2 lerp(Vec2 a, Vec2 b, float alpha) { return a * (1 - alpha) + b * alpha; }

Segment clipSegmentToAABB(Vec2 a, Vec2 b, Vec2 boxMin, Vec2 boxMax)
{
  Segment ratios = {0, 1};

  {
    const float sa = (boxMin.x - a.x) / (b.x - a.x);
    const float sb = (boxMax.x - a.x) / (b.x - a.x);
    const Segment ratioRangeAlongX{std::min(sa, sb), std::max(sa, sb)};
    sandbox_printf("along X : min=%.2f max=%.2f\n", ratioRangeAlongX.min, ratioRangeAlongX.max);

    ratios = intersectSegment(ratios, ratioRangeAlongX);
  }

  {
    const float sa = (boxMin.y - a.y) / (b.y - a.y);
    const float sb = (boxMax.y - a.y) / (b.y - a.y);
    const Segment ratioRangeAlongY{std::min(sa, sb), std::max(sa, sb)};
    sandbox_printf("along Y : min=%.2f max=%.2f\n", ratioRangeAlongY.min, ratioRangeAlongY.max);

    ratios = intersectSegment(ratios, ratioRangeAlongY);
  }

  sandbox_printf("result : min=%.2f max=%.2f\n", ratios.min, ratios.max);
  return ratios;
}

Output execute(Input input)
{
  std::vector<std::vector<Vec2>> result;
  std::vector<Vec2> polyline;

  for(int i = 0; i + 1 < (int)input.line.size(); ++i)
  {
    const auto p0 = input.line[i + 0];
    const auto p1 = input.line[i + 1];

    Segment ratios = clipSegmentToAABB(p0, p1, input.boxMin, input.boxMax);

    if(ratios.min < ratios.max)
    {
      const Vec2 c0 = lerp(p0, p1, ratios.min);
      const Vec2 c1 = lerp(p0, p1, ratios.max);

      if(polyline.empty())
        polyline.push_back(c0);
      polyline.push_back(c1);

      sandbox_line(p0, c0, Red);
      sandbox_line(c0, c1, Green);
      sandbox_line(c1, p1, Red);
    }

    if(ratios.max != 1)
    {
      if(polyline.size())
        result.push_back(std::move(polyline));
    }

    sandbox_breakpoint();
  }
  if(polyline.size())
    result.push_back(std::move(polyline));
  return result;
}

void display(Input input, Output output)
{
  for(int idx = 0; idx < int(input.line.size()) - 1; ++idx)
    sandbox_line(input.line[idx], input.line[idx + 1]);

  sandbox_rect(input.boxMin, input.boxMax - input.boxMin);

  for(auto& polyline : output)
  {
    for(int idx = 0; idx < int(polyline.size()) - 1; ++idx)
      sandbox_line(polyline[idx], polyline[idx + 1], Green);
    for(auto p : polyline)
      sandbox_circle(p, 0, Green, 8);
  }
}

inline static const TestCase<Input, Output> AllTestCases[] = {
      {
            "basic",
            {
                  {
                        {-15, 1},
                        {-4, -4},
                        {+2, +6},
                        {+10, -1},
                  },
                  {-5, -5},
                  {+6, +5},
            },
      },
      {
            "vertical segments",
            {
                  {
                        {-15, 0},
                        {-15, 7},
                        {+15, 7},
                        {+15, 0},
                  },
                  {-5, -5},
                  {+6, +5},
            },
      },
};
BEGIN_ALGO("Clipping/Polyline", execute)
WITH_INPUTGEN(generateInput)
WITH_TESTCASES(AllTestCases)
WITH_DISPLAY(display)
END_ALGO
}
