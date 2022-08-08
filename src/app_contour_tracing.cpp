// Copyright (C) 2022 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Contour-tracing algorithm.

#include "core/algorithm_app.h"
#include "core/sandbox.h"

#include <array>
#include <cmath>

#include "random.h"

namespace
{

constexpr int gridWidth = 10;
constexpr int gridHeight = 10;
constexpr int tileCount = gridWidth * gridHeight;

constexpr float tileRenderSize = 2.5;

using Grid = std::array<bool, tileCount>;

float lerp(float a, float b, float r) { return (a * (1.f - r)) + (b * r); }

Vec2 tileRenderPosition(int x, int y)
{
  const float renderX = x * tileRenderSize - gridWidth * tileRenderSize / 2.f;
  const float renderY = y * tileRenderSize - gridHeight * tileRenderSize / 2.f;
  return {renderX, renderY};
}

void drawGridLines()
{
  for(int y = 0; y < gridHeight + 1; y++)
  {
    sandbox_line(tileRenderPosition(0, y), tileRenderPosition(gridWidth, y), White);
  }
  for(int x = 0; x < gridWidth + 1; x++)
  {
    sandbox_line(tileRenderPosition(x, 0), tileRenderPosition(x, gridHeight), White);
  }
}

void drawFilledTile(const Vec2& position, const Vec2& size, const Color& color)
{
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

struct ContourTracingAlgorithm
{
  static Grid generateInput()
  {
    Grid grid;
    grid.fill(false);
    for(int tileIndex = 0; tileIndex < tileCount; tileIndex++)
      grid[tileIndex] = randomFloat(0, 1) > 0.5f;

    return grid;
  }

  static int execute(Grid input)
  {
    // TODO
    (void)input;
    return 0;
  }

  static void display(Grid input, int output)
  {
    for(int y = 0; y < gridHeight; y++)
    {
      for(int x = 0; x < gridWidth; x++)
      {
        const int tileIndex = y * gridWidth + x;
        if(input[tileIndex])
        {
          const Vec2 rectStart = tileRenderPosition(x, y);
          const Vec2 rectSize = Vec2(tileRenderSize, tileRenderSize);
          drawFilledTile(rectStart, rectSize, LightBlue);
        }
      }
    }
    drawGridLines();

    // TODO
    (void)output;
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<ContourTracingAlgorithm>>()); }
const int reg = registerApp("ContourTracing", &create);
} // namespace
