// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Triangulation: Bowyer-Watson algorithm
// This is the testbed glue for the algorithm.

#include "core/algorithm_app.h"
#include "core/sandbox.h"

#include <cstdio> // sprintf
#include <vector>

#include "random.h"
#include "triangulate_bowyerwatson.h"

template<>
std::vector<Vec2> deserialize<std::vector<Vec2>>(span<const uint8_t> data);

namespace
{
struct BowyerWatsonTriangulationAlgorithm
{
  static std::vector<Vec2> generateInput()
  {
    std::vector<Vec2> r(15);

    for(auto& p : r)
      p = randomPos({-20, -10}, {20, 10});

    return r;
  }

  static std::vector<Edge> execute(std::vector<Vec2> input)
  {
    auto result = triangulate_BowyerWatson({input.size(), input.data()});
    sandbox_printf("Triangulated, %d edges\n", (int)result.size());
    return result;
  }

  static void display(span<const Vec2> input, span<const Edge> output)
  {
    int idx = 0;

    for(auto& p : input)
    {
      sandbox_rect(p - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
      char buffer[16];
      sprintf(buffer, "%d", idx);
      sandbox_text(p + Vec2(0.3, 0), buffer, Red);
      idx++;
    }

    for(auto& edge : output)
      sandbox_line(input[edge.a], input[edge.b], Green);
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<BowyerWatsonTriangulationAlgorithm>>()); }
const int reg = registerApp("Triangulation/Points/BowyerWatson", &create);
}
