// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Visibility computation using a cells-and-portals decomposition

#include "core/app.h"
#include "core/drawer.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
struct AABB
{
  Vec2 mins;
  Vec2 maxs;
};

struct Segment
{
  Vec2 a, b;
};

struct Portal
{
  int segment;
  int destCell;
};

struct Cell
{
  std::vector<Portal> portals;
  std::vector<int> walls;
  AABB detector;
};

struct World
{
  std::vector<Segment> segments;
  std::vector<Cell> cells;
};

World createWorld()
{
  World world;

  world.segments.resize(32 + 4 * 2);

  // bottom wall
  world.segments[0] = {{0, 0}, {3, 0}};
  world.segments[1] = {{3, 0}, {3, 1}};
  world.segments[2] = {{3, 1}, {4, 1}};
  world.segments[3] = {{4, 1}, {4, 0}};
  world.segments[4] = {{4, 0}, {7, 0}};

  // top wall
  world.segments[5] = {{0, 7}, {3, 7}};
  world.segments[6] = {{3, 7}, {3, 6}};
  world.segments[7] = {{3, 6}, {4, 6}};
  world.segments[8] = {{4, 6}, {4, 7}};
  world.segments[9] = {{4, 7}, {7, 7}};

  // left wall
  world.segments[10] = {{0, 0}, {0, 3}};
  world.segments[11] = {{0, 3}, {1, 3}};
  world.segments[12] = {{1, 3}, {1, 4}};
  world.segments[13] = {{1, 4}, {0, 4}};
  world.segments[14] = {{0, 4}, {0, 7}};

  // right wall
  world.segments[15] = {{7, 0}, {7, 3}};
  world.segments[16] = {{7, 3}, {6, 3}};
  world.segments[17] = {{6, 3}, {6, 4}};
  world.segments[18] = {{6, 4}, {7, 4}};
  world.segments[19] = {{7, 4}, {7, 7}};

  // cross
  world.segments[20] = {{2.5, 3}, {3, 3}};
  world.segments[21] = {{3, 3}, {3, 2.5}};
  world.segments[22] = {{3, 2.5}, {4, 2.5}};
  world.segments[23] = {{4, 2.5}, {4, 3}};
  world.segments[24] = {{4, 3}, {4.5, 3}};
  world.segments[25] = {{4.5, 3}, {4.5, 4}};
  world.segments[26] = {{4.5, 4}, {4, 4}};
  world.segments[27] = {{4, 4}, {4, 4.5}};
  world.segments[28] = {{4, 4.5}, {3, 4.5}};
  world.segments[29] = {{3, 4.5}, {3, 4}};
  world.segments[30] = {{3, 4}, {2.5, 4}};
  world.segments[31] = {{2.5, 4}, {2.5, 3}};

  world.segments[32] = {{4, 1}, {4, 2.5}};
  world.segments[33] = {{1, 4}, {2.5, 4}};
  world.segments[34] = {{3, 4.5}, {3, 6}};
  world.segments[35] = {{4.5, 3}, {6, 3}};
  world.segments[36] = {{4, 2.5}, {4, 1}};
  world.segments[37] = {{2.5, 4}, {1, 4}};
  world.segments[38] = {{3, 6}, {3, 4.5}};
  world.segments[39] = {{6, 3}, {4.5, 3}};

  world.cells.resize(4);
  world.cells[0].walls = {0, 10, 1, 2, 11, 12, 31, 20, 21, 22};
  world.cells[0].portals = {{32, 1}, {37, 3}};
  world.cells[0].detector = {{0, 0}, {4, 4}};

  world.cells[1].walls = {3, 4, 15, 16, 23, 24};
  world.cells[1].portals = {{36, 0}, {39, 2}};
  world.cells[1].detector = {{4, 0}, {7, 3}};

  world.cells[2].walls = {17, 18, 19, 9, 8, 7, 25, 26, 27, 28};
  world.cells[2].portals = {{35, 1}, {38, 3}};
  world.cells[2].detector = {{3, 3}, {7, 7}};

  world.cells[3].walls = {13, 30, 29, 6, 5, 14};
  world.cells[3].portals = {{33, 0}, {34, 2}};
  world.cells[3].detector = {{0, 4}, {3, 7}};

  for(auto& s : world.segments)
  {
    s.a = (s.a - Vec2(3, 3)) * 3.0;
    s.b = (s.b - Vec2(3, 3)) * 3.0;
  }

  for(auto& c : world.cells)
  {
    c.detector.mins = (c.detector.mins - Vec2(3, 3)) * 3.0;
    c.detector.maxs = (c.detector.maxs - Vec2(3, 3)) * 3.0;
  }

  return world;
}

struct Plane
{
  Vec2 normal;
  float dist;
};

struct Frustum
{
  Plane a, b; // plane normals points towards the inside
};

bool isOnPositiveSide(const Plane& plane, Vec2 point) { return dot_product(point, plane.normal) > plane.dist; }

bool intersects(const Frustum& frustum, const Segment& s)
{
  if(!isOnPositiveSide(frustum.a, s.a) && !isOnPositiveSide(frustum.a, s.b))
    return false; // the whole segment is clipped by the 'a' plane

  if(!isOnPositiveSide(frustum.b, s.a) && !isOnPositiveSide(frustum.b, s.b))
    return false; // the whole segment is clipped by the 'b' plane

  return true;
}

