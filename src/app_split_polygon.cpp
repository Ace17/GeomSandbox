// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Split polygon along a straight line

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "random.h"
#include "random_polygon.h"

namespace
{
static Color IndexedColors[] = {
      // {1, 0, 0, 1},
      {0, 1, 0, 1},
      // {0, 0, 1, 1},
      {1, 1, 0, 1},
      {1, 0, 1, 1},
      {0, 1, 1, 1},
      {0.5, 0.5, 1, 1},
      {1, 0.5, 0.5, 1},
};

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

struct PointWithSide
{
  Vec2 pos;
  int side = 0;
};

struct Diagonal
{
  int a, b;
};

struct Hyperplane
{
  Vec2 normal;
  Vec2 tangent;
  float dist;
};

std::vector<std::vector<Vec2>> cutAlongDiagonals(span<const PointWithSide> polygon, span<Diagonal> diagonals)
{
  const int N = polygon.len;

  static auto arcLength = [](int N, int a, int b)
  {
    const int fwd_length = max(a, b) - min(a, b);
    const int back_length = N - fwd_length;
    return min(fwd_length, back_length);
  };

  auto byIncreasingArcLength = [N](Diagonal p, Diagonal q) { return arcLength(N, p.a, p.b) < arcLength(N, q.a, q.b); };

  std::sort(diagonals.begin(), diagonals.end(), byIncreasingArcLength);

  std::vector<std::vector<Vec2>> result;
  std::vector<int> next;
  for(int i = 0; i < N; ++i)
    next.push_back((i + 1) % N);

  int last = 0;
  for(auto [a, b] : diagonals)
  {
    const int fwd_length = (b - a + N) % N;
    const int back_length = (a - b + N) % N;

    int curr = a;
    last = b;
    if(fwd_length >= back_length)
      std::swap(curr, last);

    int watchdog = N * 2;

    std::vector<Vec2> subPoly;
    subPoly.push_back(polygon[curr].pos);
    do
    {
      assert(watchdog-- > 0);
      curr = next[curr];
      subPoly.push_back(polygon[curr].pos);
    } while(curr != last);

    if(fwd_length < back_length)
      next[a] = b;
    else
      next[b] = a;

    result.push_back(std::move(subPoly));
  }

  {
    std::vector<Vec2> subPoly;
    int curr = last;
    int first = last;
    subPoly.push_back(polygon[curr].pos);
    int watchdog = N * 2;

    while(true)
    {
      assert(watchdog-- > 0);
      curr = next[curr];
      if(curr == first)
        break;
      subPoly.push_back(polygon[curr].pos);
    }
    result.push_back(std::move(subPoly));
  }

  return result;
}

std::vector<std::vector<Vec2>> splitPolygonAlongLine(span<const Vec2> inputPolygon, const Hyperplane& plane)
{
  std::vector<PointWithSide> polygon;

  // classify points
  for(auto& pos : inputPolygon)
  {
    const float epsilon = 0.001;
    const float x = dotProduct(pos, plane.normal) - plane.dist;
    int side;
    if(x > +epsilon)
      side = +1;
    else if(x < -epsilon)
      side = -1;
    else
      side = 0;

    polygon.push_back({pos, side});
  }

  {
    for(auto& p : polygon)
    {
      char buf[256];
      snprintf(buf, 255, "%+d", p.side);
      sandbox_text(p.pos, buf);
    }

    sandbox_breakpoint();
  }

  // insert the intersection points
  {
    std::vector<PointWithSide> newPolygon;
    const int N = polygon.size();
    for(int curr = 0; curr < N; ++curr)
    {
      // insert current point
      newPolygon.push_back(polygon[curr]);

      // insert intersection point, if needed
      const int next = (curr + 1) % N;
      if(polygon[curr].side * polygon[next].side == -1)
      {
        const Vec2 p0 = polygon[curr].pos;
        const Vec2 p1 = polygon[next].pos;

        const float q0 = dotProduct(p0, plane.normal);
        const float q1 = dotProduct(p1, plane.normal);
        const float ratio = (plane.dist - q0) / (q1 - q0);

        auto newPos = p0 + ratio * (p1 - p0);
        newPolygon.push_back({newPos, 0});
      }
    }

    // replace 'polygon' with the one with all the intersections
    polygon = std::move(newPolygon);
  }

  // sort the intersection points along the splitting line
  struct IntersectionPoint
  {
    int index;
    bool isEntryPoint = false;
  };

  std::vector<IntersectionPoint> intersectionPoints;

  for(int i = 0; i < (int)polygon.size(); ++i)
    if(polygon[i].side == 0)
      intersectionPoints.push_back({i});

  {
    auto byAbscissa = [&](const IntersectionPoint& ip0, const IntersectionPoint& ip1)
    {
      auto p0 = polygon[ip0.index].pos;
      auto p1 = polygon[ip1.index].pos;
      return dotProduct(p0, plane.tangent) < dotProduct(p1, plane.tangent);
    };

    std::sort(intersectionPoints.begin(), intersectionPoints.end(), byAbscissa);
  }

  const int N = polygon.size();

  {
    static auto encode = [](int prevSide, int, int nextSide) { return prevSide * 8 + nextSide; };

    for(auto& ip : intersectionPoints)
    {
      const auto curr = ip.index;
      const auto prev = (ip.index - 1 + N) % N;
      const auto next = (ip.index + 1 + N) % N;

      auto prevPos = polygon[prev].pos;
      auto currPos = polygon[curr].pos;
      auto nextPos = polygon[next].pos;

      auto prevSide = polygon[prev].side;
      auto currSide = polygon[curr].side;
      auto nextSide = polygon[next].side;

      auto prevEdge = currPos - prevPos;
      auto nextEdge = nextPos - currPos;

      assert(currSide == 0); // we're on an intersection point

      const int turn = det2d(prevEdge, nextEdge) >= 0 ? +1 : -1;

      switch(encode(prevSide, currSide, nextSide))
      {
      case encode(-1, 0, -1):
        if(turn == -1)
          ip.isEntryPoint = true;
        break;
      case encode(0, 0, -1):
        if(turn == -1)
          ip.isEntryPoint = true;
        break;
      case encode(+1, 0, -1):
        ip.isEntryPoint = true;
        break;
      case encode(+1, 0, 0):
        if(turn == -1)
          ip.isEntryPoint = true;
        break;
      case encode(+1, 0, +1):
        if(turn == -1)
          ip.isEntryPoint = true;
        break;
      }
    }
  }

  std::vector<Diagonal> diagonals;

  for(int i = 0; i + 1 < (int)intersectionPoints.size(); ++i)
  {
    if(intersectionPoints[i].isEntryPoint)
    {
      Diagonal diag;
      diag.a = intersectionPoints[i + 0].index;
      diag.b = intersectionPoints[i + 1].index;
      diagonals.push_back(diag);
    }
  }

  {
    for(auto& ip : intersectionPoints)
    {
      sandbox_circle(polygon[ip.index].pos, 0.25, Green);
      if(ip.isEntryPoint)
        sandbox_circle(polygon[ip.index].pos, 0.35, Yellow);
    }

    for(auto& diag : diagonals)
    {
      const int k = int(&diag - diagonals.data());
      sandbox_line(polygon[diag.a].pos, polygon[diag.b].pos, IndexedColors[k % std::size(IndexedColors)]);
    }

    for(auto& p : polygon)
    {
      char buf[256];
      snprintf(buf, 255, "%d", int(&p - polygon.data()));
      sandbox_text(p.pos, buf);
    }

    sandbox_breakpoint();
  }

  return cutAlongDiagonals(polygon, diagonals);
}

///////////////////////////////////////////////////////////////////////////////

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

struct Input
{
  std::vector<Vec2> polygon;

