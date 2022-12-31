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

  Vec2 P[44];
  P[0] = {4, 0};
  P[1] = {14, 0};
  P[2] = {0, 2};
  P[3] = {4, 2};
  P[4] = {6, 2};
  P[5] = {8, 2};
  P[6] = {10, 2};
  P[7] = {12, 2};
  P[8] = {14, 2};
  P[9] = {7, 3};
  P[10] = {8, 3};
  P[11] = {10, 3};
  P[12] = {11, 3};
  P[13] = {0, 5};
  P[14] = {2, 5};
  P[15] = {4, 5};
  P[16] = {6, 5};
  P[17] = {0, 6};
  P[18] = {2, 6};
  P[19] = {4, 6};
  P[20] = {6, 6};
  P[21] = {7, 6};
  P[22] = {11, 6};
  P[23] = {12, 6};
  P[24] = {0, 10};
  P[25] = {2, 10};
  P[26] = {4, 10};
  P[27] = {6, 10};
  P[28] = {0, 11};
  P[29] = {2, 11};
  P[30] = {4, 11};
  P[31] = {5, 11};
  P[32] = {5, 12};
  P[33] = {6, 12};
  P[34] = {11, 12};
  P[35] = {12, 12};
  P[36] = {5, 14};
  P[37] = {6, 14};
  P[38] = {0, 16};
  P[39] = {5, 16};
  P[40] = {6, 16};
  P[41] = {11, 16};
  P[42] = {12, 16};
  P[43] = {14, 16};

  world.segments.resize(60);

  world.segments[0] = {P[2], P[3]};
  world.segments[1] = {P[3], P[0]};
  world.segments[2] = {P[0], P[1]};
  world.segments[3] = {P[1], P[8]};
  world.segments[4] = {P[7], P[8]};
  world.segments[5] = {P[7], P[6]};
  world.segments[6] = {P[5], P[6]};
  world.segments[7] = {P[4], P[5]};
  world.segments[8] = {P[3], P[4]};
  world.segments[9] = {P[2], P[13]};
  world.segments[10] = {P[13], P[14]};
  world.segments[11] = {P[14], P[18]};
  world.segments[12] = {P[18], P[19]};
  world.segments[13] = {P[19], P[15]};
  world.segments[14] = {P[15], P[16]};
  world.segments[15] = {P[16], P[4]};
  world.segments[16] = {P[5], P[10]};
  world.segments[17] = {P[6], P[11]};
  world.segments[18] = {P[7], P[23]};
  world.segments[19] = {P[9], P[10]};
  world.segments[20] = {P[11], P[12]};
  world.segments[21] = {P[9], P[21]};
  world.segments[22] = {P[17], P[18]};
  world.segments[23] = {P[19], P[20]};
  world.segments[24] = {P[20], P[21]};
  world.segments[25] = {P[22], P[23]};
  world.segments[26] = {P[17], P[24]};
  world.segments[27] = {P[20], P[27]};
  world.segments[28] = {P[23], P[35]};
  world.segments[29] = {P[24], P[25]};
  world.segments[30] = {P[25], P[26]};
  world.segments[31] = {P[26], P[27]};
  world.segments[32] = {P[25], P[29]};
  world.segments[33] = {P[26], P[30]};
  world.segments[34] = {P[27], P[33]};
  world.segments[35] = {P[28], P[29]};
  world.segments[36] = {P[30], P[31]};
  world.segments[37] = {P[31], P[32]};
  world.segments[38] = {P[28], P[38]};
  world.segments[39] = {P[32], P[33]};
  world.segments[40] = {P[34], P[35]};
  world.segments[41] = {P[36], P[39]};
  world.segments[42] = {P[36], P[37]};
  world.segments[43] = {P[34], P[41]};
  world.segments[44] = {P[35], P[42]};
  world.segments[45] = {P[37], P[40]};
  world.segments[46] = {P[38], P[39]};
  world.segments[47] = {P[40], P[41]};
  world.segments[48] = {P[42], P[43]};
  world.segments[49] = {P[8], P[43]};
  world.segments[50] = {P[22], P[12]};
  world.segments[51] = {P[33], P[37]};
  world.segments[52] = {P[37], P[33]};
  world.segments[53] = {P[26], P[25]};
  world.segments[54] = {P[19], P[18]};

  world.segments[55] = {P[8], P[7]};
  world.segments[56] = {P[6], P[5]};
  world.segments[57] = {P[4], P[3]};

  world.segments[58] = {P[27], P[20]};
  world.segments[59] = {P[35], P[23]};

  world.cells.resize(6);
  world.cells[0].walls = {46, 38, 35, 32, 33, 36, 37, 39, 42, 41};
  world.cells[0].portals = {{30, 1}, {51, 3}};
  world.cells[0].detector = {{0, 10}, {6, 16}};

  world.cells[1].walls = {29, 31, 26, 22, 23};
  world.cells[1].portals = {{12, 2}, {27, 3}, {53, 0}};
  world.cells[1].detector = {{0, 6}, {6, 10}};

  world.cells[2].walls = {11, 10, 9, 0, 13, 14, 15};
  world.cells[2].portals = {{8, 5}, {54, 1}};
  world.cells[2].detector = {{0, 2}, {6, 6}};

  world.cells[3].walls = {45, 47, 43, 40, 25, 20, 17, 16, 19, 21, 24, 50, 34};
  world.cells[3].portals = {{52, 0}, {28, 4}, {6, 5}, {58, 1}};
  world.cells[3].detector = {{6, 2}, {12, 16}};

  world.cells[4].walls = {44, 48, 49, 18};
  world.cells[4].portals = {{4, 5}, {59, 3}};
  world.cells[4].detector = {{12, 2}, {14, 16}};

  world.cells[5].walls = {1, 7, 5, 3, 2};
  world.cells[5].portals = {{57, 2}, {56, 3}, {55, 4}};
  world.cells[5].detector = {{4, 0}, {14, 2}};

  for(auto& s : world.segments)
  {
    s.a = (s.a - Vec2(3, 3));
    s.b = (s.b - Vec2(3, 3));
  }

  for(auto& c : world.cells)
  {
    c.detector.mins = (c.detector.mins - Vec2(3, 3));
    c.detector.maxs = (c.detector.maxs - Vec2(3, 3));
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

Segment intersectSegmentWithPositiveHalfPlane(Plane p, Segment s)
{
  float distA = dot_product(p.normal, s.a);
  float distB = dot_product(p.normal, s.b);

  if(distA < p.dist && distB < p.dist)
    return {}; // the segment is fully in the negative half space

  if(distA > p.dist && distB > p.dist)
    return s; //  the segment is fully in the positive half space

  float fraction = (p.dist - distA) / (distB - distA);

  const auto I = s.a + (s.b - s.a) * fraction;

  if(distA > p.dist)
    return {s.a, I};
  else
    return {I, s.b};
}

Segment clipSegmentToFrustum(Frustum frustum, Segment s)
{
  s = intersectSegmentWithPositiveHalfPlane(frustum.a, s);
  s = intersectSegmentWithPositiveHalfPlane(frustum.b, s);
  return s;
}

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
      auto clippedSegment = portalSegment;
      if(!(frustum.a.normal == Vec2{}))
        clippedSegment = clipSegmentToFrustum(frustum, clippedSegment);

      auto subFrustum = computeFrustum(pos, clippedSegment);
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

    pos = Vec2(0, 0);
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

  struct HalfLine
  {
    Vec2 point;
    Vec2 tangent;
  };

  static void drawFrustum(IDrawer* drawer, const Frustum& frustum)
  {
    {
      auto point = frustum.a.normal * frustum.a.dist;
      auto tangent = rotateLeft(frustum.a.normal);
      auto halfLine = clipLineAgainstPlane(point, tangent, frustum.b);
      drawHalfLine(drawer, halfLine);
      drawer->line(halfLine.point + halfLine.tangent * 3, halfLine.point + halfLine.tangent * 3 + frustum.a.normal * 1);
    }
    {
      auto point = frustum.b.normal * frustum.b.dist;
      auto tangent = rotateLeft(frustum.b.normal);
      auto halfLine = clipLineAgainstPlane(point, tangent, frustum.a);
      drawHalfLine(drawer, halfLine);
      drawer->line(halfLine.point + halfLine.tangent * 3, halfLine.point + halfLine.tangent * 3 + frustum.b.normal * 1);
    }
  }

  static HalfLine clipLineAgainstPlane(Vec2 linePoint, Vec2 lineTangent, Plane plane)
  {
    auto planeP = plane.normal * plane.dist;
    const auto k = dot_product(planeP - linePoint, plane.normal) / dot_product(lineTangent, plane.normal);
    HalfLine r;
    r.point = linePoint + lineTangent * k;
    r.tangent = lineTangent;
    if(dot_product(r.tangent, plane.normal) < 0)
      r.tangent = r.tangent * -1;
    return r;
  }

  static void drawHalfLine(IDrawer* drawer, const HalfLine& halfLine)
  {
    drawer->line(halfLine.point, halfLine.point + halfLine.tangent * 100);
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
