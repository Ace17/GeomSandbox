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

#include "app.h"
#include "bounding_box.h"
#include "drawer.h"
#include "geom.h"
#include "random.h"

namespace
{

float abs(float a) { return a >= 0 ? a : -a; }
float magnitude(Vec2 v) { return sqrt(v * v); }
Vec2 normalize(Vec2 v) { return v * (1.0 / magnitude(v)); }
Vec2 rotateLeft(Vec2 v) { return Vec2(-v.y, v.x); }

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

struct ThickLineApp : IApp
{
  ThickLineApp()
  {
    const int N = randomInt(4, 15);
    Vec2 pos{};
    for(int i = 0; i < N; ++i)
    {
      pos += randomPos({-4, -4}, {10, 4}) * 4;
      polyline.push_back(pos);
    }

    rescale(polyline, {-16, -16}, {16, 16});

    thickness = randomFloat(1, 3);
  }

  void draw(IDrawer* drawer) override
  {
    for(int i = 1; i < (int)polyline.size(); ++i)
    {
      auto v0 = polyline[i - 1];
      auto v1 = polyline[i];
      drawer->line(v0, v1, Yellow);
    }

    auto clamp = [](int val, int min, int max) {
      if(val < min)
        return min;
      if(val > max)
        return max;
      return val;
    };

    auto tangent = [&](int segmentIndex) {
      int i = clamp(segmentIndex, 0, polyline.size() - 2);
      return normalize(polyline[i + 1] - polyline[i]);
    };

    auto normal = [&](int segmentIndex) { return rotateLeft(tangent(segmentIndex)); };

    for(int i = 0; i + 1 < (int)polyline.size(); ++i)
    {
      auto v0 = polyline[i];
      auto v1 = polyline[i + 1];
      const auto N0 = normalize(normal(i) + normal(i - 1));
      const auto N1 = normalize(normal(i) + normal(i + 1));

      const auto L0 = v0 + N0 * thickness;
      const auto L1 = v1 + N1 * thickness;
      const auto R0 = v0 - N0 * thickness;
      const auto R1 = v1 - N1 * thickness;
      drawer->line(L0, L1, Red);
      drawer->line(L0, R0, Red);
      drawer->line(R0, R1, Red);
      drawer->line(L1, R1, Red);
    }
  }

  float thickness = 0;
  std::vector<Vec2> polyline;
};

const int registered = registerApp("ThickLine", []() -> IApp* { return new ThickLineApp; });
}
