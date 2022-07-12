// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Constructive Solid Geometry

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

#include "bsp.h"
#include "polygon.h"
#include "random.h"

namespace
{

Polygon2f polygonUnion(Polygon2f a, Polygon2f b)
{
  return a;
  (void)b;
}

std::vector<Hyperplane> testedPlanes;
int depth;

struct Scope
{
  Scope() { ++depth; }
  ~Scope() { --depth; }
};

float raycast(Vec2 a, Vec2 b, const BspNode* node)
{
  Scope s;
  for(int k = 0; k < depth; ++k)
    printf("  ");
  printf("plane: n=(%.2f,%.2f),d=%.2f\n", node->plane.normal.x, node->plane.normal.y, node->plane.dist);
  const auto pa = dot_product(a, node->plane.normal) - node->plane.dist;
  const auto pb = dot_product(b, node->plane.normal) - node->plane.dist;

  for(int k = 0; k < depth; ++k)
    printf("  ");
  printf("pa=%.2f, ", pa);
  printf("pb=%.2f : ", pb);

  testedPlanes.push_back(node->plane);

  if(pa < +BspEpsilon && pb < +BspEpsilon)
  {
    printf("fully inside\n");
    if(node->negChild)
    {
      for(int k = 0; k < depth; ++k)
        printf("  ");
      printf("negative child:\n");
      return raycast(a, b, node->negChild.get());
    }
    else
    {
      for(int k = 0; k < depth; ++k)
        printf("  ");
      printf("all solid\n");
      return 0; // all solid
    }
  }

  if(pa > +BspEpsilon && pb > +BspEpsilon)
  {
    printf("fully outside\n");
    if(node->posChild)
      return raycast(a, b, node->posChild.get());
    else
      return 1; // all empty
  }

  if(pa < +BspEpsilon && pb > +BspEpsilon)
  {
    printf("exiting\n");
    if(!node->negChild)
      return 0; // all solid
    auto ratio = raycast(a, b, node->negChild.get());
    if(ratio < 1)
      return ratio;
    else if(node->posChild)
      return raycast(a, b, node->posChild.get());
    else
      return 1;
  }

  if(pa > +BspEpsilon && pb < +BspEpsilon)
  {
    const auto intersection = a + (b - a) * (pa / (pa - pb));
    printf("entering\n");
    if(node->posChild)
    {
      for(int k = 0; k < depth; ++k)
        printf("  ");
      printf("positive child:\n");
      auto ratio = raycast(a, intersection, node->posChild.get());
      if(ratio < 1)
        return ratio * (pa / (pa - pb));
    }
    else
    {
      for(int k = 0; k < depth; ++k)
        printf("  ");
      printf("positive child: none\n");
    }

    if(node->negChild)
    {
      for(int k = 0; k < depth; ++k)
        printf("  ");
      printf("negative child:\n");
      auto ratio = raycast(a, b, node->negChild.get());
      if(ratio < 1)
        return ratio;
    }
    else
    {
      return pa / (pa - pb);
    }

    return 1;
  }

  printf("yo\n");
  return 0.4;
}

bool pointIsInside(Vec2 pos, const BspNode* node)
{
  bool inside = false;
  while(node)
  {
    if(dot_product(pos, node->plane.normal) - node->plane.dist < 0)
    {
      node = node->negChild.get();
      inside = true;
    }
    else
    {
      node = node->posChild.get();
      inside = false;
    }
  }

  return inside;
}

struct CsgApp : IApp
{
  CsgApp()
  {
    if(0)
    {
      m_addPolygons.push_back({});
      auto& poly = m_addPolygons.back();

      poly.vertices.push_back({-10, -12});
      poly.vertices.push_back({+10, -10});
      poly.vertices.push_back({+7, +10});
      poly.vertices.push_back({-10, +8});
      poly.vertices.push_back({0, +4});

      poly.faces.push_back({0, 1});
      poly.faces.push_back({1, 2});
      poly.faces.push_back({2, 3});
      poly.faces.push_back({3, 4});
      poly.faces.push_back({4, 0});
    }

    if(0)
    {
      m_addPolygons.push_back({});
      auto& poly = m_addPolygons.back();

      poly.vertices.push_back({-5, -10});
      poly.vertices.push_back({+14, -11});
      poly.vertices.push_back({+9, +9});
      poly.vertices.push_back({-12, +5});

      poly.faces.push_back({0, 1});
      poly.faces.push_back({1, 2});
      poly.faces.push_back({2, 3});
      poly.faces.push_back({3, 0});
    }

    if(1)
    {
      m_addPolygons.push_back({});
      auto& poly = m_addPolygons.back();

      const Vec2 center = randomPos({-10, -10}, {+10, +10});
      const auto baseRadius = randomFloat(3, 7);
      const auto spread = randomFloat(0.5, 1);
      const auto phase = randomFloat(0, M_PI);

      const int N = 3; // 24;
      for(int i = 0; i < N; ++i)
      {
        const auto radius = baseRadius * (1.0 + (i % 2 ? 0 : spread));
        Vec2 pos = center;
        pos.x += cos((2 * M_PI * i) / N + phase) * radius;
        pos.y += sin((2 * M_PI * i) / N + phase) * radius;
        poly.vertices.push_back(pos);
        poly.faces.push_back({i, (i + 1) % N});
      }
    }

    recompute();
  }

