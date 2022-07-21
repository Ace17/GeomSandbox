// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Raycast against a BSP

#include "core/algorithm_app.h"
#include "core/geom.h"
#include "core/sandbox.h"

#include <cassert>
#include <cmath>
#include <vector>
#include <cstdio>

#include "bounding_box.h"
#include "bsp.h"
#include "random.h"
#include "random_polygon.h"
#include "split_polygon.h"

namespace
{
void drawPlane(const Hyperplane& plane)
{
  auto p = plane.normal * plane.dist + Vec2(0, 0.1);
  auto t = rotateLeft(plane.normal);
  sandbox_line(p - t * 100, p + t * 100, Red);
  sandbox_line(p, p + plane.normal, Red);
  sandbox_printf("drawPlane: {%.2f,%.2f} {%.2f,%.2f}\n", p.x, p.y, t.x, t.y);
}

float raycast_rec(Vec2 a, Vec2 b, const BspNode* node);

float raycast_aux(Vec2 a, Vec2 b, const BspNode* node)
{
  const float ratio = raycast_rec(a, b, node);

  drawPlane(node->plane);
  {
    const auto pa = dot_product(a, node->plane.normal) - node->plane.dist;
    const auto pb = dot_product(b, node->plane.normal) - node->plane.dist;

    const auto p0 = node->plane.normal * node->plane.dist;
    auto acolor = pa < 0 ? Red : Green;
    auto bcolor = pb < 0 ? Red : Green;
    sandbox_circle(p0 + node->plane.normal * pa, 0.2, acolor);
    sandbox_text(p0 + node->plane.normal * pa, "a", acolor);
    sandbox_circle(p0 + node->plane.normal * pb, 0.2, bcolor);
    sandbox_text(p0 + node->plane.normal * pb, "b", bcolor);
    sandbox_line(a, a + (b - a) * ratio, Green);

    char msg[256];
    sprintf(msg, "ratio=%.2f", ratio);
    sandbox_text({0,0}, msg);
  }
  sandbox_breakpoint();

  return ratio;
}

template<typename T>
T lerp(T a, T b, float alpha)
{
  return a * (1 - alpha) + b * alpha;
}

float raycast(Vec2 a, Vec2 b, const BspNode* root)
{
  struct Chunk
  {
    float beg, end; // between 0 and 1 from 'a' to 'b'
    const BspNode* node;
    bool solid = false; // only valid if `node` is null
  };
  std::vector<Chunk> stack;

  stack.push_back(Chunk{0, 1, root});

  float ratio = 1;

  while(stack.size())
  {
    for(auto& entry : stack)
    {
      auto color = &entry == &stack.back() ? Green : LightBlue;

      const auto beg = lerp(a, b, entry.beg);
      const auto end = lerp(a, b, entry.end);
      sandbox_line(beg, end, color);
      char buf[256];
      sprintf(buf, "%d", int(stack.size()) - int(&entry - &stack.front()));
      sandbox_text((beg+end) * 0.5, buf);
      sandbox_circle(beg, 0.2, color);
      sandbox_circle(end, 0.2, color);
    }

    auto curr = stack.back();
    stack.pop_back();

    const auto beg = lerp(a, b, curr.beg);
    const auto end = lerp(a, b, curr.end);

    if(!curr.node)
    {
      if(curr.solid && ratio > curr.beg)
        ratio = curr.beg;
      sandbox_text({}, "leaf");
    }
    else
    {
      const auto proj_beg = dot_product(beg, curr.node->plane.normal) - curr.node->plane.dist;
      const auto proj_end = dot_product(end, curr.node->plane.normal) - curr.node->plane.dist;

      const auto proj_a = dot_product(a, curr.node->plane.normal) - curr.node->plane.dist;
      const auto proj_b = dot_product(b, curr.node->plane.normal) - curr.node->plane.dist;

      const auto pmid = proj_a / (proj_a - proj_b);

      if(proj_beg < +BspEpsilon && proj_end >= +BspEpsilon)
      {
        stack.push_back(Chunk{pmid, curr.end, curr.node->posChild.get(), false});
        stack.push_back(Chunk{curr.beg, pmid, curr.node->negChild.get(), true});
        sandbox_text({}, "crossed (neg to pos)");
      }
      else if(proj_beg >= +BspEpsilon && proj_end < +BspEpsilon)
      {
        stack.push_back(Chunk{pmid, curr.end, curr.node->negChild.get(), true});
        stack.push_back(Chunk{curr.beg, pmid, curr.node->posChild.get(), false});
        sandbox_text({}, "crossed (pos to neg)");
      }
      else if(proj_beg >= +BspEpsilon && proj_end >= +BspEpsilon)
      {
        stack.push_back(Chunk{pmid, curr.end, curr.node->posChild.get(), false});
        sandbox_text({}, "all positive");
      }
      else
      {
        stack.push_back(Chunk{curr.beg, pmid, curr.node->negChild.get(), true});
        sandbox_text({}, "all negative");
      }

      {
        const auto acolor = proj_beg < 0 ? Red : Green;
        const auto bcolor = proj_end < 0 ? Red : Green;

        drawPlane(curr.node->plane);

        const auto p0 = curr.node->plane.normal * curr.node->plane.dist;

        sandbox_circle(p0 + curr.node->plane.normal * proj_beg, 0.2, acolor);
        sandbox_text(p0 + curr.node->plane.normal * proj_beg, "beg", acolor);

        sandbox_circle(p0 + curr.node->plane.normal * proj_end, 0.2, bcolor);
        sandbox_text(p0 + curr.node->plane.normal * proj_end, "end", bcolor);

      }
    }

    sandbox_breakpoint();
  }

  return ratio;
}

float raycast_rec(Vec2 a, Vec2 b, const BspNode* node)
{
  const auto pa = dot_product(a, node->plane.normal) - node->plane.dist;
  const auto pb = dot_product(b, node->plane.normal) - node->plane.dist;

  // The ray is fully on the negative side
  if(pa < +BspEpsilon && pb < +BspEpsilon)
  {
    if(node->negChild)
    {
      return raycast_aux(a, b, node->negChild.get());
    }
    else
    {
      sandbox_text({0,1}, "all solid negative");
      return 0; // all solid
    }
  }

  // The ray is fully on the positive side
  if(pa > +BspEpsilon && pb > +BspEpsilon)
  {
    if(node->posChild)
      return raycast_aux(a, b, node->posChild.get());
    else
    {
      sandbox_text({0,1}, "all empty positive");
      return 1; // all empty
    }
  }

  // The ray goes from the negative side to the positive side
  if(pa < +BspEpsilon && pb > +BspEpsilon)
  {
    const auto intersection_ratio = (pa / (pa - pb));
    const auto intersection = a + (b - a) * intersection_ratio;

    // raycast inside the negative side
    if(!node->negChild)
    {
      sandbox_text({0,1}, "start solid negative (2)");
      return 0; // all solid
    }

    auto ratio = raycast_aux(a, intersection, node->negChild.get());
    if(ratio < 1)
      return ratio * intersection_ratio; // blocked inside the negative side

    // raycast inside the positive side
    if(!node->posChild)
    {
      sandbox_text({0,1}, "all empty positive (2)");
      return 1;
    }

    return intersection_ratio + raycast_aux(intersection, b, node->posChild.get()) * (1 - intersection_ratio);
  }

  // The ray goes from the positive side to the negative side
  if(pa > +BspEpsilon && pb < +BspEpsilon)
  {
    const auto intersection = a + (b - a) * (pa / (pa - pb));
    if(node->posChild)
    {
      auto ratio = raycast_aux(a, intersection, node->posChild.get());
      if(ratio < 1)
        return ratio * (pa / (pa - pb));
    }
    else
    {
    }

    if(node->negChild)
    {
      auto ratio = raycast_aux(a, b, node->negChild.get());
      if(ratio < 1)
        return ratio;
    }
    else
    {
      return pa / (pa - pb);
    }

    return 1;
  }

  return 0.4;
}

float intersectRayWithHyperPlane(Vec2 rayStart, Vec2 rayDir, Vec2 hyperplaneNormal, float hyperplaneDist)
{
  // We want:
  // [1] dot_product(I - pointOnHp, hyperplaneNormal) = 0;
  // [2] I = rayStart + k * rayDir
  // Using [1]:
  // => dot_product(I, hyperplaneNormal) = dot_product(pointOnHp, hyperplaneNormal);
  // => dot_product(I, hyperplaneNormal) = hyperplaneDist;
  // Using [2]:
  // => dot_product(rayStart + k * rayDir, hyperplaneNormal) = hyperplaneDist;
  // => dot_product(rayStart, hyperplaneNormal) + k * dot_product(rayDir, hyperplaneNormal) = hyperplaneDist;
  // => k * dot_product(rayDir, hyperplaneNormal) = hyperplaneDist - dot_product(rayStart, hyperplaneNormal);
  //
  //        hyperplaneDist - dot_product(rayStart, hyperplaneNormal)
  // => k * --------------------------------------------------------
  //                  dot_product(rayDir, hyperplaneNormal)
  //
  const float num = hyperplaneDist - dot_product(rayStart, hyperplaneNormal);
  const float den = dot_product(rayDir, hyperplaneNormal);

  return num / den;
}

float sign(float a)
{
  if(a >= 0)
    return +1;
  else
    return -1;
}

void drawPolygon(const Polygon2f& poly, Color color)
{
  for(auto f : poly.faces)
    sandbox_line(poly.vertices[f.a], poly.vertices[f.b], color);
}

struct BspRaycast
{
  struct AlgoInput
  {
    Polygon2f polygon;
    std::unique_ptr<BspNode> bspRoot;
    Vec2 rayPos;
    Vec2 rayDir;
  };

