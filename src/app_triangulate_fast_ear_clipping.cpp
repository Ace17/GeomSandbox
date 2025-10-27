// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Polygon Triangulation: Ear-clipping Algorithm.
// ( based on: Ear-clipping Based Algorithms of Generating High-quality Polygon
// Triangulation, Gang Mei )

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <cassert>
#include <cfloat>
#include <cmath>
#include <string>
#include <vector>

#include "random.h"
#include "random_polygon.h"

const bool enableDisplay = true;

float clamp(float value, float min, float max) { return std::min(max, std::max(min, value)); }

std::vector<Vec2> loadPolygon(span<const uint8_t> data);

namespace
{

struct Triangle
{
  int a, b, c;
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
    const int j = (i + 1) % N;
    auto a = Vec3(input[i].x, input[i].y, i * 0.1);
    auto b = Vec3(input[j].x, input[j].y, j * 0.1);
    auto middle = (a + b) * 0.5;
    auto normal = normalize(input[j] - input[i]);
    auto normalTip = crossProduct(Vec3(normal.x, normal.y, 0), Vec3(0, 0, 1)) * 10.0;
    sandbox_line(a, b, color);
    sandbox_line(middle, middle, {0.5, 0, 0, 1}, Vec2{}, {normalTip.x, normalTip.y});

    sandbox_rect(input[i], {}, White, Vec2(4, 4));
  }
}

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

int queryCount;

bool pointInTriangle(Vec2 A, Vec2 B, Vec2 C, Vec2 P)
{
  ++queryCount;
  return det2d(B - A, P - A) >= 0 && det2d(C - B, P - B) >= 0 && det2d(A - C, P - C) >= 0;
}

float computeArea(span<const Vec2> polygon)
{
  float r = 0;
  for(int i = 0; i < (int)polygon.len; ++i)
  {
    const Vec2 p0 = polygon[i];
    const Vec2 p1 = polygon[(i + 1) % polygon.len];
    r += det2d(p0, p1);
  }
  return r * 0.5f;
}

float computeTriangulatedArea(span<const Vec2> polygon, span<const Triangle> triangles)
{
  float r = 0;
  for(auto& t : triangles)
  {
    const Vec2 a(polygon[t.a]);
    const Vec2 b(polygon[t.b]);
    const Vec2 c(polygon[t.c]);
    assert(det2d(a - c, b - c) > 0);
    r += det2d(a - c, b - c);
  }
  return r * 0.5f;
};

std::vector<Vec2> generateInput(int /*seed*/)
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

