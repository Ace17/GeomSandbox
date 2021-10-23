// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Triangulation, Bowyer-Watson algorithm

#include "geom.h"
#include "visualizer.h"
#include <vector>

struct Edge
{
  int a, b;
};

namespace
{
static Vec2 rotateLeft(Vec2 v) { return Vec2(-v.y, v.x); }

struct Triangle
{
  Edge edges[3];
  Vec2 circleCenter;
  float circleSquaredRadius;
};

bool pointInsideCircle(Vec2 point, Vec2 center, float squaredRadius)
{
  auto delta = point - center;
  return delta * delta < squaredRadius;
}

Triangle makeTriangle(int p0, int p1, int p2, span<const Vec2> coords)
{
  auto c0 = coords[p0];
  auto c1 = coords[p1];
  auto c2 = coords[p2];

  Triangle r;
  r.edges[0] = { p0, p1 };
  r.edges[1] = { p1, p2 };
  r.edges[2] = { p2, p0 };

  // compute both perpendicular bissectors
  auto A = (c0 + c1) * 0.5;
  auto tA = c1 - c0; // tangent

  auto B = (c0 + c2) * 0.5;
  auto nB = rotateLeft(c2 - c0); // normal

  // Let I be the intersection of both bissectors.
  // We know that:
  // 1) I = B + k * nB
  // 2) AI.tA = 0
  // So:
  // k = - AB.tA / nB.tA

  const auto k = -((B - A) * tA) / (nB * tA);

  const auto I = B + nB * k;

  r.circleCenter = I;
  r.circleSquaredRadius = (I - c0) * (I - c0);

  return r;
}

Triangle createSuperTriangle(std::vector<Vec2>& coords)
{
  float min_x = coords[0].x;
  float max_x = coords[0].x;
  float min_y = coords[0].y;
  float max_y = coords[0].y;

  for(auto& p : coords)
  {
    min_x = std::min(min_x, p.x);
    max_x = std::max(max_x, p.x);
    min_y = std::min(min_y, p.y);
    max_y = std::max(max_y, p.y);
  }

  const float margin = std::max(max_x - min_x, max_y - min_y) * 10.0f;

  min_x -= margin;
  max_x += margin;
  min_y -= margin;
  min_y -= margin;

  const int i0 = (int)coords.size();
  coords.push_back({ min_x, min_y });
  const int i1 = (int)coords.size();
  coords.push_back({ max_x + (max_x - min_x) * 2.0f, min_y });
  const int i2 = (int)coords.size();
  coords.push_back({ min_x, max_y + (max_y - min_y) * 2.0f });

  return makeTriangle(i0, i1, i2, coords);
}

// reorder 'elements' so that it can be split into two parts:
// - [0 .. r[ : triangle circles don't contain 'point'
// - [r .. N[ : triangle circles contain 'point'
// Returns 'r'.
int reorder(span<Triangle> triangles, Vec2 point)
{
  int result = triangles.len;

  for(int i = 0; i < result;)
  {
    if(pointInsideCircle(point, triangles[i].circleCenter, triangles[i].circleSquaredRadius))
      std::swap(triangles[i], triangles[--result]);
    else
      ++i;
  }

  return result;
}

void print2d(span<const Triangle> triangulation, span<const Vec2> points)
{
  visu->begin();

  for(auto t : triangulation)
    for(auto e : t.edges)
      visu->line(points[e.a], points[e.b]);

  visu->end();
}
}

std::vector<Edge> triangulate_BowyerWatson(span<const Vec2> inputCoords)
{
  std::vector<Vec2> points(inputCoords.len);

  for(int i = 0; i < (int)inputCoords.len; ++i)
    points[i] = inputCoords[i];

  std::vector<Triangle> triangulation;
  triangulation.push_back(createSuperTriangle(points));

  for(int p = 0; p < (int)inputCoords.len; ++p)
  {
    std::vector<Edge> edges;

    const int s = reorder(triangulation, points[p]);

    for(int i = s; i < (int)triangulation.size(); ++i)
      for(auto& e : triangulation[i].edges)
        edges.push_back(e);

    triangulation.resize(s); // drop impacted triangles

    std::vector<int> edgeIsOnCountour(edges.size(), true);

    for(int j = 0; j < (int)edges.size(); ++j)
      for(int k = j + 1; k < (int)edges.size(); ++k)
        if((edges[j].a == edges[k].a && edges[j].b == edges[k].b) || (edges[j].a == edges[k].b && edges[j].b == edges[k].a))
          edgeIsOnCountour[j] = edgeIsOnCountour[k] = false;

    for(int j = 0; j < (int)edges.size(); ++j)
    {
      if(edgeIsOnCountour[j])
        triangulation.push_back(makeTriangle(edges[j].b, edges[j].a, p, points));
    }

    print2d(triangulation, points);
  }

  std::vector<Edge> edges;

  for(auto t : triangulation)
    for(auto e : t.edges)
    {
      // filter out edges connected to the super triangle
      if(e.a < (int)inputCoords.len && e.b < (int)inputCoords.len)
        edges.push_back(e);
    }

  return edges;
}

