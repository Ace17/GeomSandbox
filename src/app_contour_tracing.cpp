// Copyright (C) 2022 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <algorithm>
#include <array>
#include <vector>

#include "random.h"

namespace
{

constexpr int gridWidth = 30;
constexpr int gridHeight = 30;
constexpr int tileCount = gridWidth * gridHeight;

constexpr float tileRenderSize = 1.0;
constexpr float borderCornerOffset = 0.2;

using Grid = std::array<bool, tileCount>;
using PolygonBorder = std::vector<Vec2>;

struct Coord
{
  int x, y;

  bool operator==(const Coord& other) const { return x == other.x && y == other.y; }

  Coord operator+(const Coord& other) const { return {x + other.x, y + other.y}; }
  Coord operator-(const Coord& other) const { return {x - other.x, y - other.y}; }
};

Coord rotateLeft(Coord c) { return Coord({-c.y, c.x}); }
int clamp(int val, int min, int max) { return val < min ? min : (val > max ? max : val); }

struct TSegment
{
  Coord a, b;

  bool operator==(const TSegment& other) const { return a == other.a && b == other.b; }
};

float lerp(float a, float b, float r) { return (a * (1.f - r)) + (b * r); }

int tileIndex(const Coord& coord) { return coord.y * gridWidth + coord.x; }

bool tileIsFilled(const Grid& grid, const Coord& coord)
{
  if(coord.x < 0 || coord.y < 0 || coord.x >= gridWidth || coord.y >= gridHeight)
    return false;
  return grid[tileIndex(coord)];
}

Vec2 tileRenderPosition(Coord coord)
{
  const float renderX = coord.x * tileRenderSize - gridWidth * tileRenderSize / 2.f;
  const float renderY = coord.y * tileRenderSize - gridHeight * tileRenderSize / 2.f;
  return {renderX, renderY};
}

void drawGridLines()
{
  for(int y = 0; y < gridHeight + 1; y++)
  {
    sandbox_line(tileRenderPosition({0, y}), tileRenderPosition({gridWidth, y}), White);
  }
  for(int x = 0; x < gridWidth + 1; x++)
  {
    sandbox_line(tileRenderPosition({x, 0}), tileRenderPosition({x, gridHeight}), White);
  }
}

void drawFilledTile(int x, int y, const Color& color)
{
  const Vec2 position = tileRenderPosition({x, y});
  const Vec2 size = Vec2(tileRenderSize, tileRenderSize);
  const Vec2 tileEnd = position + size;
  const int traceCount = 5;
  for(int i = 0; i < traceCount; i++)
  {
    const float traceRatio = float(i + 1) / float(traceCount + 1);
    if(traceRatio < 0.5f)
    {
      const float ratio = traceRatio * 2.f;
      const Vec2 start = {lerp(position.x, tileEnd.x, ratio), position.y};
      const Vec2 end = {position.x, lerp(position.y, tileEnd.y, ratio)};
      sandbox_line(start, end, color);
    }
    else
    {
      const float ratio = (traceRatio - 0.5f) * 2.f;
      const Vec2 start = {lerp(position.x, tileEnd.x, ratio), tileEnd.y};
      const Vec2 end = {tileEnd.x, lerp(position.y, tileEnd.y, ratio)};
      sandbox_line(start, end, color);
    }
  }
}

void drawTestedCellBorders(int x, int y, const Color& color)
{
  const Vec2 topLeft = tileRenderPosition({x, y}) + Vec2(0.f, tileRenderSize);
  const Vec2 topRight = topLeft + Vec2(tileRenderSize, 0.f);
  const Vec2 bottomRight = topRight + Vec2(0.f, -tileRenderSize);
  sandbox_line(topLeft, topRight, color);
  sandbox_line(topRight, bottomRight, color);
}

void drawSegment(const TSegment& segment, const Color& color)
{
  constexpr float arrowTipLength = 0.5f;
  constexpr float arrowTipWidth = 0.3f;

  const Vec2 segmentRenderStart = tileRenderPosition(segment.a);
  const Vec2 segmentRenderEnd = tileRenderPosition(segment.b);

  const Vec2 direction = segmentRenderEnd - segmentRenderStart;
  const Vec2 perpendicular = rotateLeft(direction);
  const Vec2 arrowTipPointLeft = segmentRenderEnd - direction * arrowTipLength + perpendicular * arrowTipWidth;
  const Vec2 arrowTipPointRight = segmentRenderEnd - direction * arrowTipLength - perpendicular * arrowTipWidth;
  sandbox_line(segmentRenderStart, segmentRenderEnd, color);
  sandbox_line(segmentRenderEnd, arrowTipPointLeft, color);
  sandbox_line(segmentRenderEnd, arrowTipPointRight, color);
}

void drawSegmentList(const std::vector<TSegment>& segments, const Color& color)
{
  for(const TSegment& segment : segments)
  {
    drawSegment(segment, color);
  }
}

void drawBorder(const PolygonBorder& border, const Color& color)
{
  for(int i = 0; i < static_cast<int>(border.size()) - 1; i++)
  {
    sandbox_line(border[i], border[i + 1], color);
  }
}

void drawOutputBorders(const std::vector<PolygonBorder>& borders)
{
  for(const PolygonBorder& border : borders)
  {
    drawBorder(border, Red);
  }
}

std::vector<TSegment> fillSegments(const Grid& input)
{
  std::vector<TSegment> segments;
  for(int y = -1; y < gridHeight; y++)
  {
    for(int x = -1; x < gridWidth; x++)
    {
      drawTestedCellBorders(x, y, Green);
      drawSegmentList(segments, Yellow);
      sandbox_breakpoint();

      std::vector<TSegment> newSegments;
      const Coord topLeft = {x, y + 1};
      const Coord topRight = topLeft + Coord({1, 0});
      const Coord bottomRight = topRight + Coord({0, -1});
      if(!tileIsFilled(input, {x, y}))
      {
        if(tileIsFilled(input, {x + 1, y}))
          newSegments.push_back({topRight, bottomRight});
        if(tileIsFilled(input, {x, y + 1}))
          newSegments.push_back({topLeft, topRight});
      }
      else
      {
        if(!tileIsFilled(input, {x + 1, y}))
          newSegments.push_back({bottomRight, topRight});
        if(!tileIsFilled(input, {x, y + 1}))
          newSegments.push_back({topRight, topLeft});
      }

      if(!newSegments.empty())
      {
        drawTestedCellBorders(x, y, Green);
        drawSegmentList(segments, Yellow);
        drawSegmentList(newSegments, Green);
        sandbox_breakpoint();
        segments.insert(segments.end(), newSegments.begin(), newSegments.end());
      }
    }
  }

  return segments;
}

Vec2 segmentDirection(const TSegment& segment)
{
  const Coord direction = segment.b - segment.a;
  return Vec2(direction.x, direction.y);
}

auto getNextSegmentIterator(std::vector<TSegment>& segments, const TSegment& startSegment)
{
  const Coord segmentTip = startSegment.b;
  const Coord segmentDir = startSegment.a - startSegment.b;

  const Coord directionsToTry[3] = {
        rotateLeft(segmentDir),
        rotateLeft(rotateLeft(segmentDir)),
        rotateLeft(rotateLeft(rotateLeft(segmentDir))),
  };

  for(const Coord& direction : directionsToTry)
  {
    const TSegment segmentToTry = {segmentTip, segmentTip + direction};
    auto it = std::find(segments.begin(), segments.end(), segmentToTry);
    if(it != segments.end())
      return it;
  }

  return segments.end();
}

float sqr(float x) { return x * x; }

Grid generateInput(int /*seed*/)
{
  Grid grid{};

  for(int k = 0; k < tileCount / 5; ++k)
  {
    const bool gridValue = randomFloat(0, 1) > 0.5f;

    const int rectWidth = sqr(randomFloat(0.1, 0.5)) * gridWidth;
    const int rectHeight = sqr(randomFloat(0.1, 0.5)) * gridHeight;

    Coord pos;
    pos.x = randomFloat(0, 1) * (gridWidth - rectWidth);
    pos.y = randomFloat(0, 1) * (gridHeight - rectHeight);

    for(int y = 0; y < rectHeight; ++y)
    {
      for(int x = 0; x < rectWidth; ++x)
      {
        const int absX = clamp(x + pos.x, 0, gridWidth - 1);
        const int absY = clamp(y + pos.y, 0, gridHeight - 1);
        grid[absX + absY * gridWidth] = gridValue;
      }
    }
  }

  return grid;
}

std::vector<PolygonBorder> execute(Grid input)
{
  std::vector<TSegment> segments = fillSegments(input);

  std::vector<PolygonBorder> output;
  while(!segments.empty())
  {
    PolygonBorder newBorder;
    auto segmentIt = segments.begin();
    do
    {
      const TSegment currentSegment = *segmentIt;
      const Vec2 direction = segmentDirection(currentSegment);
      newBorder.push_back(tileRenderPosition(currentSegment.a) + direction * borderCornerOffset);
      newBorder.push_back(tileRenderPosition(currentSegment.b) - direction * borderCornerOffset);
      *segmentIt = segments.back();
      segments.pop_back();
      drawSegmentList(segments, Yellow);
      drawBorder(newBorder, Green);
      drawOutputBorders(output);
      sandbox_breakpoint();
      segmentIt = getNextSegmentIterator(segments, currentSegment);
    } while(segmentIt != segments.end());

    newBorder.push_back(newBorder.front());
    output.push_back(newBorder);
  }

  return output;
}

void display(Grid input, const std::vector<PolygonBorder>& output)
{
  for(int y = 0; y < gridHeight; y++)
  {
    for(int x = 0; x < gridWidth; x++)
    {
      if(input[tileIndex({x, y})])
        drawFilledTile(x, y, LightBlue);
    }
  }
  drawGridLines();
  drawOutputBorders(output);
}

BEGIN_ALGO("ContourTracing", execute)
WITH_INPUTGEN(generateInput)
WITH_DISPLAY(display)
END_ALGO
} // namespace
