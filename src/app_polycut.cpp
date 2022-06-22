// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Split a concave polygon along a line

#include <cassert>
#include <cmath>
#include <vector>

#include "app.h"
#include "drawer.h"
#include "geom.h"
#include "random.h"
#include "random_polygon.h"
#include "split_polygon.h"
#include "visualizer.h"

namespace
{
float abs(float a) { return a >= 0 ? a : -a; }
float magnitude(Vec2 v) { return sqrt(v * v); }
float dot_product(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
Vec2 normalize(Vec2 v) { return v * (1.0 / magnitude(v)); }
Vec2 rotateLeft(Vec2 v) { return Vec2(-v.y, v.x); }

void drawPolygon(IDrawer* drawer, const Polygon2f& poly, Color color)
{
  for(auto& face : poly.faces)
  {
    auto v0 = poly.vertices[face.a];
    auto v1 = poly.vertices[face.b];
    drawer->line(v0, v1, color);
    drawer->circle(v0, 0.1, color);

    // draw normal, pointing to the exterior
    const auto T = normalize(v1 - v0);
    const auto N = -rotateLeft(T);
    drawer->line((v0 + v1) * 0.5, (v0 + v1) * 0.5 + N * 0.15, color);

    // draw hatches, on the interior
    const auto hatchAttenuation = 0.4f;
    const Color hatchColor = {color.r * hatchAttenuation, color.g * hatchAttenuation, color.b * hatchAttenuation,
          color.a * hatchAttenuation};
    const auto dist = magnitude(v1 - v0);
    for(float f = 0; f < dist; f += 0.15)
    {
      const auto p = v0 + T * f;
      drawer->line(p, p - N * 0.15 + T * 0.05, hatchColor);
    }
  }
}

struct PolycutApp : IApp
{
  PolycutApp()
  {
    if(1)
    {
      m_poly = createRandomPolygon2f(gNullVisualizer);

      const auto i0 = (int)m_poly.vertices.size();
      m_poly.vertices.push_back({-15, -15});
      m_poly.vertices.push_back({+15, -15});
      m_poly.vertices.push_back({+15, +15});
      m_poly.vertices.push_back({-15, +15});

      m_poly.faces.push_back({i0 + 0, i0 + 1});
      m_poly.faces.push_back({i0 + 1, i0 + 2});
      m_poly.faces.push_back({i0 + 2, i0 + 3});
      m_poly.faces.push_back({i0 + 3, i0 + 0});

      m_a = {-20, -20};
      m_b = {+14, 20};
    }
    else
    {
      m_poly.vertices.push_back({-15, -15});
      m_poly.vertices.push_back({+15, -15});
      m_poly.vertices.push_back({+15, +15});
      m_poly.vertices.push_back({-15, +15});

      m_poly.faces.push_back({0, 1});
      m_poly.faces.push_back({1, 2});
      m_poly.faces.push_back({2, 3});
      m_poly.faces.push_back({3, 0});

      m_a = {-15, +40};
      m_b = {-15, -40};
    }

    recomputePlaneFromAB();
    compute();
  }

  void draw(IDrawer* drawer) override
  {
    // draw cut plane
    {
      auto p = m_cutPlane.normal * m_cutPlane.dist;
      auto tangent = rotateLeft(m_cutPlane.normal);
      drawer->line(p + tangent * 100, p - tangent * 100, Red);
    }
    // drawPolygon(drawer, m_poly, White);
    drawPolygon(drawer, m_front, Yellow);
    drawPolygon(drawer, m_back, Green);
  }

  void keydown(Key key) override
  {
    auto& p = m_selection ? m_a : m_b;

    const auto speed = 0.1;

    switch(key)
    {
    case Key::Left:
      p.x -= speed;
      recomputePlaneFromAB();
      break;
    case Key::Right:
      p.x += speed;
      recomputePlaneFromAB();
      break;
    case Key::Up:
      p.y += speed;
      recomputePlaneFromAB();
      break;
    case Key::Down:
      p.y -= speed;
      recomputePlaneFromAB();
      break;
    case Key::PageUp:
      m_selectedFace++;
      m_selectedFace %= (int)m_poly.faces.size();
      recomputePlaneFromFace();
      break;
    case Key::PageDown:
      m_selectedFace += ((int)m_poly.faces.size() - 1);
      m_selectedFace %= m_poly.faces.size();
      recomputePlaneFromFace();
      break;
    case Key::Space:
      m_selection = (m_selection + 1) % 2;
      break;
    }

    compute();
  }

  void compute()
  {
    m_front = {};
    m_back = {};

    Polygon2f front, back;
    splitPolygonAgainstPlane(m_poly, m_cutPlane, m_front, m_back);
  }

  void recomputePlaneFromFace()
  {
    auto face = m_poly.faces[m_selectedFace];
    auto a = m_poly.vertices[face.a];
    auto b = m_poly.vertices[face.b];
    m_cutPlane.normal = rotateLeft(normalize(b - a));
    m_cutPlane.dist = dot_product(m_cutPlane.normal, a);

    m_a = a;
    m_b = b;
  }

  void recomputePlaneFromAB()
  {
    m_cutPlane.normal = rotateLeft(normalize(m_b - m_a));
    m_cutPlane.dist = dot_product(m_cutPlane.normal, m_a);
  }

  Plane m_cutPlane;
  Vec2 m_a, m_b;
  int m_selection = 0;
  int m_selectedFace = 0;
  Polygon2f m_poly;
  Polygon2f m_front, m_back;
};

const int registered = registerApp("PolyCut", []() -> IApp* { return new PolycutApp; });
}
