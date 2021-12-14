// Copyright (C) 2021 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Douglas-Peucker algorithm : polyline simplification.

#include <cmath>
#include <cstdio> // snprintf
#include <vector>

#include "algorithm_app.h"
#include "random.h"

namespace
{
struct Segment
{
  int a, b;
};

static float magnitude(Vec2 v) { return sqrt(v * v); }
static Vec2 normalize(Vec2 v) { return v * (1.0 / magnitude(v)); }
static float dot(Vec2 v, Vec2 w) { return v.x * w.x + v.y * w.y; }

static float DistanceBetweenLineAndPoint(Vec2 lineA, Vec2 lineB, Vec2 point)
{
  const Vec2 segment = (lineB - lineA);
  const float lengthSq = segment * segment;
  const float t = dot(point - lineA, segment) / lengthSq;
  const Vec2 projection = lineA + segment * t;
  return magnitude(projection - point);
}


void DrawRectangleAroundSegment(Vec2 start, Vec2 end, float width, Color color)
{
  const Vec2 direction = normalize(end - start);
  const Vec2 perpendicular{-direction.y, direction.x};
  const Vec2 corners[] =
  {
    start + perpendicular * width,
    end + perpendicular * width,
    end - perpendicular * width,
    start - perpendicular * width,
  };

  gVisualizer->line(start, end, color);
  gVisualizer->line(corners[0], corners[1], color);
  gVisualizer->line(corners[1], corners[2], color);
  gVisualizer->line(corners[2], corners[3], color);
  gVisualizer->line(corners[3], corners[0], color);
}

void DrawCross(Vec2 position, Color color)
{
  const float crossBoxSize = 0.5;
  const Vec2 corners[] =
  {
    position + Vec2(-crossBoxSize, -crossBoxSize),
    position + Vec2( crossBoxSize, -crossBoxSize),
    position + Vec2( crossBoxSize,  crossBoxSize),
    position + Vec2(-crossBoxSize,  crossBoxSize),
  };

  gVisualizer->line(corners[0], corners[2], color);
  gVisualizer->line(corners[1], corners[3], color);
}

std::vector<Segment> DouglasPeucker(const std::vector<Vec2>& input, float maxDistanceToSimplify, Segment range)
{
  const Vec2& start = input[range.a];
  const Vec2& end = input[range.b];

  DrawRectangleAroundSegment(start, end, maxDistanceToSimplify, Yellow);
  gVisualizer->step();

  if (range.b == range.a + 1)
  {
    std::vector<Segment> result;
    result.push_back(range);
    return result;
  }

  float maxDistance = 0.0f;
  int fartherIdx;
  for (int idx = range.a + 1; idx <= range.b - 1; idx++)
  {
    const float distance = DistanceBetweenLineAndPoint(start, end, input[idx]);
    if (maxDistance == 0.0 || maxDistance < distance)
    {
      maxDistance = distance;
      fartherIdx = idx;
    }
  }

  std::vector<Segment> result;
  DrawRectangleAroundSegment(start, end, maxDistanceToSimplify, Yellow);
  if (maxDistance <= maxDistanceToSimplify)
  {
    for (int idx = range.a + 1; idx <= range.b - 1; idx++)
      DrawCross(input[idx], Red);
    gVisualizer->step();

    result.push_back(range);
  }
  else
  {
    DrawCross(input[fartherIdx], Green);
    gVisualizer->step();

    std::vector<Segment> beforeFartherPoint = DouglasPeucker(input, maxDistanceToSimplify, {range.a, fartherIdx});
    std::vector<Segment> afterFartherPoint = DouglasPeucker(input, maxDistanceToSimplify, {fartherIdx, range.b});
    result.insert(result.end(), beforeFartherPoint.begin(), beforeFartherPoint.end());
    result.insert(result.end(), afterFartherPoint.begin(), afterFartherPoint.end());
  }

  return result;
}

struct DouglasPeuckerAlgorithm
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
    const float maxDistanceToSimplify = 3.0f;

    std::vector<Segment> result;
    result = DouglasPeucker(input, maxDistanceToSimplify, {0, int(input.size() - 1)});
    return result;
  }

  static void drawInput(IDrawer* drawer, const std::vector<Vec2>& input)
  {
    for(int idx = 0; idx < input.size(); ++idx)
    {
      char buffer[16];
      snprintf(buffer, sizeof buffer, "%d", idx);
      drawer->rect(input[idx] - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
      drawer->text(input[idx] + Vec2(0.3, 0), buffer, Red);

      const int next_idx = (idx + 1);
      if (next_idx < input.size())
        drawer->line(input[idx], input[next_idx]);
    }
  }

  static void drawOutput(IDrawer* drawer, const std::vector<Vec2>& input, const std::vector<Segment>& output)
  {
    for(auto& segment : output)
      drawer->line(input[segment.a], input[segment.b], Green);
  }
};

const int reg = registerApp("DouglasPeucker", [] () -> IApp* { return new AlgorithmApp<DouglasPeuckerAlgorithm>; });
}
