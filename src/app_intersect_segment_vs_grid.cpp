// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// intersection between segment and grid

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "random.h"

namespace
{
struct Segment
{
  Vec2 A;
  Vec2 B;
};

struct Crossing
{
  float ratio;
  int type; // 0: crosses a vertical line, 1: crosses an horizontal line
};

using Output = std::vector<Crossing>;

const int COLS = 7;
const int ROWS = 5;
const Vec2 CELL_SIZE{4.0f, 6.0f};

float lerp(float a, float b, float alpha) { return a * (1.f - alpha) + b * alpha; }
Vec2 lerp(Vec2 a, Vec2 b, float alpha) { return a * (1.f - alpha) + b * alpha; }

Segment generateInput(int /*seed*/)
{
  randomFloat(0, 1);

  Vec2 A;
  A.x = randomFloat(0, COLS * CELL_SIZE.x);
  A.y = randomFloat(0, ROWS * CELL_SIZE.y);

  Vec2 B;
  B.x = randomFloat(0, COLS * CELL_SIZE.x);
  B.y = randomFloat(0, ROWS * CELL_SIZE.y);

  return {A, B};
}

Output execute(Segment input)
{
  const Vec2 delta = input.B - input.A;

  Output r;

  // compute crossed vertical lines
  if(fabs(delta.x) != 0)
  {
    const float xMin = std::min(input.A.x, input.B.x);
    const float xMax = std::max(input.A.x, input.B.x);

    float x = (floor(xMin / CELL_SIZE.x) + 1) * CELL_SIZE.x;

    while(x <= xMax)
    {
      r.push_back({(x - input.A.x) / (input.B.x - input.A.x), 0});
      x += CELL_SIZE.x;
    }
  }

  // compute crossed horizontal lines
  if(fabs(delta.y) != 0)
  {
    const float yMin = std::min(input.A.y, input.B.y);
    const float yMax = std::max(input.A.y, input.B.y);

    float y = (floor(yMin / CELL_SIZE.y) + 1) * CELL_SIZE.y;

    while(y <= yMax)
    {
      r.push_back({(y - input.A.y) / (input.B.y - input.A.y), 1});
      y += CELL_SIZE.y;
    }
  }

  return r;
}

void display(const Segment& input, const Output& output)
{
  for(int x = 0; x <= COLS; x++)
  {
    const float posX = x * CELL_SIZE.x;
    const Vec2 A = {posX, 0};
    const Vec2 B = {posX, ROWS * CELL_SIZE.y};
    sandbox_line(A, B, Gray);
  }

  for(int y = 0; y <= ROWS; y++)
  {
    const float posY = y * CELL_SIZE.y;
    const Vec2 A = {0, posY};
    const Vec2 B = {COLS * CELL_SIZE.x, posY};
    sandbox_line(A, B, Gray);
  }

  sandbox_line(input.A, input.B, Yellow);
  sandbox_text(input.A, "A", Yellow, {8, 8});
  sandbox_text(input.B, "B", Yellow, {8, 8});

  for(auto& v : output)
  {
    const Vec2 point = lerp(input.A, input.B, v.ratio);
    sandbox_line(point, point, Red, {-8, -8}, {+8, +8});
    sandbox_line(point, point, Red, {-8, +8}, {+8, -8});

    if(v.type == 0)
    {
      const float x = lerp(input.A.x, input.B.x, v.ratio);
      const Vec2 A = {x, 0};
      const Vec2 B = {x, ROWS * CELL_SIZE.y};
      sandbox_line(A, B, Orange);

      int row = floor(point.y / CELL_SIZE.y);
      int colA = floor(point.x / CELL_SIZE.x - 0.5f);
      int colB = floor(point.x / CELL_SIZE.x + 0.5f);

      Vec2 posA = {(colA + 0.75f) * CELL_SIZE.x, (row + 0.5f) * CELL_SIZE.y};
      Vec2 posB = {(colB + 0.25f) * CELL_SIZE.x, (row + 0.5f) * CELL_SIZE.y};
      sandbox_line(posA, posB, Green);
    }
    else
    {
      const float y = lerp(input.A.y, input.B.y, v.ratio);
      const Vec2 A = {0, y};
      const Vec2 B = {COLS * CELL_SIZE.x, y};
      sandbox_line(A, B, Orange);

      int col = floor(point.x / CELL_SIZE.x);
      int rowA = floor(point.y / CELL_SIZE.y - 0.5f);
      int rowB = floor(point.y / CELL_SIZE.y + 0.5f);

      Vec2 posA = {(col + 0.5f) * CELL_SIZE.x, (rowA + 0.75f) * CELL_SIZE.y};
      Vec2 posB = {(col + 0.5f) * CELL_SIZE.x, (rowB + 0.25f) * CELL_SIZE.y};
      sandbox_line(posA, posB, Green);
    }
  }
}

BEGIN_ALGO("Intersection/SegmentVsGrid", execute)
WITH_INPUTGEN(generateInput)
WITH_DISPLAY(display)
END_ALGO
} // namespace
