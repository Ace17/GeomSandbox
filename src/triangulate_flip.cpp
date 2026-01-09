// Copyright (C) 2026 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Delaunay Triangulation, Flip algorithm:
// 1) build a basic triangulation by sweeping the input points left-to-right
// 2) flip the edges where needed, until we're Delaunay conformant

#include "triangulate_flip.h"

#include "core/geom.h"
#include "core/sandbox.h"

#include <cmath>
#include <set>
#include <vector>

#include "triangulate_basic.h"

namespace
{
const bool EnableTrace = true;

struct Circle
{
  Vec2 center;
  float sqrRadius;
};

Circle computeCircumcircle(Vec2 c0, Vec2 c1, Vec2 c2)
{
  // compute both perpendicular bissectors
  const Vec2 A = (c0 + c1) * 0.5;
  const Vec2 tA = c1 - c0; // tangent

  const Vec2 B = (c0 + c2) * 0.5;
  const Vec2 nB = rotateLeft(c2 - c0); // normal

  // Let I be the intersection of both bissectors.
  // We know that:
  // 1) I = B + k * nB
  // 2) AI.tA = 0
  // So:
  // k = - AB.tA / nB.tA

  const auto k = -((B - A) * tA) / (nB * tA);

  const auto I = B + nB * k;

  Circle r;
  r.center = I;
  r.sqrRadius = (I - c0) * (I - c0);

  return r;
}

void flipTriangulation(span<const Vec2> points, span<HalfEdge> he)
{
  int statFlipCount = 0;
  std::set<int> stack;

  for(int i = 0; i < (int)he.len; ++i)
  {
    if(he[i].twin == -1)
      continue;

    stack.insert(i);
  }

  while(!stack.empty())
  {
    // |              B             |          |              B             |
    // |             /|\            |          |             / \            |
    // |         L1 / | \ R2        |          |         L1 /   \ R2        |
    // |           /  |  \          |          |           /     \          |
    // |        C /   |   \D        |   ==>    |        C /___E___\D        |
    // |          \  E|Et /         |          |          \   Et  /         |
    // |           \  |  /          |          |           \     /          |
    // |         L2 \ | / R1        |          |         L2 \   / R1        |
    // |             \|/            |          |             \ /            |
    // |              A             |          |              A             |
    const int E = *stack.begin();
    stack.erase(stack.begin());

    const int twinE = he[E].twin;
    if(twinE == -1)
      continue;

    const auto L1 = he[E].next;
    const auto L2 = he[L1].next;

    const auto R1 = he[twinE].next;
    const auto R2 = he[R1].next;

    const int A = he[E].point;
    const int B = he[L1].point;
    const int C = he[L2].point;
    const int D = he[R2].point;

    const Circle leftCircle = computeCircumcircle(points[A], points[B], points[C]);

    auto delta = points[D] - leftCircle.center;
    if(dotProduct(delta, delta) < leftCircle.sqrRadius)
    {
      if(EnableTrace)
      {
        for(auto edge : he)
          sandbox_line(points[edge.point], points[he[edge.next].point], Gray);

        sandbox_line(points[he[E].point], points[he[he[E].next].point], Green);
        sandbox_circle(leftCircle.center, sqrt(leftCircle.sqrRadius), Red);
        sandbox_circle(points[D], 0, Red, 8.0);
        sandbox_breakpoint();
      }

      // flip edge E
      he[E].point = C;
      he[E].next = R2;

      he[twinE].point = D;
      he[twinE].next = L2;

      he[L2].next = R1;
      he[R1].next = twinE;
      he[R2].next = L1;
      he[L1].next = E;

      // repush edges for re-evaluation
      stack.insert(L1);
      stack.insert(R1);
      stack.insert(L2);
      stack.insert(R2);

      ++statFlipCount;
    }

    if(EnableTrace)
    {
      sandbox_circle(leftCircle.center, sqrt(leftCircle.sqrRadius), Green);

      for(auto edge : he)
        sandbox_line(points[edge.point], points[he[edge.next].point], Gray);
      for(auto edgeIndex : stack)
      {
        const HalfEdge& edge = he[edgeIndex];
        sandbox_line(points[edge.point], points[he[edge.next].point], LightBlue);
      }

      sandbox_line(points[he[E].point], points[he[he[E].next].point], Green);

      sandbox_breakpoint();
    }
  }

  if(EnableTrace)
  {
    for(auto edge : he)
      sandbox_line(points[edge.point], points[he[edge.next].point], Yellow);
    sandbox_printf("%d flips\n", statFlipCount);
    sandbox_breakpoint();
  }
}
} // namespace

std::vector<Edge> triangulate_Flip(span<const Vec2> points)
{
  std::vector<HalfEdge> he = createBasicTriangulation(points);

  flipTriangulation(points, he);

  std::vector<Edge> r;
  for(int i = 0; i < (int)he.size(); ++i)
  {
    auto& edge = he[i];
    if(i > edge.twin)
      r.push_back({edge.point, he[edge.next].point});
  }

  return r;
}