  // line
  Vec2 normal;
  float dist;
};

Input generateRandomSpiral()
{
  Input r;
  const float phase = randomFloat(-M_PI, M_PI);

  for(int k = 0; k < 48; ++k)
  {
    const float rInt = 2 + k * 0.2;
    const float rExt = rInt + 2.5;

    const float angle = 2 * M_PI * k * 0.05 + phase;

    const Vec2 ray = {(float)cos(angle), (float)sin(angle)};

    r.polygon.insert(r.polygon.end(), ray * rExt);
    r.polygon.insert(r.polygon.begin(), ray * rInt);
  }

  r.normal = {0.71, 0.71};
  r.dist = -2.0875;

  if(rand() % 2)
    r.normal = -r.normal;

  return r;
}

Input generateRandomM()
{
  Input r;

  r.polygon.push_back({-3, -2});
  r.polygon.push_back({+3, -2});
  r.polygon.push_back({+3, +2});
  r.polygon.push_back({+2, 0});
  r.polygon.push_back({+1, +2});
  r.polygon.push_back({0, 0});
  r.polygon.push_back({-1, +2});
  r.polygon.push_back({-2, 0});
  r.polygon.push_back({-3, +2});

  r.normal = {0, 1};
  r.dist = 0;

  if(rand() % 2)
    r.normal = -r.normal;

  return r;
}

Input generateRandomLegacy()
{
  auto poly2f = createRandomPolygon2f();

  Input r;

  std::unordered_map<int, int> edges;
  for(auto& edge : poly2f.faces)
    edges[edge.a] = edge.b;

  int first = edges.begin()->first;
  int curr = first;
  do
  {
    r.polygon.push_back(poly2f.vertices[curr]);
    curr = edges.at(curr);
  } while(curr != first);

  const int faceIdx = randomInt(0, poly2f.faces.size());
  r.normal = poly2f.normal(faceIdx);
  r.dist = dotProduct(poly2f.vertices[poly2f.faces[faceIdx].a], r.normal);

  return r;
}

using std::min, std::max;

using Input = ::Input;

Input generateInput(int /*seed*/)
{
  typedef Input (*GenerateInputFunc)();

  GenerateInputFunc funcs[] = {
        &generateRandomSpiral,
        &generateRandomM,
        &generateRandomLegacy,
        &generateRandomLegacy,
        &generateRandomLegacy,
        &generateRandomLegacy,
        &generateRandomLegacy,
        &generateRandomLegacy,
        &generateRandomLegacy,
  };

  return funcs[rand() % std::size(funcs)]();
}

std::vector<std::vector<Vec2>> execute(Input input)
{
  Hyperplane plane;
  plane.normal = input.normal;
  plane.tangent = -rotateLeft(input.normal);
  plane.dist = input.dist;
  return splitPolygonAlongLine(input.polygon, plane);
}

void display(const Input& input, span<const std::vector<Vec2>> output)
{
  drawPolygon(input.polygon, White);

  for(auto& subPoly : output)
  {
    const int k = int(&subPoly - output.ptr);
    drawPolygon(subPoly, IndexedColors[k % std::size(IndexedColors)]);
  }

  if(output.len)
  {
    char buf[256];
    snprintf(buf, 255, "%d polygons", (int)output.len);
    sandbox_text({0, 10}, buf);
  }

  // draw split line
  {
    Color ThinRed = {1, 0, 0, 0.5};
    auto p = input.normal * input.dist;
    auto t = -rotateLeft(input.normal);
    sandbox_line(p - t * 50, p + t * 50, ThinRed);
    sandbox_line(p, p - t * 0.2 + input.normal * 0.2, ThinRed);
    sandbox_line(p, p - t * 0.2 - input.normal * 0.2, ThinRed);
  }
}

BEGIN_ALGO("Split/Polygon", execute)
WITH_INPUTGEN(generateInput)
WITH_DISPLAY(display)
END_ALGO
}
