// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////

#include "core/algorithm_app.h"
#include "core/sandbox.h"

#include <algorithm> // remove_if
#include <cmath>
#include <vector>

#include "random.h"

template<>
std::vector<Vec2> deserialize<std::vector<Vec2>>(span<const uint8_t> data);

namespace
{
float sqr(float v) { return v * v; }

float sqrMagnitude(Vec2 a) { return dotProduct(a, a); }

bool segmentsIntersect(Vec2 u0, Vec2 u1, Vec2 v0, Vec2 v1, float toleranceRadius, Vec2& where)
{
  // raycast of u0->u1 against the "wall" v0v1
  const Vec2 tangent = normalize(v1 - v0);
  const Vec2 normal = rotateLeft(tangent);

  // project everybody on this normal
  const float pos0 = dotProduct(normal, u0);
  const float pos1 = dotProduct(normal, u1);
  const float posWall = dotProduct(normal, v0);

  if(pos0 > posWall + toleranceRadius && pos1 > posWall + toleranceRadius)
    return false;

  if(pos0 < posWall - toleranceRadius && pos1 < posWall - toleranceRadius)
    return false;

  float fraction;

  // parallel segments
  if(fabs(pos1 - pos0) < toleranceRadius)
  {
    if(fabs(pos0 - posWall) > toleranceRadius)
      return false; // not colinear enough

    const float uMin = std::min(dotProduct(tangent, u0), dotProduct(tangent, u1));
    const float uMax = std::max(dotProduct(tangent, u0), dotProduct(tangent, u1));

    const float vMin = std::min(dotProduct(tangent, v0), dotProduct(tangent, v1));
    const float vMax = std::max(dotProduct(tangent, v0), dotProduct(tangent, v1));

    if(uMax + toleranceRadius < vMin || vMax < uMin - toleranceRadius)
      return false; // colinear, but no common point

    if(uMin - toleranceRadius < vMin)
      fraction = (vMin - uMin) / (uMax - uMin);
    else
      fraction = (vMax - uMin) / (uMax - uMin);

    if(dotProduct(u1 - u0, tangent) < 0)
      fraction = 1 - fraction;
  }
  else
  {
    fraction = (posWall - pos0) / (pos1 - pos0);
  }

  if(fraction < -toleranceRadius || fraction > 1 + toleranceRadius)
    return false;

  // intersection of u0u1 with the line v0v1
  where = u0 + fraction * (u1 - u0);

  // check if the intersection belongs to the segment v0v1
  if(dotProduct(where - v0, tangent) < -toleranceRadius)
    return false;

  if(dotProduct(where - v1, tangent) > +toleranceRadius)
    return false;

  // we exclude the circle centered on u1, with a radius of toleranceRadius.
  if(sqrMagnitude(where - u1) < sqr(toleranceRadius))
    return false;

  // we exclude the circle centered on v1, with a radius of toleranceRadius.
  if(sqrMagnitude(where - v1) < sqr(toleranceRadius))
    return false;

  return true;
}

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

// get the side of P relative to the oriented segment A-B
int classifySide(Vec2 a, Vec2 b, Vec2 p, float segmentThickness)
{
  const Vec2 n = rotateLeft(normalize(b - a));

  const float y = dotProduct(p - a, n);

  if(y < -segmentThickness)
    return -1;

  if(y > +segmentThickness)
    return +1;

  return 0;
}

// get the side of P relative to the oriented polyline A-B-C
// +1 for 'on the left'
//  0 for 'on A-B-C'
// -1 for 'on the right'
int classifySide(Vec2 a, Vec2 b, Vec2 c, Vec2 p, float segmentThickness)
{
  const int P_Side_AB = classifySide(a, b, p, segmentThickness);
  const int P_Side_BC = classifySide(b, c, p, segmentThickness);

  if(det2d(b - a, c - b) >= 0) // ABC makes a left-turn
  {
    if(P_Side_AB == -1 || P_Side_BC == -1)
      return -1; // we're on the right of ABC

    if(P_Side_AB == 0 || P_Side_BC == 0)
      return 0; // we're on ABC

    return 1; // we're on the left of ABC
  }
  else // ABC makes a right-turn
  {
    if(P_Side_AB == 1 || P_Side_BC == 1)
      return +1; // we're on the left of ABC
                 //
    if(P_Side_AB == 0 || P_Side_BC == 0)
      return 0; // we're on ABC

    return -1; // we're on the right of ABC
  }
}

struct Intersection
{
  Vec2 pos;
  int i, j; // segment start indices. By construction, we always have i<j.
};

struct Crossing
{
  Vec2 pos;
  int i, j; // segment start indices.
  float ti, tj; // positions on segment i and segment j
};

std::vector<Intersection> computeSelfIntersections(span<const Vec2> input)
{
  std::vector<Crossing> crossings;

  const float toleranceRadius = 0.001;

  const int N = input.len;

  for(int i = 0; i < N - 1; ++i)
  {
    for(int j = i + 2; j < N - 1; ++j)
    {
      const Vec2 I0 = input[i + 0];
      const Vec2 I1 = input[i + 1];
      const Vec2 J0 = input[j + 0];
      const Vec2 J1 = input[j + 1];

      Vec2 where;

      if(segmentsIntersect(I0, I1, J0, J1, toleranceRadius, where))
      {
        const Vec2 segmentI = I1 - I0;
        const float fractionI = dotProduct(where - I0, segmentI);

        const Vec2 segmentJ = J1 - J0;
        const float fractionJ = dotProduct(where - J0, segmentJ);

        crossings.push_back({where, i, j, fractionI, fractionJ});
        crossings.push_back({where, j, i, fractionJ, fractionI});
      }
    }
  }

  {
    auto byTime = [](const Crossing& a, const Crossing& b)
    {
      if(a.i != b.i)
        return a.i < b.i;

      return a.ti < b.ti;
    };
    std::sort(crossings.begin(), crossings.end(), byTime);
  }

  // traverse all crossings, determine real intersections
  std::vector<Intersection> r;

  {
    int lastRealSide = 0;
    int side = 0;
    for(auto c : crossings)
    {
      const auto X = c.pos;

      // compute prevPosI, nextPosI
      const bool isOnVertexI = sqrMagnitude(X - input[c.i]) < sqr(toleranceRadius);
      const Vec2 prevPosI = isOnVertexI ? input[(c.i - 1 + N) % N] : input[c.i];
      const Vec2 nextPosI = input[(c.i + 1) % N];

      // compute prevPosJ, nextPosJ
      const bool isOnVertexJ = sqrMagnitude(X - input[c.j]) < sqr(toleranceRadius);
      const Vec2 prevPosJ = isOnVertexJ ? input[(c.j - 1 + N) % N] : input[c.j];
      const Vec2 nextPosJ = input[(c.j + 1) % N];

      const int sideBefore = classifySide(prevPosJ, X, nextPosJ, prevPosI, toleranceRadius);
      const int sideAfter = classifySide(prevPosJ, X, nextPosJ, nextPosI, toleranceRadius);
      side += (sideAfter - sideBefore);

      bool mustPush = false;

      if(isOnVertexI != isOnVertexJ)
        mustPush = true;

      if((side % 2 == 0) && side != lastRealSide)
      {
        if(c.i < c.j)
          mustPush = true; // side change
        lastRealSide = side;
      }

      if(mustPush)
        r.push_back({X, c.i, c.j});

      sandbox_circle(X, 0, Red, 5);
      sandbox_line(prevPosI, X, Green);
      sandbox_line(X, nextPosI, Green);
      sandbox_line(prevPosJ, X, Yellow);
      sandbox_line(X, nextPosJ, Yellow);
      char buf[256];
      sprintf(buf, "sideBefore=%d sideAfter=%d side=%d", sideBefore, sideAfter, side);
      sandbox_text({0, 11}, buf);
      sandbox_breakpoint();
    }
  }

  return r;
}

struct PolygonSelfIntersectionAlgorithm
{
  static std::vector<Vec2> generateInput()
  {
    std::vector<Vec2> points;
    const int N = 10;

    for(int i = 0; i < N; ++i)
    {
      Vec2 pos;
      pos.x = randomFloat(-20, 20);
      pos.y = randomFloat(-20, 20);
      points.push_back(pos);
    }

    return points;
  }

