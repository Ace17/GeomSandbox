// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////

#include "core/algorithm_app2.h"
#include "core/sandbox.h"

#include <cmath>
#include <cstdio>
#include <vector>

#include "random.h"

std::vector<Vec2> loadPolygon(span<const uint8_t> data);

bool segmentsIntersect(Vec2 u0, Vec2 u1, Vec2 v0, Vec2 v1, Vec2& where);

namespace
{
float sqr(float v) { return v * v; }

float sqrMagnitude(Vec2 a) { return dotProduct(a, a); }

float det2d(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

// get the side of P relative to the oriented segment A-B
int classifySide(Vec2 a, Vec2 b, Vec2 p, float segmentThickness)
{
  const Vec2 n = rotateLeft(normalize(b - a));

  const float y = dotProduct(p - a, n);

  if(y < -segmentThickness)
    return -1;

  if(y > +segmentThickness)
    return +1;

  return 0;
}

// get the side of P relative to the oriented polyline A-B-C
// +1 for 'on the left'
//  0 for 'on A-B-C'
// -1 for 'on the right'
int classifySide(Vec2 a, Vec2 b, Vec2 c, Vec2 p, float segmentThickness)
{
  const int P_Side_AB = classifySide(a, b, p, segmentThickness);
  const int P_Side_BC = classifySide(b, c, p, segmentThickness);

  if(det2d(b - a, c - b) >= 0) // ABC makes a left-turn
  {
    if(P_Side_AB == -1 || P_Side_BC == -1)
      return -1; // we're on the right of ABC

    if(P_Side_AB == 0 || P_Side_BC == 0)
      return 0; // we're on ABC

    return 1; // we're on the left of ABC
  }
  else // ABC makes a right-turn
  {
    if(P_Side_AB == 1 || P_Side_BC == 1)
      return +1; // we're on the left of ABC
                 //
    if(P_Side_AB == 0 || P_Side_BC == 0)
      return 0; // we're on ABC

    return -1; // we're on the right of ABC
  }
}

struct Intersection
{
  Vec2 pos;
  int i, j; // segment start indices. By construction, we always have i<j.
};

struct Crossing
{
  Vec2 pos;
  int i, j; // segment start indices.
  float ti, tj; // positions on segment i and segment j
};

std::vector<Intersection> computeSelfIntersections(span<const Vec2> input)
{
  std::vector<Crossing> crossings;

  const float toleranceRadius = 0.001;

  const int N = input.len;

  for(int i = 0; i < N; ++i)
  {
    const Vec2 I0 = input[(i + 0) % N];
    const Vec2 I1 = input[(i + 1) % N];

    for(int j = i + 2; j < N && j < i + N - 1; ++j)
    {
      const Vec2 J0 = input[(j + 0) % N];
      const Vec2 J1 = input[(j + 1) % N];

      Vec2 where;

      bool isIntersection = segmentsIntersect(I0, I1, J0, J1, where);

      if(isIntersection && !(where == I1 || where == J1))
      {
        const float fractionI = dotProduct(where - I0, I1 - I0) / sqrMagnitude(I1 - I0);
        const float fractionJ = dotProduct(where - J0, J1 - J0) / sqrMagnitude(J1 - J0);

        crossings.push_back({where, i, j, fractionI, fractionJ});
      }

      {
        const auto colorI = isIntersection ? (where == I1 ? Orange : Red) : Green;
        const auto colorJ = isIntersection ? (where == J1 ? Orange : Red) : Green;
        sandbox_text(I0, "A", colorI, {-20, 20});
        sandbox_line(I0, I1, colorI);
        sandbox_text(J0, "B", colorJ, {-20, 20});
        sandbox_line(J0, J1, colorJ);
        sandbox_breakpoint();
      }
    }
  }

  // traverse all crossings, determine real intersections
  std::vector<Intersection> r;

  {
    for(auto c : crossings)
    {
      const auto X = c.pos;

      // compute prevPosI, nextPosI
      const bool isOnVertexI = sqrMagnitude(X - input[c.i]) < sqr(toleranceRadius);
      const Vec2 prevPosI = isOnVertexI ? input[(c.i - 1 + N) % N] : input[c.i];
      const Vec2 nextPosI = input[(c.i + 1) % N];

      // compute prevPosJ, nextPosJ
      const bool isOnVertexJ = sqrMagnitude(X - input[c.j]) < sqr(toleranceRadius);
      const Vec2 prevPosJ = isOnVertexJ ? input[(c.j - 1 + N) % N] : input[c.j];
      const Vec2 nextPosJ = input[(c.j + 1) % N];

      const int sidePrevPosJ = classifySide(prevPosI, X, nextPosI, prevPosJ, toleranceRadius);
      const int sideNextPosJ = classifySide(prevPosI, X, nextPosI, nextPosJ, toleranceRadius);

      bool mustPush = false;

      if(isOnVertexI != isOnVertexJ)
        mustPush = true;

      if(sidePrevPosJ == +1 || sideNextPosJ == +1)
        mustPush = true;

      if(mustPush)
        r.push_back({X, c.i, c.j});

      sandbox_circle(X, 0, Red, 5);
      sandbox_line(prevPosI, X, Green);
      sandbox_line(X, nextPosI, Green);
      sandbox_line(prevPosJ, X, Yellow);
      sandbox_line(X, nextPosJ, Yellow);
      char buf[256];
      sprintf(buf, "sidePrevPosJ=%d sideNextPosJ=%d", sidePrevPosJ, sideNextPosJ);
      sandbox_text({0, 11}, buf);
      sandbox_breakpoint();
    }
  }

  return r;
}

std::vector<Vec2> generateInput(int /*seed*/)
{
  std::vector<Vec2> points;
  const int N = 10;

  for(int i = 0; i < N; ++i)
  {
    Vec2 pos;
    pos.x = randomFloat(-20, 20);
    pos.y = randomFloat(-20, 20);
    if(points.size() >= 2 && rand() % 10 == 0)
    {
      auto p = points[rand() % points.size() - 1];
      points.push_back(p);
    }
    else
    {
      points.push_back(pos);
    }
  }

  return points;
}

std::vector<Intersection> execute(std::vector<Vec2> input) { return computeSelfIntersections(input); }

void display(span<const Vec2> input, span<const Intersection> output)
{
  for(int i = 0; i < (int)input.len; ++i)
  {
    char buf[256];
    sprintf(buf, "%d", i);
    sandbox_text(input[i], buf, White, Vec2(6, -6));
    sandbox_rect(input[i], {}, White, Vec2(6, 6));
  }

  for(int i = 0; i < (int)input.len; ++i)
    sandbox_line(input[i], input[(i + 1) % input.len]);

  int idx = 0;
  for(auto& point : output)
  {
    sandbox_circle(input[point.i], {}, Orange, 6);
    sandbox_circle(input[point.j], {}, Orange, 6);

    sandbox_circle(point.pos, {}, Red, 5);

    char buf[256];
    sprintf(buf, "I%d", idx);
    sandbox_text(point.pos, buf, Red, Vec2(6, +24));
    ++idx;
  }

  {
    char buf[256];
    sprintf(buf, "%d intersection(s)", (int)output.len);
    sandbox_text({0, 9}, buf, White);
  }
}

const TestCase<std::vector<Vec2>, span<const Intersection>> AllTestCases[] = {
      {
            "Lungs in pyramid",
            {
                  {0, -1},
                  {8, -1},
                  {4, 5},
                  {4, 3},
                  {5, 2},
                  {6, 0},
                  {5, 0},
                  {5, 2},
                  {4, 3},
                  {3, 2},
                  {3, 0},
                  {2, 0},
                  {3, 2},
                  {4, 3},
                  {4, 5},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },

      {
            "Two bridges, one lake (Railway)",
            {
                  {-3.8, 0.8},
                  {8.2, -4.8},
                  {9.4, 1.6},
                  {6.6, 1.2},
                  {3.6, -1.0},
                  {6.6, 1.2},
                  {3.2, 2},
                  {6.4, 2.6},
                  {6.6, 1.2},
                  {9.4, 1.6},
                  {9.4, 6.2},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },

      {
            "side change after coinciding border",
            {
                  {0, 0},
                  {4, 0},
                  {4, 4},
                  {0, 4},
                  {0, 2},
                  {0.5, 2},
                  {1.5, 2},
                  {2, 2},
                  {2, 3},
                  {3, 3},
                  {3, 1},
                  {2, 1},
                  {2, 2},
                  {1.5, 2},
                  {1.0, 2.5},
                  {0.5, 2},
                  {0, 2},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },

      {
            "saw, multiple intersections",
            {
                  {0, 0},
                  {10, 0},
                  {10, 2},
                  {1, 2},
                  {1, 3},
                  {1.5, 3},
                  {2, 2},
                  {2.5, 2},
                  {3, 3},
                  {4, 1},
                  {5, 3},
                  {5.8, 2},
                  {6.2, 2},
                  {6.5, 1},
                  {7, 2},
                  {7.3, 2},
                  {8, 3},
                  {8.5, 2},
                  {9, 3},
                  {10, 3},
                  {10, 4},
                  {0, 4},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },

      {
            "vertex/vertex contact, intersection",
            {
                  {-10, -10},
                  {0, -10},
                  {0, 0},
                  {0, +10},
                  {+10, +10},
                  {+10, 0},
                  {0, 0},
                  {-10, 0},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },
      {
            "vertex/vertex contact, no intersection",
            {
                  {-10, -10},
                  {0, 0},
                  {+10, -10},
                  {+10, 10},
                  {0, 0},
                  {-10, 10},
                  {-10, 1},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },
      {
            "twisted bridge",
            {
                  {-20, 0},
                  {-15, -5},
                  {-10, 0},
                  {0, 0},
                  {+10, 0},
                  {+15, 5},
                  {+20, 0},
                  {+15, -5},
                  {+10, 0},
                  {0, 0},
                  {-10, 0},
                  {-15, 5},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },
      {
            "bridge",
            {
                  {0, 0},
                  {9, 0},
                  {9, 6},
                  {6, 6},
                  {6, 3},
                  {3, 3},
                  {3, 9},
                  {6, 9},
                  {6, 6},
                  {9, 6},
                  {9, 12},
                  {0, 12},
                  {0, 1},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },
      {
            "vertex/segment contact",
            {
                  {-10, 0},
                  {+10, 0},
                  {+10, 5},
                  {0, 0},
                  {-10, 5},
                  {-10, 1},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },
      {
            "cross",
            {
                  {-10, -10},
                  {+10, +10},
                  {-10, +10},
                  {+10, -10},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },
      {
            "basic",
            {
                  {-10, 0},
                  {10, 0},
                  {10, -5},
                  {0, -5},
                  {0, 0},
                  {0, 5},
                  {-10, 5},
            },
            [](const span<const Intersection>&)
            {
              // todo
            },
      },
};

BEGIN_ALGO("Intersection/Polygon/SelfIntersection", execute);
WITH_INPUTGEN(generateInput)
WITH_LOADER(loadPolygon)
WITH_TESTCASES(AllTestCases)
WITH_DISPLAY(display)
END_ALGO
}
