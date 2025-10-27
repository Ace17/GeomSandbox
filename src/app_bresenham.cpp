// Copyright (C) 2023 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Bresenham's algorithm for line drawing

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <algorithm>
#include <vector>

#include "random.h"

namespace
{
struct Segment
{
  Vec2 start;
  Vec2 end;
};

struct Tile
{
  int x;
  int y;
};

using Output = std::vector<Tile>;

constexpr int gridWidth = 20;
constexpr int gridHeight = 15;
constexpr float tileRenderSize = 2.f;

float lerp(float a, float b, float r) { return (a * (1.f - r)) + (b * r); }

Vec2 tileRenderPosition(Vec2 pos)
{
  const float x = (pos.x - gridWidth / 2.f + 0.5f) * tileRenderSize;
  const float y = (pos.y - gridHeight / 2.f + 0.5f) * tileRenderSize;
  return {x, y};
}

void drawFilledTile(Tile tile, const Color& color)
{
  const Vec2 position = tileRenderPosition(Vec2(tile.x, tile.y));
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

void drawOutput(const Output& output)
{
  for(auto tile : output)
  {
    drawFilledTile(tile, Green);
  }
}

// abs(dir.x) > abs(dir.y)
Output lineLowSlope(Segment input)
{
  Output output;
  const Vec2 inputDir = input.end - input.start;
  Vec2 dir = inputDir;
  int y_step = 1;
  if(dir.y < 0)
  {
    y_step = -1;
    dir.y = -dir.y;
  }
  float delta = (2 * dir.y) - dir.x;
  int y = input.start.y;

  for(int x = input.start.x; x <= input.end.x; x++)
  {
    output.push_back({x, y});
    if(delta > 0)
    {
      y += y_step;
      delta += 2 * (dir.y - dir.x);
    }
    else
    {
      delta += 2 * dir.y;
    }

    const float lineX = x + 0.5f;
    const float lineY = input.start.y + inputDir.y * ((lineX - input.start.x) / inputDir.x);
    sandbox_circle(tileRenderPosition(Vec2(lineX, lineY)), 0.2f, Red);
    drawOutput(output);
    sandbox_breakpoint();
  }
  return output;
}

// abs(dir.x) <= abs(dir.y)
Output lineHighSlope(Segment input)
{
  Output output;
  const Vec2 inputDir = input.end - input.start;
  Vec2 dir = inputDir;
  int x_step = 1;
  if(dir.x < 0)
  {
    x_step = -1;
    dir.x = -dir.x;
  }
  float delta = (2 * dir.x) - dir.y;
  int x = input.start.x;

  for(int y = input.start.y; y <= input.end.y; y++)
  {
    output.push_back({x, y});
    if(delta > 0)
    {
      x += x_step;
      delta += 2 * (dir.x - dir.y);
    }
    else
    {
      delta += 2 * dir.x;
    }

    const float lineY = y + 0.5f;
    const float lineX = input.start.x + inputDir.x * ((lineY - input.start.y) / inputDir.y);
    sandbox_circle(tileRenderPosition(Vec2(lineX, lineY)), 0.2f, Red);
    drawOutput(output);
    sandbox_breakpoint();
  }
  return output;
}

Segment generateInput(int /*seed*/)
{
  const Vec2 start = Vec2(randomInt(0, gridWidth) + 0.5f, randomInt(0, gridHeight) + 0.5f);
  const Vec2 end = Vec2(randomInt(0, gridWidth) + 0.5f, randomInt(0, gridHeight) + 0.5f);
  return {start, end};
}

Output execute(Segment input)
{
  const Vec2 dir = input.end - input.start;
  if(std::abs(dir.y) < std::abs(dir.x))
  {
    sandbox_printf("low slope (x < y)\n");
    if(input.start.x < input.end.x)
      return lineLowSlope({input.start, input.end});
    else
      return lineLowSlope({input.end, input.start});
  }
  sandbox_printf("high slope (x >= y)\n");
  if(input.start.y < input.end.y)
    return lineHighSlope({input.start, input.end});
  else
    return lineHighSlope({input.end, input.start});
}

void display(const Segment& input, const Output& output)
{
  for(int x = 0; x <= gridWidth; x++)
  {
    const float posX = static_cast<float>(x);
    const Vec2 start = tileRenderPosition({posX, 0});
    const Vec2 end = tileRenderPosition({posX, gridHeight});
    sandbox_line(start, end, Gray);
  }
  for(int y = 0; y <= gridHeight; y++)
  {
    const float posY = static_cast<float>(y);
    const Vec2 start = tileRenderPosition({0, posY});
    const Vec2 end = tileRenderPosition({gridWidth, posY});
    sandbox_line(start, end, Gray);
  }

  const Vec2 dir = normalize(input.end - input.start);
  sandbox_line(tileRenderPosition(input.start), tileRenderPosition(input.end), White);
  sandbox_text(tileRenderPosition(input.start - dir * 0.5), "Start", Yellow);
  sandbox_text(tileRenderPosition(input.end + dir * 0.5), "End", Yellow);

  drawOutput(output);
}

BEGIN_ALGO("DrawLine/Bresenham", execute)
WITH_INPUTGEN(generateInput)
WITH_DISPLAY(display)
END_ALGO
} // namespace
