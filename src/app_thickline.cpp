// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Tesselate a polyline

#include <cassert>
#include <cmath>
#include <vector>

#include "algorithm_app.h"
#include "bounding_box.h"
#include "drawer.h"
#include "geom.h"
#include "random.h"

namespace
{

void rescale(std::vector<Vec2>& polyline, Vec2 mins, Vec2 maxs)
{
  BoundingBox bbox;
  for(auto& p : polyline)
    bbox.add(p);

  const float xScale = (maxs.x - mins.x) / (bbox.max.x - bbox.min.x);
  const float yScale = (maxs.y - mins.y) / (bbox.max.y - bbox.min.y);

  for(auto& p : polyline)
  {
    p.x = (p.x - bbox.min.x) * xScale + mins.x;
    p.y = (p.y - bbox.min.y) * yScale + mins.y;
  }
}

struct ThickLineAlgorithm
{
  struct Input
  {
    std::vector<Vec2> polyline;
    float thickness;
  };

  struct Segment
  {
    Vec2 a, b;
  };

  static Input generateInput()
  {
    Input input;

    const int N = randomInt(4, 15);
    Vec2 pos{};
    for(int i = 0; i < N; ++i)
    {
      pos += randomPos({-4, -4}, {10, 4}) * 4;
      input.polyline.push_back(pos);
    }

    rescale(input.polyline, {-16, -16}, {16, 16});

    input.thickness = randomFloat(1, 3);
    return input;
  }

  static std::vector<Segment> execute(Input input)
  {
    auto& polyline = input.polyline;
    auto clamp = [](int val, int min, int max)
    {
      if(val < min)
        return min;
      if(val > max)
        return max;
      return val;
    };

    auto tangent = [&](int segmentIndex)
    {
      int i = clamp(segmentIndex, 0, polyline.size() - 2);
      return normalize(polyline[i + 1] - polyline[i]);
    };

    auto normal = [&](int segmentIndex) { return rotateLeft(tangent(segmentIndex)); };

    std::vector<Segment> segments;

    Vec2 N = normal(0);
    Vec2 L = polyline[0] + N * input.thickness;
    Vec2 R = polyline[0] - N * input.thickness;

    for(int i = 1; i < (int)polyline.size(); ++i)
    {
      auto v1 = polyline[i];
      const auto N1 = normalize(N + normal(i));

      const auto L0 = L;
      const auto L1 = v1 + N1 * input.thickness;
      const auto R0 = R;
      const auto R1 = v1 - N1 * input.thickness;

      const int i0 = (int)segments.size();
      segments.push_back({L0, L1});
      segments.push_back({L0, R0});
      segments.push_back({R0, R1});
      segments.push_back({L1, R1});

      L = L1;
      R = R1;
      N = N1;

      for(int i = i0; i < (int)segments.size(); ++i)
        sandbox_line(segments[i].a, segments[i].b, Red);
      sandbox_breakpoint();
    }

    return segments;
  }

  static void display(const Input& input, const std::vector<Segment>& output)
  {
    for(int i = 1; i < (int)input.polyline.size(); ++i)
    {
      auto v0 = input.polyline[i - 1];
      auto v1 = input.polyline[i];
      sandbox_line(v0, v1, Yellow);
    }

    for(auto& s : output)
      sandbox_line(s.a, s.b, Green);
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<ThickLineAlgorithm>>()); }
const int registered = registerApp("ThickLine", &create);
}