  void draw(IDrawer* drawer) override
  {
    for(auto& poly : m_addPolygons)
      drawPolygon(drawer, poly, Green);
    for(auto& poly : m_subPolygons)
      drawPolygon(drawer, poly, Red);

    drawPolygon(drawer, m_result, Yellow);

    drawVertex(drawer, m_probePos, m_probeInsidePolygon ? Red : Yellow);
    drawer->line(m_probePos, m_probePos2, Red);
    drawer->line(m_probePos, m_probePos + (m_probePos2 - m_probePos) * m_ratio, Green);
    drawer->text(m_probePos, "SRC", Yellow);
    drawer->text(m_probePos2, "DST", Yellow);

    int i = 0;
    for(auto& plane : testedPlanes)
    {
      auto p = plane.normal * plane.dist;
      auto t = rotateLeft(plane.normal);
      drawer->line(p - t * 100, p + t * 100, Yellow);
      drawer->line(p, p + plane.normal, Yellow);

      char name[256];
      sprintf(name, "P%d", i);
      drawer->text(p, name, Yellow);
      ++i;
    }
  }

  void drawPolygon(IDrawer* drawer, const Polygon2f& poly, Color color)
  {
    for(auto f : poly.faces)
    {
      drawer->line(poly.vertices[f.a], poly.vertices[f.b], color);
      drawVertex(drawer, poly.vertices[f.a], color);
    }
  }

  void drawVertex(IDrawer* drawer, Vec2 pos, Color color) { drawer->rect(pos - Vec2{0.2, 0.2}, Vec2{0.4, 0.4}, color); }

  void keydown(Key key) override
  {
    auto& pos = m_selection ? m_probePos : m_probePos2;
    switch(key)
    {
    case Key::Left:
      pos.x -= 1;
      break;
    case Key::Right:
      pos.x += 1;
      break;
    case Key::Up:
      pos.y += 1;
      break;
    case Key::Down:
      pos.y -= 1;
      break;
    case Key::Space:
      m_selection = !m_selection;
      break;
    default:
      break;
    }
    recompute();
  }

  void recompute()
  {
    m_result = {};

    for(auto& p : m_addPolygons)
      m_result = polygonUnion(m_result, p);

    for(auto& p : m_addPolygons)
      createBspTree(p);

    auto bsp = createBspTree(m_addPolygons[0]);
    m_probeInsidePolygon = pointIsInside(m_probePos, bsp.get());
    printf("------------ raycast ------------\n");
    testedPlanes.clear();
    m_ratio = raycast(m_probePos, m_probePos2, bsp.get());
  }

  bool m_selection = false;
  Vec2 m_probePos{};
  Vec2 m_probePos2{};
  float m_ratio = 0;
  bool m_probeInsidePolygon = false;
  std::vector<Polygon2f> m_addPolygons;
  std::vector<Polygon2f> m_subPolygons;
  Polygon2f m_result;
};

const int registered = registerApp("App.CSG", []() -> IApp* { return new CsgApp; });
}
