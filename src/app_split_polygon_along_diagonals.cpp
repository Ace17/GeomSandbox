// Copyright (C) 2026 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Split polygon along a given subset of its diagonals

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "random.h"
#include "random_polygon_with_holes.h"

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

struct Diagonal
{
  int a, b;
};

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

//                 C
//         P     __o
//         o      /|
//               /
// A           B/
// o---------->o
//
// Returns true if P is on the left side of the corner ABC
bool isOnLeftSide(Vec2 A, Vec2 B, Vec2 C, Vec2 P)
{
  const Vec2 AB = B - A;
  const Vec2 BC = C - B;

  const Vec2 AP = P - A;
  const Vec2 BP = P - B;

  const bool leftOfAB = det2d(AB, AP) >= 0;
  const bool leftOfBC = det2d(BC, BP) >= 0;

  if(det2d(AB, BC) >= 0)
    return leftOfAB && leftOfBC;
  else
    return leftOfAB || leftOfBC;
}
///////////////////////////////////////////////////////////////////////////////

void drawEdge(Vec2 a, Vec2 b, Color color)
{
  auto middle = (a + b) * 0.5;
  auto normalTip = middle - rotateLeft(normalize(b - a)) * 0.3;
  sandbox_line(a, b, color);
  sandbox_line(middle, normalTip, {0.5, 0, 0, 1});
}

void drawPolygon(span<const Vec2> input, Color color)
{
  const int N = input.len;
  for(int i = 0; i < N; ++i)
  {
    auto a = input[i];
    auto b = input[(i + 1) % N];
    drawEdge(a, b, color);
  }
}

struct Input
{
  std::vector<Vec2> vertices;
  std::vector<int> nextVertex; // edge (a,b) is represented as: b=nextVertex[a]
  std::vector<Diagonal> diagonals;
};

float sign(float v)
{
  if(v < 0)
    return -1;
  if(v > 0)
    return +1;
  return 0;
}

bool segmentIntersects(Vec2 p0, Vec2 p1, Vec2 q0, Vec2 q1)
{
  const int side_p0 = sign(det2d(q1 - q0, p0 - q0));
  const int side_p1 = sign(det2d(q1 - q0, p1 - q0));
  if(side_p0 == side_p1 && side_p0 != 0)
    return false; // the lines don't cross

  const int side_q0 = sign(det2d(p1 - p0, q0 - p0));
  const int side_q1 = sign(det2d(p1 - p0, q1 - p0));
  if(side_q0 == side_q1 && side_q0 != 0)
    return false; // the lines don't cross

  return true;
}

Input generateInput(int /*seed*/)
{
  auto polygonWithHoles = createRandomPolygonWithHoles();

  Input r;

  for(auto& polygon : polygonWithHoles.boundaries)
  {
    const int first = r.vertices.size();
    int curr = first;
    for(auto& vertex : polygon)
    {
      r.vertices.push_back(vertex);
      r.nextVertex.push_back(++curr);
    }

    r.nextVertex.back() = first;
  }

  const int N = r.vertices.size();

  std::vector<std::pair<Vec2, Vec2>> edges;
  for(int i = 0; i < N; ++i)
    edges.push_back({r.vertices[i], r.vertices[r.nextVertex[i]]});

  auto isValidDiagonal = [&](Diagonal diag)
  {
    if(diag.a == diag.b)
      return false;

    for(auto& [a, b] : edges)
    {
      auto newEdge = std::make_pair(r.vertices[diag.a], r.vertices[diag.b]);

      if(newEdge == std::make_pair(a, b) || newEdge == std::make_pair(b, a))
        return false; // edge already exists

      if(a == newEdge.first || a == newEdge.second || b == newEdge.first || b == newEdge.second)
        continue;

      if(segmentIntersects(a, b, r.vertices[diag.a], r.vertices[diag.b]))
        return false;
    }

    return true;
  };

  const int K = N / 10;
  for(int k = 0; k < K * 10; ++k)
  {
    Diagonal diag;
    diag.a = rand() % N;
    diag.b = rand() % N;

    if(!isValidDiagonal(diag))
      continue;

    r.diagonals.push_back(diag);
    edges.push_back({r.vertices[diag.a], r.vertices[diag.b]});
  }

  return r;
}

std::vector<std::vector<Vec2>> execute(Input input)
{
  const int N = input.nextVertex.size();

  std::unordered_map<int, std::vector<int>> next;
  for(int vertex = 0; vertex < N; ++vertex)
    next[vertex] = {input.nextVertex[vertex]};

  for(auto diag : input.diagonals)
  {
    next[diag.a].push_back(diag.b);
    next[diag.b].push_back(diag.a);
  }

  std::unordered_multiset<int> entryPoints;
  for(int i = 0; i < N; ++i)
    entryPoints.insert(i);
  for(auto diag : input.diagonals)
  {
    entryPoints.insert(diag.a);
    entryPoints.insert(diag.b);
  }

  {
    for(int i = 0; i < N; ++i)
    {
      char buf[256];
      sprintf(buf, "%d", (int)entryPoints.count(i));
      sandbox_text(input.vertices[i], buf);
    }
  }
  sandbox_breakpoint();

  std::vector<std::vector<Vec2>> r;

  while(entryPoints.size())
  {
    int v = *entryPoints.begin();

    sandbox_circle(input.vertices[v], 0, Red, 8);

    const int first = v;
    int prev = 0;

    r.push_back({});
    auto& polygon = r.back();

    do
    {
      auto it = entryPoints.find(v);
      if(it == entryPoints.end())
      {
        sandbox_circle(input.vertices[v], 0, Orange, 8);
        sandbox_breakpoint();
      }
      assert(it != entryPoints.end());
      entryPoints.erase(it);

      polygon.push_back(input.vertices[v]);

      int bestEdge = 0;

      if(v != first)
      {
        // choose the edge that causes the leftmost turn
        const Vec2 A = input.vertices[prev];
        const Vec2 B = input.vertices[v];
        for(int i = 0; i < (int)next[v].size(); ++i)
        {
          if(isOnLeftSide(A, B, input.vertices[next[v][bestEdge]], input.vertices[next[v][i]]) && next[v][i] != prev)
            bestEdge = i;
        }
      }
      const int n = next[v][bestEdge];
      next[v].erase(next[v].begin() + bestEdge);
      sandbox_line(input.vertices[v], input.vertices[n], Green);
      sandbox_circle(input.vertices[n], 0, Green, 8);

      prev = v;
      v = n;
    } while(first != v);

    sandbox_breakpoint();
  }

  return r;
}

void display(const Input& input, span<const std::vector<Vec2>> output)
{
  for(auto [a, b] : input.diagonals)
    sandbox_line(input.vertices[a], input.vertices[b], {1, 1, 0, 0.5});

  for(int i = 0; i < (int)input.nextVertex.size(); ++i)
  {
    const int next = input.nextVertex[i];
    drawEdge(input.vertices[i], input.vertices[next], White);
  }

  for(auto& subPoly : output)
  {
    const int k = int(&subPoly - output.ptr);
    drawPolygon(subPoly, IndexedColors[k % std::size(IndexedColors)]);
  }
}

BEGIN_ALGO("Split/PolygonAgainstDiagonals", execute)
WITH_INPUTGEN(generateInput)
WITH_DISPLAY(display)
END_ALGO
}
