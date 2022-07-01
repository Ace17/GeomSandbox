// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Split a polygon into convex pieces

#include "core/algorithm_app.h"
#include "core/geom.h"
#include "core/sandbox.h"

#include <cassert>
#include <cmath>
#include <deque>
#include <vector>

#include "bounding_box.h"
#include "random.h"
#include "random_polygon.h"
#include "split_polygon.h"

namespace
{

constexpr auto epsilon = 0.01f;

const Color colors[] = {
      {0, 1, 0, 1},
      {0, 1, 1, 1},
      {1, 0, 0, 1},
      {1, 0, 1, 1},
      {1, 1, 0, 1},
      {1, 1, 1, 1},
      {0.5, 0.5, 1, 1},
      {0.5, 1, 0.5, 1},
      {0.5, 1, 1, 1},
      {1, 0.5, 0.5, 1},
      {1, 0.5, 1, 1},
};

Color choosePolygonColor(const Polygon2f& poly)
{
  Vec2 extremePos = poly.vertices[poly.faces[0].a];

  for(auto face : poly.faces)
  {
    auto v = poly.vertices[face.a];
    if(v.x > extremePos.x || (v.x == extremePos.x && v.y > extremePos.y))
      extremePos = v;
  }

  const int N = sizeof(colors) / sizeof(*colors);
  const unsigned idx = unsigned(std::abs(extremePos.x * extremePos.y * 123.456));
  return colors[idx % N];
}

void drawPolygon(const Polygon2f& poly, Color color)
{
  for(auto& face : poly.faces)
  {
    auto v0 = poly.vertices[face.a];
    auto v1 = poly.vertices[face.b];
    sandbox_line(v0, v1, color);
    sandbox_circle(v0, 0.1, color);

    // draw normal, pointing to the exterior
    const auto T = normalize(v1 - v0);
    const auto N = -rotateLeft(T);
    sandbox_line((v0 + v1) * 0.5, (v0 + v1) * 0.5 + N * 0.15, color);

    // draw hatches, on the interior
    const auto hatchAttenuation = 0.4f;
    const Color hatchColor = {color.r * hatchAttenuation, color.g * hatchAttenuation, color.b * hatchAttenuation,
          color.a * hatchAttenuation};
    const auto dist = magnitude(v1 - v0);
    for(float f = 0; f < dist; f += 0.15)
    {
      const auto p = v0 + T * f;
      sandbox_line(p, p - N * 0.15 + T * 0.05, hatchColor);
    }
  }
}

bool isConvex(const Polygon2f& poly)
{
  for(auto face : poly.faces)
  {
    const auto T = poly.vertices[face.b] - poly.vertices[face.a];
    const auto N = -rotateLeft(T); // normals point to the outside
    for(auto vertex : poly.vertices)
    {
      auto dv = vertex - poly.vertices[face.a];
      if(dot_product(dv, N) > epsilon)
        return false;
    }
  }

  return true;
}

Plane chooseCuttingPlane(const Polygon2f& poly)
{
  Plane bestPlane;
  int bestScore = 0;

  for(auto face : poly.faces)
  {
    const auto T = poly.vertices[face.b] - poly.vertices[face.a];
    const auto N = normalize(-rotateLeft(T));
    const auto plane = Plane{N, dot_product(N, poly.vertices[face.a])};

    int frontCount = 0;
    int backCount = 0;

    for(auto v : poly.vertices)
    {
      if(dot_product(v, plane.normal) > plane.dist + epsilon)
        frontCount++;
      else
        backCount++;
    }

    const int score = std::min(frontCount, backCount);
    if(score > bestScore)
    {
      bestScore = score;
      bestPlane = plane;
    }
  }

  return bestPlane;
}

std::vector<Polygon2f> decomposePolygonToConvexParts(const Polygon2f& input)
{
  std::deque<Polygon2f> fifo;
  fifo.push_back(input);

  std::vector<Polygon2f> result;

  while(fifo.size())
  {
    auto poly = fifo.front();
    fifo.pop_front();

    if(poly.faces.empty())
      continue;

    if(isConvex(poly))
    {
      result.push_back(poly);
      continue;
    }

    auto plane = chooseCuttingPlane(poly);

    {
      for(auto& p : result)
        drawPolygon(p, White);
      for(auto& p : fifo)
        drawPolygon(p, Gray);

      drawPolygon(poly, LightBlue);

      const auto T = rotateLeft(plane.normal);
      auto p = plane.normal * plane.dist;
      auto shift = plane.normal * 0.1;
      auto tmin = p + T * +1000;
      auto tmax = p + T * -1000;
      sandbox_line(tmin + shift, tmax + shift, Yellow);
      sandbox_line(tmin - shift, tmax - shift, Yellow);
      sandbox_breakpoint();
    }

    Polygon2f front, back;
    splitPolygonAgainstPlane(poly, plane, front, back);

    fifo.push_back(front);
    fifo.push_back(back);

    {
      for(auto& p : result)
        drawPolygon(p, White);
      for(auto& p : fifo)
        drawPolygon(p, Gray);

      drawPolygon(front, Red);
      drawPolygon(back, Green);
      sandbox_breakpoint();
    }
  }

  return result;
}

struct FastConvexSplit
{
  static Polygon2f generateInput()
  {
    auto input = createRandomPolygon2f();
    BoundingBox bbox;

    for(const Vec2 vertex : input.vertices)
      bbox.add(vertex);

    const int idx = (int)input.vertices.size();

    input.vertices.push_back({bbox.min.x * 1.1f, bbox.min.y * 1.1f});
    input.vertices.push_back({bbox.max.x * 1.1f, bbox.min.y * 1.1f});
    input.vertices.push_back({bbox.max.x * 1.1f, bbox.max.y * 1.1f});
    input.vertices.push_back({bbox.min.x * 1.1f, bbox.max.y * 1.1f});

    input.faces.push_back({idx + 0, idx + 1});
    input.faces.push_back({idx + 1, idx + 2});
    input.faces.push_back({idx + 2, idx + 3});
    input.faces.push_back({idx + 3, idx + 0});

    return input;
  }

  static std::vector<Polygon2f> execute(Polygon2f input) { return decomposePolygonToConvexParts(input); }

  static void display(const Polygon2f& input, span<const Polygon2f> output)
  {
    drawPolygon(input, Gray);

    for(auto& poly : output)
      drawPolygon(poly, choosePolygonColor(poly));
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<FastConvexSplit>>()); }
const int registered = registerApp("FastConvexSplit", &create);
}