bool inside(const AABB& aabb, Vec2 pos)
{
  if(pos.x < aabb.mins.x || pos.x > aabb.maxs.x)
    return false;
  if(pos.y < aabb.mins.y || pos.y > aabb.maxs.y)
    return false;
  return true;
}

Frustum computeFrustum(Vec2 origin, Segment backPlane)
{
  Frustum r;
  r.a.normal = normalize(rotateLeft(backPlane.a - origin));
  r.b.normal = normalize(rotateLeft(backPlane.b - origin));
  r.a.dist = dot_product(r.a.normal, origin);
  r.b.dist = dot_product(r.b.normal, origin);
  if(!isOnPositiveSide(r.a, backPlane.b))
  {
    r.a.normal = r.a.normal * -1;
    r.a.dist = r.a.dist * -1;
  }
  if(!isOnPositiveSide(r.b, backPlane.a))
  {
    r.b.normal = r.b.normal * -1;
    r.b.dist = r.b.dist * -1;
  }
  return r;
}

void append(std::vector<int>& dst, const std::vector<int>& src)
{
  for(auto val : src)
    dst.push_back(val);
}

std::vector<Frustum> frustums;

std::vector<int> computeListOfVisibleCellsAux(const World& world, int fromCell, Vec2 pos, int depth, Frustum frustum)
{
  std::vector<int> r;
  r.push_back(fromCell);

  if(depth >= 5)
    return r;

  for(auto& portal : world.cells[fromCell].portals)
  {
    auto portalSegment = world.segments[portal.segment];
    if(portal.destCell == fromCell)
      continue;

    // backface culling of portals
    {
      const auto segmentNormal = rotateLeft(portalSegment.b - portalSegment.a);
      if(dot_product(segmentNormal, pos - portalSegment.a) < 0)
        continue;
    }

    if(frustum.a.normal == Vec2{} || intersects(frustum, portalSegment))
    {
      auto subFrustum = computeFrustum(pos, portalSegment);
      frustums.push_back(subFrustum);
      append(r, computeListOfVisibleCellsAux(world, portal.destCell, pos, depth + 1, subFrustum));
    }
  }

  return r;
}

std::vector<int> computeListOfVisibleCells(const World& world, int fromCell, Vec2 pos)
{
  return computeListOfVisibleCellsAux(world, fromCell, pos, 0, {});
}

struct Collide2DApp : IApp
{
  Collide2DApp()
  {
    world = createWorld();

    pos = Vec2(-3, -4);
  }

  void tick() override
  {
    const bool left = keyState[(int)Key::Left];
    const bool right = keyState[(int)Key::Right];
    const bool up = keyState[(int)Key::Up];
    const bool down = keyState[(int)Key::Down];

    const auto speed = 0.1;

    if(left)
      pos.x -= speed;
    if(right)
      pos.x += speed;
    if(up)
      pos.y += speed;
    if(down)
      pos.y -= speed;
  }

  void draw(IDrawer* drawer) override
  {
    for(auto& cell : world.cells)
    {
      if(inside(cell.detector, pos))
        currCell = int(&cell - world.cells.data());
    }

    static const Color colors[] = {
          LightBlue,
          Green,
          Blue,
          Yellow,
    };
    static const auto colorCount = sizeof(colors) / sizeof(*colors);

    frustums.clear();
    auto visibleCells = computeListOfVisibleCells(world, currCell, pos);
    std::vector<char> visible(world.cells.size());
    for(auto& cellIdx : visibleCells)
      visible[cellIdx] = true;

    for(auto& frustum : frustums)
      drawFrustum(drawer, frustum);

    for(auto& cell : world.cells)
    {
      int cellIdx = int(&cell - world.cells.data());
      if(!visible[cellIdx])
        continue;

      for(auto& idx : cell.walls)
      {
        auto& segment = world.segments[idx];
        drawer->line(segment.a, segment.b, colors[cellIdx % colorCount]);
      }

      for(auto& portal : cell.portals)
      {
        auto& segment = world.segments[portal.segment];
        drawer->line(segment.a, segment.b, Red);

        auto middle = (segment.a + segment.b) * 0.5;
        auto normal = normalize(rotateLeft(segment.b - segment.a));
        drawer->line(middle, middle + normal * 0.2, Red);
      }

      if(0)
        drawer->rect(cell.detector.mins, cell.detector.maxs - cell.detector.mins, Green);
    }

    drawer->circle(pos, 0.1, Green);

    char buf[256];
    sprintf(buf, "Current cell: %d", currCell);
    drawer->text({-7, -10}, buf);
  }

  static void drawFrustum(IDrawer* drawer, const Frustum& frustum)
  {
    drawPlane(drawer, frustum.a);
    drawPlane(drawer, frustum.b);
  }

  static void drawPlane(IDrawer* drawer, const Plane& plane)
  {
    auto p = plane.normal * plane.dist;
    auto tangent = rotateLeft(plane.normal);
    drawer->line(p, p + tangent * 100);
    drawer->line(p, p - tangent * 100);
    drawer->line(p, p + plane.normal);
  }

  void keydown(Key key) override { keyState[(int)key] = true; }
  void keyup(Key key) override { keyState[(int)key] = false; }

  bool keyState[128]{};
  World world{};
  Vec2 pos;
  int currCell = 0;
};

const int registered = registerApp("App.Portals2D", []() -> IApp* { return new Collide2DApp; });
}
