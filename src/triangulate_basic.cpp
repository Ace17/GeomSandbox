// Copyright (C) 2026 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// build a basic triangulation by sweeping the input points left-to-right
#include "triangulate_basic.h"

#include "core/geom.h"
#include "core/sandbox.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <vector>

namespace
{
const bool EnableTrace = true;

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

void printPolygon(span<const Vec2> points, float margin)
{
  const int N = (int)points.len;

  std::vector<Vec2> insetTx(N);

  for(int i = 0; i < N; ++i)
  {
    const Vec2 pPrev = points[(i - 1 + N) % N];
    const Vec2 pCurr = points[i];
    const Vec2 pNext = points[(i + 1) % N];

    const Vec2 N1 = normalize(rotateLeft(pCurr - pPrev));
    const Vec2 N2 = normalize(rotateLeft(pNext - pCurr));

    const float delta = det2d(N1, N2);

    Vec2 tx;
    tx.x = +margin / delta * (N2.y - N1.y);
    tx.y = -margin / delta * (N2.x - N1.x);
    insetTx[i] = tx;
  }

  for(int i = 0; i < N; ++i)
  {
    const Vec2 curr = points[i];
    const Vec2 next = points[(i + 1) % N];

    sandbox_line(curr, next, White);
    sandbox_line(curr, next, Gray, insetTx[i], insetTx[(i + 1) % N]);
  }
}

void printEdgeChain(span<const HalfEdge> he, span<const Vec2> points, int firstEdge, span<char> hit)
{
  std::vector<int> edgeChain;
  std::vector<Vec2> polygon;

  // compute edgeChain
  {
    int curr = firstEdge;

    do
    {
      const int next = he[curr].next;
      const int p0 = he[curr].point;
      if(p0 == -1 || next == -1)
      {
        sandbox_text({}, "UAF", Red);
        break;
      }
      if(hit[curr])
      {
        sandbox_text(points[p0], "ERR", Red);
        sandbox_circle(points[p0], 0, Red, 8);
        break;
      }

      edgeChain.push_back(curr);
      polygon.push_back(points[p0]);
      hit[curr] = 1;

      curr = next;
    } while(curr != firstEdge);
  }

  printPolygon(polygon, 8);

  // print labels

  const int N = (int)edgeChain.size();

  for(int i = 0; i < N; ++i)
  {
    const int curr = edgeChain[i];
    const int next = edgeChain[(i + 1) % N];

    const int p0 = he[curr].point;
    const int p1 = he[next].point;

    char buf[256];
    sprintf(buf, "%d", curr);
    const Vec2 N2 = rotateLeft(normalize(points[p1] - points[p0]));
    sandbox_text((points[p0] + points[p1]) * 0.5, buf, {0.3, 0.3, 0.3, 0.3}, N2 * 20);
  }
}

void printMesh(span<const HalfEdge> he, span<const Vec2> points)
{
  std::vector<char> hit(he.len);

  for(int i = 0; i < (int)he.len; ++i)
    if(!hit[i] && he[i].point != -1)
      printEdgeChain(he, points, i, hit);

  sandbox_circle(points[he[0].point], 0, Green, 8);
}

std::vector<int> sortPointsFromLeftToRight(span<const Vec2> points)
{
  std::vector<int> order(points.len);

  for(int i = 0; i < (int)points.len; ++i)
    order[i] = i;

  auto byCoordinates = [&](int ia, int ib)
  {
    auto a = points[ia];
    auto b = points[ib];

    if(a.x != b.x)
      return a.x < b.x;

    if(a.y != b.y)
      return a.y < b.y;

    return true;
  };

  std::sort(order.begin(), order.end(), byCoordinates);

  return order;
}

void blankHull(span<HalfEdge> he)
{
  int curr = 0;

  do
  {
    he[curr].point = -1;
    curr = he[curr].next;
  } while(curr != 0);
}

std::vector<HalfEdge> removeBlankEdges(span<const HalfEdge> he)
{
  std::vector<HalfEdge> r;
  std::vector<int> oldToNew(he.len, -1);
  for(int oldIndex = 0; oldIndex < (int)he.len; ++oldIndex)
  {
    const HalfEdge& oldEdge = he[oldIndex];

    if(oldEdge.point == -1)
      continue; // ignore blank edges

    if(oldToNew[oldIndex] == -1)
    {
      oldToNew[oldIndex] = r.size();
      r.push_back({});
    }

    if(oldToNew[oldEdge.next] == -1)
    {
      oldToNew[oldEdge.next] = r.size();
      r.push_back({});
    }

    if(oldEdge.twin != -1 && oldToNew[oldEdge.twin] == -1)
    {
      oldToNew[oldEdge.twin] = r.size();
      r.push_back({});
    }

    r[oldToNew[oldIndex]].point = oldEdge.point;
    r[oldToNew[oldIndex]].next = oldToNew[oldEdge.next];
    r[oldToNew[oldIndex]].twin = oldEdge.twin != -1 ? oldToNew[oldEdge.twin] : -1;
  }
  return r;
}

std::pair<int, int> computeVisibleChain(span<const HalfEdge> he, span<const Vec2> points, Vec2 p)
{
  int visFirst = -1;
  int visLast = -1;
  int hullCurr = 0;

  do
  {
    const int hullNext = he[hullCurr].next;

    const int p0 = he[hullCurr].point;
    const int p1 = he[hullNext].point;

    const auto a = points[p0];
    const auto b = points[p1];

    if(det2d(b - a, p - a) > 0.001)
    {
      if(visFirst == -1)
        visFirst = hullCurr;
      visLast = hullNext;

      if(EnableTrace)
        sandbox_line(a, b, Orange);
    }

    hullCurr = hullNext;
  } while(hullCurr != 0);

  if(EnableTrace)
  {
    sandbox_text(points[he[visFirst].point], "First", Orange);
    sandbox_text(points[he[visLast].point], "Last", Orange);
    sandbox_circle(p, 0, Orange, 8);
    sandbox_breakpoint();
  }

  return {visFirst, visLast};
}

}

