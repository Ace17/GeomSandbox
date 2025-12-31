// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Triangulation: Bowyer-Watson algorithm
// This is the testbed glue for the algorithm.

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <cstdio> // sprintf
#include <vector>

#include "random.h"
#include "triangulate_bowyerwatson.h"

namespace
{
std::vector<Vec2> generateInput(int /*seed*/)
{
  std::vector<Vec2> r(15);

  for(auto& p : r)
    p = randomPos({-20, -10}, {20, 10});

  return r;
}

std::vector<Edge> execute(std::vector<Vec2> input)
{
  auto result = triangulate_BowyerWatson({input.size(), input.data()});
  sandbox_printf("Triangulated, %d edges\n", (int)result.size());
  return result;
}

void display(span<const Vec2> input, span<const Edge> output)
{
  for(auto& p : input)
    sandbox_rect(p, {}, White, Vec2(4, 4));

  for(auto& edge : output)
    sandbox_line(input[edge.a], input[edge.b], Green);
}

BEGIN_ALGO("Triangulation/Points/BowyerWatson", execute)
WITH_INPUTGEN(generateInput)
WITH_DISPLAY(display)
END_ALGO
}
