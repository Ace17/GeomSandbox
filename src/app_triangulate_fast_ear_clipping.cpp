// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Polygon Triangulation: Ear-clipping Algorithm.
// ( based on: Ear-clipping Based Algorithms of Generating High-quality Polygon
// Triangulation, Gang Mei )

#include "core/algorithm_app.h"
#include "core/sandbox.h"

#include <cassert>
#include <cfloat>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "bounding_box.h"
#include "random.h"
#include "random_polygon.h"

const bool enableDisplay = true;

template<>
std::vector<Vec2> deserialize<std::vector<Vec2>>(span<const uint8_t> data)
{
  std::vector<Vec2> r;

  std::istringstream ss(std::string((const char*)data.ptr, data.len));
  std::string line;
  while(std::getline(ss, line))
  {
    std::istringstream lineSS(line);
    Vec2 v;
    char comma;
    if(lineSS >> v.x >> comma >> v.y)
      r.push_back(v);
  }

  if(r.back() == r.front())
    r.pop_back();

  // recenter polygon
  {
    BoundingBox box;
    for(auto& v : r)
      box.add(v);

    Vec2 translate = -(box.min + box.max) * 0.5;
    Vec2 scale;
    scale.x = 30.0 / (box.max.x - box.min.x);
    scale.y = 30.0 / (box.max.y - box.min.y);

    for(auto& v : r)
    {
      v = (v + translate);
      v.x *= scale.x;
      v.y *= scale.y;
    }
  }

  return r;
}

namespace
{

struct Segment
{
  int a, b;
};

struct Ear
{
  int tip;
  int a, b;
  float cosine;
};

void drawPolygon(span<const Vec2> input, Color color)
{
  const int N = input.len;
  for(int i = 0; i < N; ++i)
  {
    auto a = input[i];
    auto b = input[(i + 1) % N];
    auto middle = (a + b) * 0.5;
    auto normalTip = middle - rotateLeft(normalize(b - a)) * 0.3;
    sandbox_line(a, b, color);
    sandbox_line(middle, normalTip, {0.5, 0, 0, 1});
  }
}

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

struct FastEarClippingAlgorithm
{
  static std::vector<Vec2> generateInput()
  {
    const int N = rand() % 50 + 10;
    const float radius1 = randomFloat(5, 10);
    const float radius2 = radius1 + randomFloat(5, 10);

    std::vector<Vec2> r;

    for(int i = 0; i < N; ++i)
    {
      const float angle = 2 * M_PI * i / N;
      Vec2 u(cos(angle), sin(angle));

      const auto alpha = sin(angle * 8) * 0.5 + 0.5;
      const auto radius = alpha * radius1 + (1 - alpha) * radius2;
      r.push_back(u * radius);
    }

    return r;
  }

  static std::vector<Segment> execute(std::vector<Vec2> input)
  {
    span<const Vec2> polygon = input;
    const int N = polygon.len;

    std::vector<int> prev(N);
    std::vector<int> next(N);

    for(int i = 0; i < N; ++i)
    {
      prev[i] = (i - 1 + N) % N;
      next[i] = (i + 1 + N) % N;
    }

    // compute angles
    std::vector<float> angles(N);

    auto computeAngleAtVertex = [&](int i)
    {
      const auto A = polygon[prev[i]];
      const auto B = polygon[i];
      const auto C = polygon[next[i]];

      const auto lenAB = magnitude(B - A);
      const auto lenBC = magnitude(B - C);
      const auto mag = lenAB * lenBC;

      // degenerated angles are considered to be of zero value:
      // thus, they will be removed first from the polygon.
      if(lenAB <= 0.0001 || lenBC <= 0.00001)
      {
        angles[i] = 0;
        return;
      }

      const auto det = det2d(B - A, C - B) / mag;
      const auto dot = dotProduct(A - B, C - B) / mag;

      angles[i] = acos(dot);
      if(det < 0)
        angles[i] = 2 * M_PI - angles[i];
    };

    auto isValidEarTip = [&](int tip, int n)
    {
      const auto A = polygon[prev[tip]];
      const auto B = polygon[tip];
      const auto C = polygon[next[tip]];

      // test all other vertices, they must not be in the ABC triangle
      for(int i = 0, curr = next[next[tip]]; i + 3 < n; ++i, curr = next[curr])
      {
        if(polygon[curr] == A || polygon[curr] == B || polygon[curr] == C)
          continue;

        if(det2d(B - A, polygon[curr] - A) > 0 && det2d(C - B, polygon[curr] - B) > 0 &&
              det2d(A - C, polygon[curr] - C) > 0)
          return false; // point 'curr' is inside ABC.
      }

      return true;
    };

    for(int i = 0; i < N; ++i)
      computeAngleAtVertex(i);

    std::vector<Segment> result;

    int first = 0;
    int n = N;

    while(n > 3)
    {
      // find best ear to cut
      float minAngle = M_PI;
      int earIndex = -1;
      for(int i = 0, curr = first; i < n; ++i, curr = next[curr])
      {
        if(angles[curr] < minAngle && isValidEarTip(curr, n))
        {
          earIndex = curr;
          minAngle = angles[curr];
        }
      }

      assert(earIndex >= 0);

      // remove ear
      first = next[earIndex];
      next[prev[earIndex]] = next[earIndex];
      prev[next[earIndex]] = prev[earIndex];
      --n;

      // recompute the angles at the vertices surrounding the ear tip.
      computeAngleAtVertex(next[earIndex]);
      computeAngleAtVertex(prev[earIndex]);

      result.push_back({next[earIndex], prev[earIndex]});

      if(enableDisplay)
      {
        for(auto& segment : result)
          sandbox_line(polygon[segment.a], polygon[segment.b], Gray);

        for(int i = 0, curr = first; i < n; ++i, curr = next[curr])
          sandbox_line(polygon[curr], polygon[next[curr]], Yellow);

        const auto A = polygon[prev[earIndex]];
        const auto C = polygon[next[earIndex]];
        sandbox_line(A, C, Red);
        sandbox_breakpoint();
      }
    }

    return result;
  }

  static void display(const std::vector<Vec2>& input, span<const Segment> output)
  {
    drawPolygon(input, White);
    for(auto segment : output)
      sandbox_line(input[segment.a], input[segment.b], Green);
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<FastEarClippingAlgorithm>>()); }
const int reg = registerApp("Triangulation/Polygon/FastEarClipping", &create);
}
