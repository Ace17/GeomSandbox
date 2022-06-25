// Copyright (C) 2021 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Polygon Triangulation: Ear-clipping Algorithm.

#include <algorithm>
#include <cmath>
#include <vector>

#include "algorithm_app.h"
#include "bounding_box.h"
#include "random.h"
#include "random_polygon.h"

namespace
{
static float cross(Vec2 v, Vec2 w) { return v.x * w.y - v.y * w.x; }

struct Segment
{
  int a, b;
};

struct Ear
{
  int tip;
  int a, b;
};

void drawPolygon(const Polygon2f& input, Color color)
{
  for(Face face : input.faces)
    sandbox_line(input.vertices[face.a], input.vertices[face.b], color);
}

Ear polygonEarFromIndex(const Polygon2f& polygon, int index)
{
  Ear ear;
  ear.tip = index;
  ear.a = -1;
  ear.b = -1;
  for(Face face : polygon.faces)
  {
    if(face.a == index)
      ear.b = face.b;
    else if(face.b == index)
      ear.a = face.a;
  }

  return ear;
}

bool isInsideTriangle(Vec2 a, Vec2 b, Vec2 c, Vec2 point)
{
  auto isOnTheRightOfSegment = [](Vec2 segmentStart, Vec2 segmentEnd, Vec2 point)
  {
    const Vec2 a = point - segmentStart;
    const Vec2 b = segmentEnd - point;
    const float epsilon = 0.0001;
    return cross(a, b) > -epsilon;
  };

  return isOnTheRightOfSegment(a, b, point) && isOnTheRightOfSegment(b, c, point) && isOnTheRightOfSegment(c, a, point);
}

bool isValidEar(const Polygon2f& polygon, Ear& ear)
{
  if(ear.a == -1 || ear.b == -1)
    return false;

  const Vec2 a = polygon.vertices[ear.a];
  const Vec2 b = polygon.vertices[ear.b];
  const Vec2 tip = polygon.vertices[ear.tip];
  const Vec2 segmentA = tip - a;
  const Vec2 segmentB = b - tip;
  const float epsilon = 0.0001; // In order to ignore "flat" corners.
  const bool convexAngle = cross(segmentA, segmentB) < -epsilon;
  if(!convexAngle)
    return false;

  for(int idx = 0; idx < polygon.vertices.size(); idx++)
  {
    if(idx != ear.tip && idx != ear.a && idx != ear.b && isInsideTriangle(a, tip, b, polygon.vertices[idx]))
      return false;
  }

  return true;
};

void removeFacesForCorner(Polygon2f& polygon, int idx)
{
  auto isConcernedFace = [idx](Face face) { return (face.a == idx || face.b == idx); };
  auto it = std::remove_if(polygon.faces.begin(), polygon.faces.end(), isConcernedFace);
  polygon.faces.erase(it, polygon.faces.end());
}

Segment clipEar(Polygon2f& polygon)
{
  for(int idx = 0; idx < polygon.vertices.size(); idx++)
  {
    Ear ear = polygonEarFromIndex(polygon, idx);
    if(isValidEar(polygon, ear))
    {
      removeFacesForCorner(polygon, idx);
      polygon.faces.push_back({ear.a, ear.b});
      return {ear.a, ear.b};
    }
  }

  // Should never be reached : a polygon with 4 or more vertices should have at least 2 ears.
  return {};
}

struct EarClippingAlgorithm
{
  static Polygon2f generateInput() { return createRandomPolygon2f(); }

  static std::vector<Segment> execute(Polygon2f input)
  {
    std::vector<Segment> result;
    while(input.faces.size() > 3)
    {
      drawPolygon(input, Yellow);
      sandbox_breakpoint();
      Segment segment = clipEar(input);
      result.push_back(segment);
    }
    drawPolygon(input, Yellow);
    sandbox_breakpoint();
    return result;
  }

  static void drawStatic(const Polygon2f& input, const std::vector<Segment>& output)
  {
    drawPolygon(input, White);
    for(auto& segment : output)
      sandbox_line(input.vertices[segment.a], input.vertices[segment.b], Green);
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<EarClippingAlgorithm>>()); }
const int reg = registerApp("EarClippingAlgorithm", &create);
}
