// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Create a BSP tree from a random polygon

#include "core/algorithm_app.h"
#include "core/geom.h"
#include "core/sandbox.h"

#include <cassert>
#include <cmath>
#include <vector>

#include "bounding_box.h"
#include "bsp.h"
#include "random.h"
#include "random_polygon.h"
#include "split_polygon.h"

namespace
{

void drawPolygon(const Polygon2f& poly, Color color)
{
  for(auto& face : poly.faces)
  {
    auto v0 = poly.vertices[face.a];
    auto v1 = poly.vertices[face.b];
    sandbox_line(v0, v1, color);
    sandbox_circle(v0, 0.1, color);
  }
}

float intersectRayWithHyperPlane(Vec2 rayStart, Vec2 rayDir, Vec2 hyperplaneNormal, float hyperplaneDist)
{
  // We want:
  // [1] dotProduct(I - pointOnHp, hyperplaneNormal) = 0;
  // [2] I = rayStart + k * rayDir
  // Using [1]:
  // => dotProduct(I, hyperplaneNormal) = dotProduct(pointOnHp, hyperplaneNormal);
  // => dotProduct(I, hyperplaneNormal) = hyperplaneDist;
  // Using [2]:
  // => dotProduct(rayStart + k * rayDir, hyperplaneNormal) = hyperplaneDist;
  // => dotProduct(rayStart, hyperplaneNormal) + k * dotProduct(rayDir, hyperplaneNormal) = hyperplaneDist;
  // => k * dotProduct(rayDir, hyperplaneNormal) = hyperplaneDist - dotProduct(rayStart, hyperplaneNormal);
  //
  //        hyperplaneDist - dotProduct(rayStart, hyperplaneNormal)
  // => k * --------------------------------------------------------
  //                  dotProduct(rayDir, hyperplaneNormal)
  //
  const float num = hyperplaneDist - dotProduct(rayStart, hyperplaneNormal);
  const float den = dotProduct(rayDir, hyperplaneNormal);

  return num / den;
}

float sign(float a)
{
  if(a >= 0)
    return +1;
  else
    return -1;
}

struct BspBuild
{
  static Polygon2f generateInput() { return createRandomPolygon2f(); }

  struct BspHolder
  {
    std::unique_ptr<BspNode> root;
  };

  static BspHolder execute(Polygon2f input) { return {createBspTree(input)}; }

  static void display(const Polygon2f& input, const BspHolder& output)
  {
    drawPolygon(input, Gray);

    if(output.root)
    {
      std::vector<Hyperplane> clips;
      drawBspNode(output.root.get(), clips);
    }
  }

  static void drawBspNode(const BspNode* node, std::vector<Hyperplane>& clips)
  {
    const Vec2 rayStart = node->plane.normal * node->plane.dist;
    const Vec2 rayDir = rotateLeft(node->plane.normal);

    float enter = +100.0f;
    float leave = -100.0f;

    for(auto& clipPlane : clips)
    {
      const float k = intersectRayWithHyperPlane(rayStart, rayDir, clipPlane.normal, clipPlane.dist);

      if(dotProduct(clipPlane.normal, rayDir) < 0)
      {
        if(k < enter && fabs(k) < 1.0 / 0.0)
          enter = k;
      }
      else
      {
        if(k > leave && fabs(k) < 1.0 / 0.0)
          leave = k;
      }
    }

    if(fabs(enter) > 100)
      enter = sign(enter) * 100;

    if(fabs(leave) > 100)
      leave = sign(leave) * 100;

    const Color colors[] = {
          Red,
          Green,
          LightBlue,
          Yellow,
    };

    const auto beg = rayStart + rayDir * enter;
    const auto end = rayStart + rayDir * leave;
    const auto color = colors[clips.size() % (sizeof(colors) / sizeof(*colors))];

    sandbox_line(beg, end, color);

    if(node->posChild)
    {
      clips.push_back(node->plane);
      drawBspNode(node->posChild.get(), clips);
      clips.pop_back();
    }

    if(node->negChild)
    {
      Hyperplane reversedPlane;
      reversedPlane.normal = node->plane.normal * -1;
      reversedPlane.dist = node->plane.dist * -1;
      clips.push_back(reversedPlane);
      drawBspNode(node->negChild.get(), clips);
      clips.pop_back();
    }
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<BspBuild>>()); }
const int registered = registerApp("Bsp.Build", &create);
}
