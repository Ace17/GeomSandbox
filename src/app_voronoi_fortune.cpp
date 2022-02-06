// Copyright (C) 2022 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Voronoi diagram: Fortune algorithm

#include <vector>

#include "algorithm_app.h"
#include "bounding_box.h"
#include "random.h"

namespace
{

struct VoronoiDiagram
{
  struct Edge
  {
    int a, b;
  };

  std::vector<Vec2> vertices;
  std::vector<Edge> edges;
};

const BoundingBox box{{-20, -10}, {20, 10}};
struct FortuneVoronoiAlgoritm
{
  static std::vector<Vec2> generateInput()
  {
    std::vector<Vec2> r(15);

    for(auto& p : r)
      p = randomPos(box.min, box.max);

    return r;
  }

  static VoronoiDiagram execute(std::vector<Vec2> input)
  {
    // TODO
    return {};
  }

  static void drawInput(IDrawer* drawer, const std::vector<Vec2>& input)
  {
    drawer->rect(box.min, box.max - box.min);
    for(auto& p : input)
    {
      drawer->rect(p - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
    }
  }

  static void drawOutput(IDrawer* drawer, const std::vector<Vec2>& input, const VoronoiDiagram& output)
  {
    for(auto& edge : output.edges)
      drawer->line(output.vertices[edge.a], output.vertices[edge.b], Green);
  }
};

const int reg = registerApp("FortuneVoronoi", []() -> IApp* { return new AlgorithmApp<FortuneVoronoiAlgoritm>; });
}
