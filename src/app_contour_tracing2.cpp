// Copyright (C) 2026 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <unordered_set>

#include "random.h"

namespace
{
struct Vec2i
{
  int x, y;
  bool operator==(Vec2i other) const { return x == other.x && y == other.y; }
  Vec2i operator+(Vec2i other) const { return {x + other.x, y + other.y}; }
};

Vec2 toVec2(Vec2i a) { return {(float)a.x, (float)a.y}; }

struct Vec2iHash
{
  std::size_t operator()(Vec2i val) const { return val.x + val.y * 100; }
};

struct InputGrid
{
  Vec2i size;
  std::vector<char> data;
  char get(Vec2i pos) const
  {
    if(pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y)
      return 0;
    return data[pos.x + pos.y * size.x];
  }
};

struct Output
{
  std::vector<std::vector<Vec2>> contours;
};

InputGrid generateInput(int)
{
  InputGrid r;
  r.size.x = int(randomFloat(0.5, 1) * 20);
  r.size.y = int(randomFloat(0.5, 1) * 20);
  r.data.resize(r.size.x * r.size.y);

  for(auto& val : r.data)
    val = randomFloat(0, 1) < 0.6 ? 1 : 0;

  return r;
}

Output computeContours(InputGrid grid)
{
  std::unordered_set<Vec2i, Vec2iHash> entryPoints;
  for(int y = 0; y < grid.size.y; y++)
  {
    for(int x = 0; x < grid.size.x + 1; x++)
    {
      // [x-1,y]    |   [x,y]
      //    A       |     D
      // ---------(x,y) ------
      //    B       |     C
      // [x-1,y-1]  |   [x,y-1]
      const Vec2i posA = {x - 1, y};
      const Vec2i posB = {x - 1, y - 1};
      const Vec2i posC = {x, y - 1};
      const Vec2i posD = {x, y};

      const char A = grid.get(posA);
      const char B = grid.get(posB);
      const char C = grid.get(posC);
      const char D = grid.get(posD);

      if(!B && !D && A)
      {
        // outer boundary
        entryPoints.insert(Vec2i{x, y});
        sandbox_circle({(float)x, (float)y}, 0, Red, 8);
      }
      if(A && B && C && !D)
      {
        // inner boundary (=hole)
        entryPoints.insert(Vec2i{x, y});
        sandbox_circle({(float)x, (float)y}, 0, Red, 8);
      }
    }
  }

  sandbox_breakpoint();

  Output r;

  while(entryPoints.size())
  {
    r.contours.push_back({});
    auto& contour = r.contours.back();

    const Vec2i ep = *entryPoints.begin();

    static const Vec2i dirs[] = {{0, 1}, {-1, 0}, {0, -1}, {1, 0}};
    static const Vec2i leftScanPos[] = {{-1, 0}, {-1, -1}, {0, -1}, {0, 0}};
    static const Vec2i rightScanPos[] = {{0, 0}, {-1, 0}, {-1, -1}, {0, -1}};
    int currDir = 0;

    Vec2i pos = ep;
    contour.push_back(toVec2(pos));
    do
    {
      if(!grid.get(pos + leftScanPos[currDir]))
      {
        contour.push_back(toVec2(pos) - toVec2(dirs[currDir]) * 0.1);
        currDir = (currDir + 1) % 4;
        contour.push_back(toVec2(pos) + toVec2(dirs[currDir]) * 0.1);
      }
      else if(grid.get(pos + rightScanPos[currDir]))
      {
        contour.push_back(toVec2(pos));
        currDir = (currDir + 4 - 1) % 4;
      }

      // move up, the interior is on our left
      const Vec2 a = toVec2(pos);
      const Vec2 b = toVec2(pos + dirs[currDir]);

      sandbox_line(a, b, Green);
      sandbox_circle(a, 0, Green, 4);

      if(currDir == 0) // an entry point is always upwards
        entryPoints.erase(pos);

      pos = pos + dirs[currDir];
    } while(!(pos == ep));

    sandbox_breakpoint();
  }

  return r;
}

void display(const InputGrid& grid, const Output& out)
{
  for(int y = 0; y < grid.size.y; y++)
  {
    for(int x = 0; x < grid.size.x; x++)
    {
      Vec2 pos = {(float)x, (float)y};
      sandbox_rect(pos + Vec2{0.5, 0.5}, Vec2{1, 1}, Gray, {-4, -4});
      if(grid.get({x, y}))
      {
        sandbox_rect(pos + Vec2{0.5, 0.5}, Vec2{1, 1}, White, {-8, -8});
        sandbox_line(pos, pos + Vec2{1, 1}, White);
        sandbox_line(pos + Vec2{1, 0}, pos + Vec2{0, 1}, White);
      }
    }
  }

  for(auto& c : out.contours)
  {
    const int N = c.size();
    for(int i = 0; i < N; ++i)
    {
      const int j = (i + 1) % N;
      sandbox_line(c[i], c[j], Green);
    }
  }
}

BEGIN_ALGO("ContourTracing2", computeContours)
WITH_INPUTGEN(generateInput)
WITH_DISPLAY(display)
END_ALGO
} // namespace
