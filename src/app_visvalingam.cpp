// Copyright (C) 2022 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Visvalingam–Whyatt algorithm : polyline or polygon simplification.

#include "core/algorithm_app.h"
#include "core/sandbox.h"

#include <algorithm>
#include <array>
#include <vector>

#include "random.h"

namespace
{
struct Segment
{
  int a, b;
};

struct Vertex
{
  int index;
  Vec2 position;
  Color color;
  float area;
};

Color getColorFromIndex(int index)
{

  constexpr Color colors[] = {
        {1, 0, 0, 1},
        {0, 1, 1, 1},
        {1, 0, 1, 1},
        {1, 1, 0, 1},
        {0.5, 0.5, 0.5, 1},
        {0.5, 0.5, 1, 1},
        {0.5, 1, 0.5, 1},
        {1, 0.5, 0.5, 1},
        {0.0, 0.5, 1, 1},
        {0.5, 1, 0.0, 1},
        {1, 0.0, 0.5, 1},
  };

  constexpr int count = sizeof(colors) / sizeof(*colors);
  return colors[index % count];
}

float clamp(float value, float min, float max) { return std::min(max, std::max(min, value)); }

float magnitudeSq(Vec2 v) { return v * v; }

float distanceBetweenLineAndPoint(Vec2 lineA, Vec2 lineB, Vec2 point)
{
  const Vec2 segment = (lineB - lineA);
  const float lengthSq = segment * segment;
  const float t = clamp(dotProduct(point - lineA, segment) / lengthSq, 0, 1);
  const Vec2 projection = lineA + segment * t;
  return magnitude(projection - point);
}

float triangleArea(Vec2 a, Vec2 b, Vec2 c)
{
  struct TriangleEdge
  {
    Vec2 baseA;
    Vec2 baseB;
    Vec2 topPoint;
  };
  std::array<TriangleEdge, 3> edges = {{
        {a, b, c},
        {b, c, a},
        {c, a, b},
  }};
  auto CompareEdges = [](const TriangleEdge& a, const TriangleEdge& b) {
    return magnitudeSq(a.baseB - a.baseA) < magnitudeSq(b.baseB - b.baseA);
  };
  const auto maxEdgeIt = std::max_element(edges.begin(), edges.end(), CompareEdges);
  const float height = distanceBetweenLineAndPoint(maxEdgeIt->baseA, maxEdgeIt->baseB, maxEdgeIt->topPoint);
  return magnitude(maxEdgeIt->baseB - maxEdgeIt->baseA) * height / 2.f;
}

void drawAreas(const std::vector<Vertex>& vertices)
{
  for(int i = 0; i < (int)vertices.size() - 1; ++i)
  {
    const Vertex& vertex = vertices[i];
    sandbox_line(vertex.position, vertices[i + 1].position, Green);

    if(i > 0)
    {
      const Color color = vertex.color;
      const Vec2 prevPos = vertices[i - 1].position;
      const Vec2 nextPos = vertices[i + 1].position;
      sandbox_line(prevPos, nextPos, color);

      const Vec2 newEdgeCenter = (prevPos + nextPos) / 2.f;
      char buffer[16];
      snprintf(buffer, sizeof buffer, "%.2f", vertex.area);
      sandbox_text(newEdgeCenter + Vec2(0.3, 0), buffer, color);
    }
  }
}

struct VisvalingamAlgorithm
{
  static std::vector<Vec2> generateInput()
  {
    std::vector<Vec2> points;
    const int N = int(randomFloat(3, 15));
    const float length = 40.0f;

    for(int i = 0; i < N; ++i)
    {
      Vec2 pos;
      pos.x = -length / 2 + length * i / N;
      pos.y = randomFloat(-10, 10);
      points.push_back(pos);
    }

    return points;
  }

  static std::vector<Segment> execute(std::vector<Vec2> input)
  {
    std::vector<Vertex> vertices;
    vertices.reserve(input.size());
    for(int i = 0; i < (int)input.size(); ++i)
    {
      float area = 0.f;
      const bool isFirstOrLastVertex = (i == 0 || i == (int)input.size() - 1);
      if(!isFirstOrLastVertex)
      {
        const Vec2 a = input[i - 1];
        const Vec2 b = input[i];
        const Vec2 c = input[i + 1];
        area = triangleArea(a, b, c);
      }
      vertices.push_back({i, input[i], getColorFromIndex(i), area});
      if(i > 0)
      {
        drawAreas(vertices);
        sandbox_breakpoint();
      }
    }

    const float minAreaToKeep = 15.f;

    auto CompareAreas = [](Vertex a, Vertex b) { return a.area < b.area; };
    auto minAreaIt = std::min_element(vertices.begin() + 1, vertices.end() - 1, CompareAreas);
    while(minAreaIt->area < minAreaToKeep)
    {
      if(minAreaIt - 1 != vertices.begin())
      {
        const Vec2 a = (minAreaIt - 2)->position;
        const Vec2 b = (minAreaIt - 1)->position;
        const Vec2 c = (minAreaIt + 1)->position;
        (minAreaIt - 1)->area = triangleArea(a, b, c);
      }
      if(minAreaIt + 1 != vertices.end() - 1)
      {
        const Vec2 a = (minAreaIt - 1)->position;
        const Vec2 b = (minAreaIt + 1)->position;
        const Vec2 c = (minAreaIt + 2)->position;
        (minAreaIt + 1)->area = triangleArea(a, b, c);
      }
      vertices.erase(minAreaIt);

      drawAreas(vertices);
      sandbox_breakpoint();
      minAreaIt = std::min_element(vertices.begin() + 1, vertices.end() - 1, CompareAreas);
    }

    std::vector<Segment> output;
    output.reserve(vertices.size() - 1);
    for(int i = 0; i < (int)vertices.size() - 1; ++i)
      output.push_back({vertices[i].index, vertices[i + 1].index});
    return output;
  }

  static void display(span<const Vec2> input, span<const Segment> output)
  {
    for(int idx = 0; idx < (int)input.len; ++idx)
    {
      sandbox_rect(input[idx] - Vec2(0.2, 0.2), Vec2(0.4, 0.4), White);

      const int next_idx = (idx + 1);
      if(next_idx < (int)input.len)
        sandbox_line(input[idx], input[next_idx], White);
    }

    for(auto& segment : output)
      sandbox_line(input[segment.a], input[segment.b], Green);
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<VisvalingamAlgorithm>>()); }
const int reg = registerApp("Visvalingam", &create);
}
