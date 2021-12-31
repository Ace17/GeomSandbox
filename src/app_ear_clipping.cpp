// Copyright (C) 2021 - Vivien Bonnet
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Polygon Triangulation: Ear-clipping Algorithm.

#include <cmath>
#include <vector>

#include "algorithm_app.h"
#include "random.h"
#include "random_polygon.h"

namespace
{
struct EarClippingAlgorithm
{
  struct Segment
  {
    int a, b;
  };

  static Polygon2f generateInput()
  {
    IVisualizer* visualizerForPolygon = nullptr;
    return createRandomPolygon2f(visualizerForPolygon);
  }

  static std::vector<Segment> execute(Polygon2f input)
  {
    // TODO
    return {};
  }

  static void drawInput(IDrawer* drawer, const Polygon2f& input)
  {
    for(Face face : input.faces)
      drawer->line(input.vertices[face.a], input.vertices[face.b]);
  }

  static void drawOutput(IDrawer* drawer, const Polygon2f& input, const std::vector<Segment>& output)
  {
    // TODO
  }
};

const int reg = registerApp("EarClipping", []() -> IApp* { return new AlgorithmApp<EarClippingAlgorithm>; });
}
