// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Random polygon generation

#include "algorithm_app.h"
#include "random_polygon.h"

namespace
{

struct RandomPolygon
{
  static int generateInput() { return 0; }

  static Polygon2f execute(int input) { return createRandomPolygon2f(gVisualizer); }

  static void drawInput(IDrawer* drawer, const int& input)
  {
    // no input to draw
    drawer->line({-1, 0}, {1, 0});
    drawer->line({0, -1}, {0, 1});
  }

  static void drawOutput(IDrawer* drawer, const int& input, const Polygon2f& output)
  {
    for(auto face : output.faces)
      drawer->line(output.vertices[face.a], output.vertices[face.b], Green);
  }
};

const int reg = registerApp("RandomPolygon", []() -> IApp* { return new AlgorithmApp<RandomPolygon>; });
}
