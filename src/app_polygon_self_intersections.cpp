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

  struct Intersection
  {
    Vec2 pos;
    int i, j; // segment start indices. By construction, we always have i<j.
  };

  static std::vector<Intersection> execute(std::vector<Vec2> input)
  {
    std::vector<Intersection> r;

    const float toleranceRadius = 0.001;

    for(int i = 0; i < int(input.size()) - 1; ++i)
    {
      for(int j = i + 2; j < int(input.size()) - 1; ++j)
      {
        Vec2 where;
        if(segmentsIntersect(input[i], input[i + 1], input[j], input[j + 1], toleranceRadius, where))
          r.push_back({where, i, j});
      }
    }

    {
      int idx = 0;
      for(auto& point : r)
      {
        sandbox_circle(input[point.i], 0.1, Orange);
        sandbox_circle(input[point.j], 0.1, Orange);

        sandbox_circle(point.pos, 0.2, Red);

        char buf[256];
        sprintf(buf, "I%d", idx);
        sandbox_text(point.pos - Vec2(1, 0), buf, Red);
        fprintf(stderr, "Contact at %.2f,%.2f [%d, %d]\n", point.pos.x, point.pos.y, point.i, point.j);
        ++idx;
      }

      {
        char buf[256];
        sprintf(buf, "%d contacts", (int)r.size());
        sandbox_text({0, 10}, buf, White);
      }
      sandbox_breakpoint();
    }

    // remove simple contacts
    auto isSimpleContact = [&](Intersection point)
    {
      const bool isOnVertexI = sqrMagnitude(point.pos - input[point.i]) < sqr(toleranceRadius);
      const bool isOnVertexJ = sqrMagnitude(point.pos - input[point.j]) < sqr(toleranceRadius);

      fprintf(stderr, "i=%d j=%d isOnVertexI=%d isOnVertexJ=%d\n", point.i, point.j, isOnVertexI, isOnVertexJ);

      if(!isOnVertexI && !isOnVertexJ)
        return false; // segment/segment crossing in the middle
                      //
      if(isOnVertexI != isOnVertexJ)
        return false; // vertex/edge contact : always count as an intersection

      // vertex/vertex contact
      return true;
    };

    auto it = std::remove_if(r.begin(), r.end(), isSimpleContact);
    r.erase(it, r.end());

    return r;
  }

  static void display(span<const Vec2> input, span<const Intersection> output)
  {
    for(int i = 0; i < (int)input.len; ++i)
    {
      char buf[256];
      sprintf(buf, "%d", i);
      sandbox_text(input[i] + Vec2(0.2, 0.2), buf);
      sandbox_line(input[i] + Vec2(-0.1, -0.1), input[i] + Vec2(0.1, -0.1));
      sandbox_line(input[i] + Vec2(-0.1, +0.1), input[i] + Vec2(0.1, +0.1));
      sandbox_line(input[i] + Vec2(-0.1, -0.1), input[i] + Vec2(-0.1, 0.1));
      sandbox_line(input[i] + Vec2(+0.1, -0.1), input[i] + Vec2(+0.1, 0.1));
    }

    for(int i = 0; i + 1 < (int)input.len; ++i)
      sandbox_line(input[i], input[i + 1]);

    int idx = 0;
    for(auto& point : output)
    {
      sandbox_circle(input[point.i], 0.1, Orange);
      sandbox_circle(input[point.j], 0.1, Orange);

      sandbox_circle(point.pos, 0.2, Red);

      char buf[256];
      sprintf(buf, "I%d", idx);
      sandbox_text(point.pos - Vec2(1, 0), buf, Red);
      ++idx;
    }
  }

  inline static const TestCase<std::vector<Vec2>> AllTestCases[] = {
        {"vertex/vertex contact",
              {
                    {-10, -10},
                    {0, -0.0005},
                    {+10, -10},
                    {+10, 10},
                    {0, 0},
                    {-10, 10},
                    {-10, 1},
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
