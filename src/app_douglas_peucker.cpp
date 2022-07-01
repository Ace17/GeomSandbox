// Copyright (C) 2021 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Douglas-Peucker algorithm : polyline simplification.

#include "core/algorithm_app.h"
#include "core/sandbox.h"

#include <cmath>
#include <cstdio> // snprintf
#include <vector>

#include "random.h"

namespace
{
struct Segment
{
  int a, b;
};

static float clamp(float value, float min, float max) { return std::min(max, std::max(min, value)); }

static float distanceBetweenLineAndPoint(Vec2 lineA, Vec2 lineB, Vec2 point)
{
  const Vec2 segment = (lineB - lineA);
  const float lengthSq = segment * segment;
  const float t = clamp(dot_product(point - lineA, segment) / lengthSq, 0, 1);
  const Vec2 projection = lineA + segment * t;
  return magnitude(projection - point);
}

void drawHalfCircle(Vec2 center, Vec2 direction, float radius, Color color)
{
  const int N = 20;
  Vec2 halfCirclePoints[N];

  const float startAngle = asin(direction.x);
  for(int i = 0; i < N; i++)
  {
    float angle = startAngle + 3.14 * i / (N - 1);
    if(direction.y > 0)
      angle = -angle;
    halfCirclePoints[i].x = center.x + cos(angle) * radius;
    halfCirclePoints[i].y = center.y + sin(angle) * radius;
  }

  for(int i = 0; i < N - 1; i++)
  {
    sandbox_line(halfCirclePoints[i], halfCirclePoints[i + 1], color);
  }
}

void drawDistanceShapeAroundSegment(Vec2 start, Vec2 end, float width, Color color)
{
  const Vec2 direction = normalize(end - start);
  const Vec2 perpendicular = {-direction.y, direction.x};
  const Vec2 corners[] = {
        start + perpendicular * width,
        end + perpendicular * width,
        end - perpendicular * width,
        start - perpendicular * width,
  };

  sandbox_line(start, end, color);
  sandbox_line(corners[0], corners[1], color);
  sandbox_line(corners[2], corners[3], color);

  drawHalfCircle(start, direction, width, color);
  drawHalfCircle(end, -direction, width, color);
}

void drawPointWithIdentifier(Vec2 position, int index, Color pointColor, Color textColor)
{
  char buffer[16];
  snprintf(buffer, sizeof buffer, "%d", index);
  sandbox_rect(position - Vec2(0.2, 0.2), Vec2(0.4, 0.4), pointColor);
  sandbox_text(position + Vec2(0.3, 0), buffer, textColor);
}

void drawContainedPointsIdentifiers(const std::vector<Vec2>& input, Segment range, Color color)
{
  for(int idx = range.a + 1; idx < range.b; idx++)
  {
    drawPointWithIdentifier(input[idx], idx, color, color);
  }
}

void drawCross(Vec2 position, Color color)
{
  const float crossBoxSize = 0.5;
  const Vec2 corners[] = {
        position + Vec2(-crossBoxSize, -crossBoxSize),
        position + Vec2(crossBoxSize, -crossBoxSize),
        position + Vec2(crossBoxSize, crossBoxSize),
        position + Vec2(-crossBoxSize, crossBoxSize),
  };

  sandbox_line(corners[0], corners[2], color);
  sandbox_line(corners[1], corners[3], color);
}

std::vector<Segment> simplify_DouglasPeucker(const std::vector<Vec2>& input, float maxDistanceToSimplify, Segment range)
{
  const Vec2& start = input[range.a];
  const Vec2& end = input[range.b];

  drawDistanceShapeAroundSegment(start, end, maxDistanceToSimplify, Yellow);
  drawContainedPointsIdentifiers(input, range, Yellow);
  sandbox_breakpoint();

  if(range.b == range.a + 1)
  {
    std::vector<Segment> result;
    result.push_back(range);
    return result;
  }

  float maxDistance = 0.0f;
  int fartherIdx;
  for(int idx = range.a + 1; idx <= range.b - 1; idx++)
  {
    const float distance = distanceBetweenLineAndPoint(start, end, input[idx]);
    if(maxDistance == 0.0 || maxDistance < distance)
    {
      maxDistance = distance;
      fartherIdx = idx;
    }
  }

  std::vector<Segment> result;
  drawDistanceShapeAroundSegment(start, end, maxDistanceToSimplify, Yellow);
  if(maxDistance <= maxDistanceToSimplify)
  {
    for(int idx = range.a + 1; idx <= range.b - 1; idx++)
      drawCross(input[idx], Red);
    sandbox_breakpoint();

    result.push_back(range);
  }
  else
  {
    drawPointWithIdentifier(input[fartherIdx], fartherIdx, Green, Green);
    sandbox_breakpoint();

    std::vector<Segment> beforeFartherPoint =
          simplify_DouglasPeucker(input, maxDistanceToSimplify, {range.a, fartherIdx});
    std::vector<Segment> afterFartherPoint =
          simplify_DouglasPeucker(input, maxDistanceToSimplify, {fartherIdx, range.b});
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
    result = simplify_DouglasPeucker(input, maxDistanceToSimplify, {0, int(input.size() - 1)});
    return result;
  }

  static void display(const std::vector<Vec2>& input, const std::vector<Segment>& output)
  {
    for(int idx = 0; idx < input.size(); ++idx)
    {
      drawPointWithIdentifier(input[idx], idx, White, Red);

      const int next_idx = (idx + 1);
      if(next_idx < input.size())
        sandbox_line(input[idx], input[next_idx]);
    }

    for(auto& segment : output)
      sandbox_line(input[segment.a], input[segment.b], Green);
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<DouglasPeuckerAlgorithm>>()); }
const int reg = registerApp("DouglasPeucker", &create);
}