std::vector<HalfEdge> createBasicTriangulation(span<const Vec2> points)
{
  std::vector<HalfEdge> he;
  std::map<std::pair<int, int>, int> pointToEdge;

  auto findHalfEdge = [&](int a, int b)
  {
    auto i = pointToEdge.find({a, b});

    if(i == pointToEdge.end())
      return -1;

    return i->second;
  };

  auto allocHalfEdge = [&]()
  {
    const auto r = (int)he.size();
    he.push_back({});
    return r;
  };

  const auto order = sortPointsFromLeftToRight(points);
  span<const int> queue = order;

  if(points.len < 3)
    return {};

  // Init the hull. By convention, he[0] is always its leftmost edge.
  {
    int i0 = queue.pop();
    int i1 = queue.pop();

    he.resize(2);
    he[0] = {i0, 1};
    he[1] = {i1, 0};
  }

  if(EnableTrace)
  {
    printMesh(he, points);
    sandbox_breakpoint();
  }

  while(queue.len > 0)
  {
    const int idx = queue.pop();
    const auto p = points[idx];

    if(EnableTrace)
      printMesh(he, points);

    const auto [visFirst, visLast] = computeVisibleChain(he, points, p);

    // create each triangle composed of p and an edge of the visible chain
    int visEdge = visFirst;
    while(visEdge != visLast)
    {
      const int hullNext = he[visEdge].next;

      const int p0 = he[visEdge].point;
      const int p1 = he[hullNext].point;
      const int p2 = idx;

      const auto e0 = allocHalfEdge();
      const auto e1 = allocHalfEdge();
      const auto e2 = allocHalfEdge();
      he[e0] = {p0, e1};
      he[e1] = {p1, e2};
      he[e2] = {p2, e0};

      // search for a twin for e0 (=the edge that links [p0 -> p1])
      he[e0].twin = findHalfEdge(p1, p0);
      he[e1].twin = findHalfEdge(p2, p1);
      he[e2].twin = findHalfEdge(p0, p2);

      if(he[e0].twin != -1)
        he[he[e0].twin].twin = e0;
      if(he[e1].twin != -1)
        he[he[e1].twin].twin = e1;
      if(he[e2].twin != -1)
        he[he[e2].twin].twin = e2;

      pointToEdge[{p0, p1}] = e0;
      pointToEdge[{p1, p2}] = e1;
      pointToEdge[{p2, p0}] = e2;

      // we only reuse the first edge, blank the other visible hull edges
      if(visEdge != visFirst)
        he[visEdge].point = -1;

      visEdge = hullNext;
    }

    // modify the hull to include the added point 'idx'
    {
      const int n = allocHalfEdge();
      he[n] = {idx, visLast};
      he[visFirst].next = n;
    }

    if(EnableTrace)
    {
      printMesh(he, points);
      sandbox_breakpoint();
    }
  }

  blankHull(he);
  return removeBlankEdges(he);
}
