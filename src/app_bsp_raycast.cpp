// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Raycast against a BSP

#include "core/algorithm_app.h"
#include "core/bounding_box.h"
#include "core/geom.h"
#include "core/sandbox.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

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

  float ratio = 0;

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
      sandbox_text((beg + end) * 0.5, buf);
      sandbox_circle(beg, 0.2, color);
      sandbox_circle(end, 0.2, color);
    }

    auto curr = stack.back();
    stack.pop_back();

    const auto beg = lerp(a, b, curr.beg);
    const auto end = lerp(a, b, curr.end);

    if(!curr.node)
    {
      if(curr.solid)
      {
        // the ray hit a solid leaf and can't continue further
        sandbox_text({0, 1}, "solid leaf");
        break;
      }

      sandbox_text({0, 1}, "empty leaf");
      ratio = curr.end;
    }
    else
    {
      const auto proj_beg = dotProduct(beg, curr.node->plane.normal) - curr.node->plane.dist;
      const auto proj_end = dotProduct(end, curr.node->plane.normal) - curr.node->plane.dist;

      const auto proj_a = dotProduct(a, curr.node->plane.normal) - curr.node->plane.dist;
      const auto proj_b = dotProduct(b, curr.node->plane.normal) - curr.node->plane.dist;

      const auto pmid = proj_a / (proj_a - proj_b);

      if(proj_beg < +BspEpsilon && proj_end >= +BspEpsilon)
      {
        stack.push_back(Chunk{pmid, curr.end, curr.node->posChild.get(), false});
        stack.push_back(Chunk{curr.beg, pmid, curr.node->negChild.get(), true});
        sandbox_text({0, 1}, "crossed (neg to pos)");
      }
      else if(proj_beg >= +BspEpsilon && proj_end < +BspEpsilon)
      {
        stack.push_back(Chunk{pmid, curr.end, curr.node->negChild.get(), true});
        stack.push_back(Chunk{curr.beg, pmid, curr.node->posChild.get(), false});
        sandbox_text({0, 1}, "crossed (pos to neg)");
      }
      else if(proj_beg >= +BspEpsilon && proj_end >= +BspEpsilon)
      {
        stack.push_back(Chunk{curr.beg, curr.end, curr.node->posChild.get(), false});
        sandbox_text({0, 1}, "all positive");
      }
      else
      {
        stack.push_back(Chunk{curr.beg, curr.end, curr.node->negChild.get(), true});
        sandbox_text({0, 1}, "all negative");
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

    sandbox_circle(a, 0.3, Yellow);
    sandbox_circle(lerp(a, b, ratio), 0.3, Yellow);
    sandbox_breakpoint();
  }

  return ratio;
}

struct BspRaycast
{
  struct AlgoInput
  {
    Polygon2f polygon;
    Vec2 rayPos;
    Vec2 rayDir;
  };

  static AlgoInput generateInput()
  {
    Polygon2f poly = createRandomPolygon2f();

    for(auto& face : poly.faces)
      std::swap(face.a, face.b);

    if(0)
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
    input.rayPos = randomPos({-20, -2}, {-15, +2});
    input.rayDir = randomPos({+10, -10}, {+10, +10}) - input.rayPos;
    return input;
  }

  static float execute(const AlgoInput& input)
  {
    std::unique_ptr<const BspNode> bspRoot = createBspTree(input.polygon);
    return raycast(input.rayPos, input.rayPos + input.rayDir, bspRoot.get());
  }

  static void display(const AlgoInput& input, float fraction)
  {
    for(auto f : input.polygon.faces)
      sandbox_line(input.polygon.vertices[f.a], input.polygon.vertices[f.b], Yellow);

    sandbox_line(input.rayPos, input.rayPos + input.rayDir, Red);
    sandbox_circle(input.rayPos + input.rayDir, 0.2, Red);

    sandbox_line(input.rayPos, input.rayPos + input.rayDir * fraction, Green);
    sandbox_circle(input.rayPos, 0.2, Green);
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<BspRaycast>>()); }
const int registered = registerApp("SpatialPartitioning/Bsp/Raycast", &create);
}