std::vector<Triangle> execute(std::vector<Vec2> input)
{
  queryCount = 0;
  span<const Vec2> polygon = input;
  const int N = polygon.len;

  struct VertexInfo
  {
    int prev;
    int next;
    float angle;
    bool tipContainsOtherVertices;
    bool tipIsChokePoint;

    // for tie break
    Vec2 pos;
  };

  static auto isBetter = [](const VertexInfo& a, const VertexInfo& b)
  {
    if(a.tipContainsOtherVertices != b.tipContainsOtherVertices)
      return a.tipContainsOtherVertices < b.tipContainsOtherVertices;
    if(a.tipIsChokePoint != b.tipIsChokePoint)
      return a.tipIsChokePoint < b.tipIsChokePoint;
    if(a.angle != b.angle)
      return a.angle < b.angle;
    if(a.pos.x != b.pos.x)
      return a.pos.x < b.pos.x;
    if(a.pos.y != b.pos.y)
      return a.pos.y < b.pos.y;
    return false;
  };

  std::vector<VertexInfo> info(N);

  for(int i = 0; i < N; ++i)
  {
    info[i].prev = (i - 1 + N) % N;
    info[i].next = (i + 1 + N) % N;
    info[i].pos = input[i];
  }

  auto tipContainsOtherVertices = [&](int tip, int n)
  {
    const auto A = polygon[info[tip].prev];
    const auto B = polygon[tip];
    const auto C = polygon[info[tip].next];

    // test all other vertices, they must not be in the ABC triangle
    for(int i = 0, curr = info[info[tip].next].next; i + 3 < n; ++i, curr = info[curr].next)
    {
      if(polygon[curr] == A || polygon[curr] == B || polygon[curr] == C)
        continue;

      if(pointInTriangle(A, B, C, polygon[curr]))
        return true;
    }

    return false;
  };

  auto tipIsChokePoint = [&](int tip, int n)
  {
    const auto A = polygon[info[tip].prev];
    const auto B = polygon[tip];
    const auto C = polygon[info[tip].next];

    // check for other edges starting from the tip
    for(int i = 0, curr = info[info[tip].next].next; i + 3 < n; ++i, curr = info[curr].next)
    {
      const auto Q = polygon[curr];
      if(!(Q == B))
        continue;

      const auto P = polygon[info[curr].next];

      // if the edge lies inside the interior of ABC, this is a choke point
      if(det2d(P - B, A - B) > 0 && det2d(P - B, C - B) < 0)
        return true;
    }

    return false;
  };

  auto recomputeInfoAtVertex = [&](int i, int n)
  {
    const auto A = polygon[info[i].prev];
    const auto B = polygon[i];
    const auto C = polygon[info[i].next];

    info[i].tipContainsOtherVertices = tipContainsOtherVertices(i, n);
    info[i].tipIsChokePoint = tipIsChokePoint(i, n);

    const auto lenAB = magnitude(B - A);
    const auto lenBC = magnitude(B - C);
    const auto mag = lenAB * lenBC;

    // degenerated angles are considered to be of zero value:
    // thus, they will be removed first from the polygon.
    if(lenAB <= 0.0001 || lenBC <= 0.00001)
    {
      info[i].angle = 0;
      return;
    }

    const auto det = det2d(B - A, C - B) / mag;
    const auto dot = clamp(dotProduct(A - B, C - B) / mag, -1, +1);

    const float acuteAngle = acos(dot);
    info[i].angle = det >= 0 ? acuteAngle : 2 * M_PI - acuteAngle;

    // 360 degrees angles are created when to edges coincide
    // in this case, the ear will have an area of zero.
    if(info[i].angle > 2 * M_PI - 0.001)
      info[i].angle = 0;
  };

  for(int i = 0; i < N; ++i)
    recomputeInfoAtVertex(i, N);

  std::vector<Triangle> result;

  int first = 0;
  int n = N;

  while(n > 3)
  {
    // find best ear to cut
    int earIndex = first;
    for(int i = 0, curr = first; i < n; ++i, curr = info[curr].next)
    {
      if(isBetter(info[curr], info[earIndex]))
        earIndex = curr;
    }

    if(info[earIndex].tipIsChokePoint || info[earIndex].tipContainsOtherVertices || info[earIndex].angle > M_PI)
    {
      FILE* fp = fopen("/tmp/triangulation_failure.txt", "wb");
      for(int i = 0, curr = first; i < n; ++i, curr = info[curr].next)
        fprintf(fp, "%.5f, %.5f\n", polygon[curr].x, polygon[curr].y);
      fclose(fp);
    }

    // remove ear
    first = info[earIndex].next;
    info[info[earIndex].prev].next = info[earIndex].next;
    info[info[earIndex].next].prev = info[earIndex].prev;
    --n;

    // recompute the angles at the vertices surrounding the ear tip.
    recomputeInfoAtVertex(info[earIndex].next, n);
    recomputeInfoAtVertex(info[earIndex].prev, n);

    if(det2d(polygon[earIndex] - polygon[info[earIndex].prev], polygon[info[earIndex].next] - polygon[earIndex]) > 0)
      result.push_back({earIndex, info[earIndex].next, info[earIndex].prev});

    if(enableDisplay)
    {
      for(auto& segment : result)
      {
        sandbox_line(polygon[segment.a], polygon[segment.b], Gray);
        sandbox_line(polygon[segment.b], polygon[segment.c], Gray);
        sandbox_line(polygon[segment.c], polygon[segment.a], Gray);
      }

      for(int i = 0, curr = first; i < n; ++i, curr = info[curr].next)
        sandbox_line(polygon[curr], polygon[info[curr].next], Yellow);

      for(int i = 0, curr = first; i < n; ++i, curr = info[curr].next)
      {
        char buf[256];
        sprintf(buf, "%.1f", info[curr].angle * 180 / M_PI);

        Color color = Green;
        if(info[curr].tipContainsOtherVertices)
          color = Red;
        if(info[curr].tipIsChokePoint)
          color = Orange;
        if(info[curr].angle >= M_PI)
          color = Yellow;
        sandbox_text(polygon[curr], buf, color);
      }

      const auto A = polygon[info[earIndex].prev];
      const auto C = polygon[info[earIndex].next];
      sandbox_line(A, C, Red);
      if(info[earIndex].tipContainsOtherVertices || info[earIndex].angle > M_PI)
      {
        char buf[256];
        sprintf(
              buf, "Concealed: angle=%.3f, CE2=%d", info[earIndex].angle, int(info[earIndex].tipContainsOtherVertices));
        sandbox_text({-20, 4}, buf);
      }
      sandbox_breakpoint();
    }
  }

  if(det2d(polygon[first] - polygon[info[first].prev], polygon[info[first].next] - polygon[first]) > 0)
    result.push_back({first, info[first].next, info[first].prev});

  fprintf(stderr, "%d triangles, queryCount=%d\n", (int)result.size(), queryCount);
  return result;
}

void display(const std::vector<Vec2>& input, span<const Triangle> output)
{
  drawPolygon(input, White);
  for(auto segment : output)
  {
    sandbox_line(input[segment.a], input[segment.b], Green);
    sandbox_line(input[segment.b], input[segment.c], Green);
    sandbox_line(input[segment.c], input[segment.a], Green);
  }

  char buf[256];
  sprintf(buf, "polygon area: %.2f\n", computeArea(input));
  sandbox_text({0, 0}, buf);
  sprintf(buf, "triangulated area: %.2f\n", computeTriangulatedArea(input, output));
  sandbox_text({0, -2}, buf);
}

inline static const TestCase<std::vector<Vec2>, span<const Triangle>> AllTestCases[] = {
      {
            "bridge",
            {
                  {-10, -10},
                  {+10, +10},
                  {-10, +10},
                  {+10, -10},
            },
      },

      {
            "bad ear, bollocks",
            {
                  {0, 1},
                  {1, -3},
                  {2, -3},
                  {2, -2},
                  {0, 1},
                  {-2, -2},
                  {-2, -3},
                  {-1, -3},
            },
      },
};

BEGIN_ALGO("Triangulation/Polygon/FastEarClipping", execute)
WITH_INPUTGEN(generateInput)
WITH_TESTCASES(AllTestCases)
WITH_LOADER(loadPolygon)
WITH_DISPLAY(display)
END_ALGO
}