  static std::vector<Intersection> execute(std::vector<Vec2> input) { return computeSelfIntersections(input); }

  static void display(span<const Vec2> input, span<const Intersection> output)
  {
    for(int i = 0; i < (int)input.len; ++i)
    {
      char buf[256];
      sprintf(buf, "%d", i);
      sandbox_text(input[i], buf, White, Vec2(6, -6));
      sandbox_rect(input[i], {}, White, Vec2(6, 6));
    }

    for(int i = 0; i + 1 < (int)input.len; ++i)
      sandbox_line(input[i], input[i + 1]);

    int idx = 0;
    for(auto& point : output)
    {
      sandbox_circle(input[point.i], {}, Orange, 6);
      sandbox_circle(input[point.j], {}, Orange, 6);

      sandbox_circle(point.pos, {}, Red, 5);

      char buf[256];
      sprintf(buf, "I%d", idx);
      sandbox_text(point.pos, buf, Red, Vec2(6, +24));
      ++idx;
    }

    {
      char buf[256];
      sprintf(buf, "%d intersection(s)", (int)output.len);
      sandbox_text({0, 9}, buf, White);
    }
  }

  inline static const TestCase<std::vector<Vec2>> AllTestCases[] = {
        {"side change after coinciding border",
              {
                    {0, 0},
                    {4, 0},
                    {4, 4},
                    {0, 4},
                    {0, 2},
                    {0.5, 2},
                    {1.5, 2},
                    {2, 2},
                    {2, 3},
                    {3, 3},
                    {3, 1},
                    {2, 1},
                    {2, 2},
                    {1.5, 2},
                    {1.0, 2.5},
                    {0.5, 2},
                    {0, 2},
              }},

        {"saw, multiple intersections",
              {
                    {0, 0},
                    {10, 0},
                    {10, 2},
                    {1, 2},
                    {1, 3},
                    {1.5, 3},
                    {2, 2},
                    {2.5, 2},
                    {3, 3},
                    {4, 1},
                    {5, 3},
                    {5.8, 2},
                    {6.2, 2},
                    {6.5, 1},
                    {7, 2},
                    {7.3, 2},
                    {8, 3},
                    {8.5, 2},
                    {9, 3},
                    {10, 3},
                    {10, 4},
                    {0, 4},
                    {0, 0.1},
              }},

        {"vertex/vertex contact, intersection",
              {
                    {-10, -10},
                    {0, -10},
                    {0, 0},
                    {0, +10},
                    {+10, +10},
                    {+10, 0},
                    {0, 0},
                    {-10, 0},
                    {-10, -10 + 0.1},
              }},
        {"vertex/vertex contact, no intersection",
              {
                    {-10, -10},
                    {0, -0.0005},
                    {+10, -10},
                    {+10, 10},
                    {0, 0},
                    {-10, 10},
                    {-10, 1},
              }},
        {"twisted bridge",
              {
                    {-20, 0},
                    {-15, -5},
                    {-10, 0},
                    {0, 0},
                    {+10, 0},
                    {+15, 5},
                    {+20, 0},
                    {+15, -5},
                    {+10, 0},
                    {0, 0},
                    {-10, 0},
                    {-15, 5},
                    {-20.1, 0.1},
              }},
        {"bridge",
              {
                    {0, 0},
                    {9, 0},
                    {9, 6},
                    {6, 6},
                    {6, 3},
                    {3, 3},
                    {3, 9},
                    {6, 9},
                    {6, 6},
                    {9, 6},
                    {9, 12},
                    {0, 12},
                    {0, 1},
              }},
        {"vertex/segment contact",
              {
                    {-10, 0},
                    {+10, 0},
                    {+10, 5},
                    {0, 0},
                    {-10, 5},
                    {-10, 1},
              }},
        {"cross",
              {
                    {-10, -10},
                    {+10, +10},
                    {-10, +10},
                    {+10, -10},
              }},
        {"basic",
              {
                    {-10, 0},
                    {10, 0},
                    {10, -5},
                    {0, -5},
                    {0, 0},
                    {0, 5},
                    {-10, 5},
              }},
  };
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<PolygonSelfIntersectionAlgorithm>>()); }
const int reg = registerApp("Intersection/Polygon/SelfIntersection", &create);
}
