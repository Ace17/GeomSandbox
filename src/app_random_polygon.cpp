// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Random polygon generation

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include "random_polygon.h"

namespace
{

int generateInput(int /*seed*/) { return 0; }

Polygon2f execute(int /*dummy*/) { return createRandomPolygon2f(); }

void display(int /*input*/, const Polygon2f& output)
{
  // no input to draw
  sandbox_line({-1, 0}, {1, 0});
  sandbox_line({0, -1}, {0, 1});

  for(auto face : output.faces)
    sandbox_line(output.vertices[face.a], output.vertices[face.b], Green);
}

BEGIN_ALGO("Random/Polygon", execute)
WITH_INPUTGEN(generateInput)
WITH_DISPLAY(display)
END_ALGO
}