  static AlgoInput generateInput()
  {
    Polygon2f poly = createRandomPolygon2f();

    {
      poly = {};
      const Vec2 center = {0, 0};
      const auto baseRadius = randomFloat(3, 7);
      const auto phase = 0;

      const int N = 3;
      for(int i = 0; i < N; ++i)
      {
        const auto radius = baseRadius;
        Vec2 pos = center;
        pos.x += cos((2 * M_PI * i) / N + phase) * radius;
        pos.y += sin((2 * M_PI * i) / N + phase) * radius;
        poly.vertices.push_back(pos);
        poly.faces.push_back({i, (i + 1) % N});
      }
    }

    AlgoInput input;
    input.polygon = poly;
    input.bspRoot = createBspTree(poly);
    input.rayPos = {-10, -4}; // randomPos({-30, -30}, {+30, +30});
    input.rayDir = {24, 2}; // randomPos({-30, -30}, {+30, +30});
    return input;
  }

  static float execute(const AlgoInput& input)
  {
    return raycast(input.rayPos, input.rayPos + input.rayDir, input.bspRoot.get());
  }

  static void display(const AlgoInput& input, float fraction)
  {
    std::vector<Hyperplane> clips;
    drawBspNode(input.bspRoot.get(), clips);

    drawPolygon(input.polygon, Yellow);

    sandbox_line(input.rayPos, input.rayPos + input.rayDir, Red);
    sandbox_circle(input.rayPos + input.rayDir, 0.2, Red);
    (void)fraction;
    //sandbox_line(input.rayPos, input.rayPos + input.rayDir * fraction, Green);
    //sandbox_circle(input.rayPos, 0.2, Green);
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

      if(dot_product(clipPlane.normal, rayDir) < 0)
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

    const auto beg = rayStart + rayDir * enter;
    const auto end = rayStart + rayDir * leave;

    sandbox_line(beg, end, White);

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

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<BspRaycast>>()); }
const int registered = registerApp("Bsp.Raycast", &create);
}
