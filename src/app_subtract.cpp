// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Polygon convex subtraction

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cassert>
#include <cmath>
#include <vector>

#include "random.h"

namespace
{

struct Hyperplane
{
  Vec2 normal;
  float dist;
};

struct ConvexPolygon
{
  Vec2 pos;
  std::vector<Hyperplane> planes;
};

std::vector<ConvexPolygon> subtract(ConvexPolygon minuend, const ConvexPolygon& subtrahend)
{
  std::vector<ConvexPolygon> fragments;

  // iteratively cut 'minuend' according to the planes of 'subtrahend'
  for(auto& sub : subtrahend.planes)
  {
    // frag is the convex in-front-of the cutting plane
    ConvexPolygon frag = minuend;
    frag.planes.push_back({-sub.normal, -sub.dist - dot_product(subtrahend.pos - minuend.pos, sub.normal)});
    fragments.push_back(frag);

    // minuend becomes the convex behind the cutting plane
    minuend.planes.push_back({sub.normal, sub.dist + dot_product(subtrahend.pos - minuend.pos, sub.normal)});
  }
  return fragments;
}

ConvexPolygon randomPolygon()
{
  ConvexPolygon r{};

  const float dist = randomFloat(3.0f, 10.0f);
  const float tilting = randomFloat(0, M_PI);
  const int N = randomInt(3, 9);

  for(int i = 0; i < N; ++i)
  {
    const auto angle = i * M_PI * 2.0 / N + tilting + randomFloat(-0.3, 0.3);
    Hyperplane p;
    p.dist = dist + randomFloat(-3.0f, 3.0f);
    p.normal.x = cos(angle);
    p.normal.y = sin(angle);

    r.planes.push_back(p);
  }

  return r;
}

struct HyperFace
{
  Vec2 a, b;
};

std::vector<HyperFace> clip(const std::vector<HyperFace>& faces, const Hyperplane& plane)
{
  std::vector<HyperFace> r;

  std::vector<Vec2> orphans;

  for(auto f : faces)
  {
    const auto dist_a = dot_product(f.a, plane.normal) - plane.dist;
    const auto dist_b = dot_product(f.b, plane.normal) - plane.dist;

    if(dist_a > 0 && dist_b > 0)
      continue; // all outside: discard

    if(dist_a < 0 && dist_b < 0)
    {
      r.push_back(f); // all inside: the face doesn't intersect the plane
      continue;
    }

    const auto intersection = f.a + (f.b - f.a) * (dist_a / (dist_a - dist_b));

    if(dist_a < 0 && dist_b > 0)
    {
      r.push_back({f.a, intersection});
      orphans.push_back(intersection);
      continue;
    }

    if(dist_a > 0 && dist_b < 0)
    {
      r.push_back({f.b, intersection});
      orphans.push_back(intersection);
      continue;
    }
  }

  if(orphans.size())
  {
    assert(orphans.size() == 2);
    r.push_back({orphans[0], orphans[1]});
  }

  return r;
}

std::vector<HyperFace> tesselate(const ConvexPolygon& polygon)
{
  std::vector<HyperFace> r;

  // make an infinite box
  {
    const float max = +200;
    const float min = -200;
    r.push_back({{min, min}, {max, min}});
    r.push_back({{max, min}, {max, max}});
    r.push_back({{max, max}, {min, max}});
    r.push_back({{min, max}, {min, min}});
  }

  for(auto& plane : polygon.planes)
    r = clip(r, {plane.normal, plane.dist + dot_product(plane.normal, polygon.pos)});

  return r;
}

struct SubtractApp : IApp
{
  SubtractApp()
  {
    m_a = randomPolygon();
    m_b = randomPolygon();

    m_a.pos = {-2, 0};
    m_b.pos = {6, 1};
  }

  void draw(IDrawer* drawer) override
  {
    drawPolygon(drawer, m_a, Red);
    drawPolygon(drawer, m_b, Green);

    for(auto& frag : subtract(m_a, m_b))
      drawPolygon(drawer, frag, Yellow);

    if(0)
      drawClipTest(drawer);
  }

  void drawClipTest(IDrawer* drawer)
  {
    std::vector<HyperFace> faces;

    {
      const float max = +15; // +1.0 / 0.0;
      const float min = -15; // -1.0 / 0.0;
      faces.push_back({{min, min}, {max, min}});
      faces.push_back({{max, min}, {max, max}});
      faces.push_back({{max, max}, {min, max}});
      faces.push_back({{min, max}, {min, min}});
    }

    Hyperplane plane;
    static float m_angle = 0;
    m_angle += 0.01;
    plane.normal.x = cos(m_angle);
    plane.normal.y = sin(m_angle);
    plane.dist = (sin(m_angle * 3.0) + 1.0) * 2.5;
    const auto clippedFaces = clip(faces, plane);

    for(auto& face : faces)
    {
      drawer->line(face.a, face.b, Blue);
      drawCross(drawer, face.a, Blue);
      drawCross(drawer, face.b, Blue);
    }

    for(auto& face : clippedFaces)
    {
      drawer->line(face.a, face.b, Yellow);
      drawCross(drawer, face.a, Yellow);
      drawCross(drawer, face.b, Yellow);
    }
  }

  void drawPolygon(IDrawer* drawer, const ConvexPolygon& poly, Color color)
  {
    if(0)
      for(auto& plane : poly.planes)
        drawer->line(poly.pos, poly.pos + plane.normal * plane.dist, color);

    auto faces = tesselate(poly);
    Vec2 center{};
    for(auto& v : faces)
    {
      center += v.a;
      center += v.b;
    }
    center = center * (1.0 / (faces.size() * 2));
    drawCross(drawer, center, color);
    for(auto& face : faces)
      drawer->line(face.a, face.b, color);
  }

  void drawCross(IDrawer* drawer, Vec2 pos, Color color)
  {
    drawer->line(pos - Vec2{1, 0}, pos + Vec2{1, 0}, color);
    drawer->line(pos - Vec2{0, 1}, pos + Vec2{0, 1}, color);
  }

  void keydown(Key key) override
  {
    switch(key)
    {
    case Key::Left:
      m_a.pos.x -= 1;
      break;
    case Key::Right:
      m_a.pos.x += 1;
      break;
    case Key::Up:
      m_a.pos.y += 1;
      break;
    case Key::Down:
      m_a.pos.y -= 1;
      break;
    default:
      break;
    }
  }

  ConvexPolygon m_a, m_b;
};

const int registered = registerApp("App.Subtract", []() -> IApp* { return new SubtractApp; });
}
