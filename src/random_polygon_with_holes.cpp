// Copyright (C) 2026 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////

#include "random_polygon_with_holes.h"

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

}

PolygonWithHoles createRandomPolygonWithHoles()
{
  InputGrid grid;
  grid.size.x = int(randomFloat(0.5, 1) * 20);
  grid.size.y = int(randomFloat(0.5, 1) * 20);
  grid.data.resize(grid.size.x * grid.size.y);

  for(auto& val : grid.data)
    val = randomFloat(0, 1) < 0.6 ? 1 : 0;

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
      }
      if(A && B && C && !D)
      {
        // inner boundary (=hole)
        entryPoints.insert(Vec2i{x, y});
      }
    }
  }

  PolygonWithHoles r;

  while(entryPoints.size())
  {
    r.boundaries.push_back({});
    auto& contour = r.boundaries.back();

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

      if(currDir == 0) // an entry point is always upwards
        entryPoints.erase(pos);

      // move forward, the interior is on our left
      pos = pos + dirs[currDir];
    } while(!(pos == ep));
  }

  return r;
}
