// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Split a polygon into convex pieces

#include <cmath>
#include <deque>
#include <vector>

#include "algorithm_app.h"
#include "bounding_box.h"
#include "drawer.h"
#include "geom.h"
#include "random.h"
#include "random_polygon.h"
#include "split_polygon.h"

namespace
{

static constexpr auto epsilon = 0.01f;

float abs(float a) { return a >= 0 ? a : -a; }
float magnitude(Vec2 v) { return sqrt(v * v); }
float dot_product(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
Vec2 normalize(Vec2 v) { return v * (1.0 / magnitude(v)); }
Vec2 rotateLeft(Vec2 v) { return Vec2(-v.y, v.x); }

static const Color colors[] = {
      {0, 0, 1, 1},
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

struct FastConvexSplit
{
  static Polygon2f generateInput()
  {
    auto input = createRandomPolygon2f(gNullVisualizer);
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

  static std::vector<Polygon2f> execute(Polygon2f input)
  {
    std::deque<Polygon2f> fifo;
    fifo.push_back(input);

    std::vector<Polygon2f> result;

    while(fifo.size())
    {
      auto poly = fifo.front();
      fifo.pop_front();

      drawPolygon(gVisualizer, poly, Red);

      if(isConvex(poly))
      {
        result.push_back(poly);
      }
      else
      {
        auto plane = chooseCuttingPlane(poly);

        Polygon2f front, back;
        splitPolygonAgainstPlane(poly, plane, front, back);

        if(front.faces.size())
          fifo.push_back(front);

        if(back.faces.size())
          fifo.push_back(back);

        {
          const auto T = rotateLeft(plane.normal);
          auto p = plane.normal * plane.dist;
          auto shift = plane.normal * 0.1;
          auto tmin = p + T * +1000;
          auto tmax = p + T * -1000;
          gVisualizer->line(tmin + shift, tmax + shift, LightBlue);
          gVisualizer->line(tmin - shift, tmax - shift, LightBlue);
        }
      }

      gVisualizer->step();

      {
        int i = 0;
        for(auto& p : fifo)
        {
          drawPolygon(gVisualizer, p, colors[i]);
          i++;
          i %= (sizeof colors) / (sizeof *colors);
        }

        gVisualizer->step();
      }
    }

    return result;
  }

  static bool isConvex(const Polygon2f& poly)
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

  static Plane chooseCuttingPlane(const Polygon2f& poly)
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

  static void drawInput(IDrawer* drawer, const Polygon2f& input) { drawPolygon(drawer, input, Gray); }

  static void drawOutput(IDrawer* drawer, const Polygon2f& input, const std::vector<Polygon2f>& output)
  {
    int i = 0;
    for(auto poly : output)
    {
      drawPolygon(drawer, poly, colors[i]);
      ++i;
      i %= (sizeof colors) / (sizeof *colors);
    }
  }

  static void drawPolygon(IDrawer* drawer, const Polygon2f& poly, Color color)
  {
    for(auto& face : poly.faces)
    {
      auto v0 = poly.vertices[face.a];
      auto v1 = poly.vertices[face.b];
      drawer->line(v0, v1, color);
      drawer->circle(v0, 0.1, color);

      // draw normal, pointing to the exterior
      const auto T = normalize(v1 - v0);
      const auto N = -rotateLeft(T);
      drawer->line((v0 + v1) * 0.5, (v0 + v1) * 0.5 + N * 0.15, color);

      // draw hatches, on the interior
      const auto hatchAttenuation = 0.4f;
      const Color hatchColor = {color.r * hatchAttenuation, color.g * hatchAttenuation, color.b * hatchAttenuation,
            color.a * hatchAttenuation};
      const auto dist = magnitude(v1 - v0);
      for(float f = 0; f < dist; f += 0.15)
      {
        const auto p = v0 + T * f;
        drawer->line(p, p - N * 0.15 + T * 0.05, hatchColor);
      }
    }
  }
};

const int registered = registerApp("FastConvexSplit", []() -> IApp* { return new AlgorithmApp<FastConvexSplit>; });
}
