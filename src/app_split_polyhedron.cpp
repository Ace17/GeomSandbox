// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Split a concave polyhedron along a line

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cmath>

#include "random_polyhedron.h"
#include "split_polyhedron.h"

namespace
{

void drawPolygon(IDrawer* drawer, const Polyhedron3f& poly, Color color)
{
  for(auto& face : poly.faces)
  {
    const int N = face.indices.size();

    Vec3 faceCenter{};
    for(int i = 0; i < N; ++i)
    {
      auto a = poly.vertices[face.indices[i]];
      auto b = poly.vertices[face.indices[(i + 1) % N]];
      drawer->line(a, b, color);
      faceCenter += a;
    }

    {
      faceCenter *= (1.0f / N);

      const auto A = poly.vertices[face.indices[0]];
      const auto B = poly.vertices[face.indices[1]];
      const auto C = poly.vertices[face.indices[2]];
      Vec3 faceNormal = normalize(crossProduct(B - A, C - A));

      drawer->line(faceCenter + faceNormal * 0.5, faceCenter + faceNormal * 1.0f, {0.4f, 0, 0, 1});
      drawer->line(faceCenter, faceCenter + faceNormal * 0.5f, {0.7f, 0, 0, 1});
    }
  }
}

struct SplitPolyhedronApp : IApp
{
  SplitPolyhedronApp()
  {
    m_poly = createRandomPolyhedron3f();

    m_abc[0] = {-20, -20, 0};
    m_abc[1] = {+14, 0, 0};
    m_abc[2] = {+14, 20, 10};

    recomputePlaneFromABC();
    compute();
  }

  void draw(IDrawer* drawer) override
  {
    drawer->line({0, 0, 0}, {1, 0, 0}, Red);
    drawer->line({0, 0, 0}, {0, 1, 0}, Green);
    drawer->line({0, 0, 0}, {0, 0, 1}, LightBlue);

    // draw cut plane
    {
      auto p = m_cutPlane.normal * m_cutPlane.dist;

      Vec3 tangentBasedOnX = crossProduct(m_cutPlane.normal, Vec3(1, 0, 0));
      Vec3 tangentBasedOnY = crossProduct(m_cutPlane.normal, Vec3(0, 1, 0));
      auto mag1 = magnitude(tangentBasedOnX);
      auto mag2 = magnitude(tangentBasedOnY);
      Vec3 tangent1 = normalize(mag1 > mag2 ? tangentBasedOnX : tangentBasedOnY);
      Vec3 tangent2 = crossProduct(m_cutPlane.normal, tangent1);

      for(int i = -10; i <= 10; ++i)
      {
        auto q = p + tangent1 * i * 4;
        drawer->line(q + tangent2 * 40, q - tangent2 * 40, {0.2, 0, 0, 1});
      }

      for(int i = -10; i <= 10; ++i)
      {
        auto q = p + tangent2 * i * 4;
        drawer->line(q + tangent1 * 40, q - tangent1 * 40, {0.2, 0, 0, 1});
      }
    }

    if(0)
      drawPolygon(drawer, m_poly, White);
    drawPolygon(drawer, m_front, Yellow);
    drawPolygon(drawer, m_back, Green);
  }

  void processEvent(InputEvent inputEvent) override
  {
    if(inputEvent.pressed)
      keydown(inputEvent.key);
  }

  void keydown(Key key)
  {
    auto& p = m_abc[m_selection];

    const auto speed = 0.1;

    switch(key)
    {
    case Key::Left:
      p.x -= speed;
      recomputePlaneFromABC();
      break;
    case Key::Right:
      p.x += speed;
      recomputePlaneFromABC();
      break;
    case Key::Up:
      p.y += speed;
      recomputePlaneFromABC();
      break;
    case Key::Down:
      p.y -= speed;
      recomputePlaneFromABC();
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
      m_selection = (m_selection + 1) % 3;
      break;
    default:
      break;
    }

    compute();
  }

  void compute()
  {
    m_front = {};
    m_back = {};

    splitPolyhedronAgainstPlane(m_poly, m_cutPlane, m_front, m_back);
  }

  void recomputePlaneFromFace()
  {
    auto face = m_poly.faces[m_selectedFace];
    auto a = m_poly.vertices[face.indices[0]];
    auto b = m_poly.vertices[face.indices[1]];
    auto c = m_poly.vertices[face.indices[2]];
    m_cutPlane.normal = normalize(crossProduct(b - a, c - a));
    m_cutPlane.dist = dotProduct(m_cutPlane.normal, a);

    m_abc[0] = a;
    m_abc[1] = b;
    m_abc[2] = c;
  }

  void recomputePlaneFromABC()
  {
    m_cutPlane.normal = normalize(crossProduct(m_abc[1] - m_abc[0], m_abc[2] - m_abc[0]));
    m_cutPlane.dist = dotProduct(m_cutPlane.normal, m_abc[0]);
  }

  Plane3 m_cutPlane;
  Vec3 m_abc[3];
  int m_selection = 0;
  int m_selectedFace = 0;
  Polyhedron3f m_poly;
  Polyhedron3f m_front, m_back;
};

const int registered = registerApp("Split/Polyhedron", []() -> IApp* { return new SplitPolyhedronApp; });
}
