// Copyright (C) 2026 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Triangulation: Flip algorithm

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <algorithm>
#include <cassert>
#include <cstdio> // sprintf
#include <cstring>
#include <vector>

#include "random.h"
#include "triangulate_flip.h"

namespace
{
std::vector<Vec2> generateInput(int /*seed*/)
{
  std::vector<Vec2> r(100);

  for(auto& p : r)
    p = randomPos({-20, -10}, {20, 10});

  return r;
}

std::vector<Edge> execute(std::vector<Vec2> input)
{
  auto result = triangulate_Flip({input.size(), input.data()});
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

bool sameEdges(std::vector<Edge> a, span<const Edge> b)
{
  if(a.size() != b.len)
    return false;
  std::vector<Edge> c(b.begin(), b.end());
  auto byLexicographicalOrder = [](Edge p, Edge q)
  {
    if(p.a != q.a)
      return p.a < q.a;

    return p.b < q.b;
  };
  std::sort(c.begin(), c.end(), byLexicographicalOrder);
  return memcmp(a.data(), c.data(), sizeof(Edge) * a.size()) == 0;
}

const TestCase<std::vector<Vec2>, span<const Edge>> AllTestCases[] = {
      {
            "Basic",
            {
                  {0, 0},
                  {8, 0},
                  {0, 8},
            },
            [](const span<const Edge>& edges) {
              assert(sameEdges({{0, 1}, {1, 2}, {2, 0}}, edges));
            },
      },
};

BEGIN_ALGO("Triangulation/Points/Flip", execute)
WITH_INPUTGEN(generateInput)
WITH_TESTCASES(AllTestCases)
WITH_DISPLAY(display)
END_ALGO
} // namespace
