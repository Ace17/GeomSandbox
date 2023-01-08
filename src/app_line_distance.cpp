// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// interactive app: distance between two 3D lines

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cmath>

#include "random.h"

namespace
{

Vec3 randomPosition()
{
  Vec3 r;
  r.x = randomFloat(-10, 10);
  r.y = randomFloat(-10, 10);
  r.z = randomFloat(-10, 10);
  return r;
}

struct LineDistance : IApp
{
  LineDistance()
  {
    P1 = randomPosition();
    P2 = randomPosition();
    Q1 = randomPosition();
    Q2 = randomPosition();

    s = 0.5;
    t = 0.5;
  }

  void draw(IDrawer* drawer) override
  {
    drawer->line(Vec3{0, 0, 0}, Vec3{1, 0, 0}, Red);
    drawer->line(Vec3{0, 0, 0}, Vec3{0, 1, 0}, Green);
    drawer->line(Vec3{0, 0, 0}, Vec3{0, 0, 1}, LightBlue);

    auto drawInfiniteLine = [](IDrawer* drawer, Vec3 a, Vec3 b)
    {
      auto tangent = normalize(a - b);
      drawer->line(a + tangent * -100, a + tangent * +100, White);
    };

    drawInfiniteLine(drawer, P1, P2);
    drawInfiniteLine(drawer, Q1, Q2);

    solve();

    Vec3 I = P1 + (P2 - P1) * s;
    Vec3 J = Q1 + (Q2 - Q1) * t;
    drawer->line(I, J, Red);
  }

  void solve()
  {
    auto P1P2 = P2 - P1;
    auto Q1Q2 = Q2 - Q1;
    auto Q1P1 = P1 - Q1;

    // Let's first parameterize both lines:
    //
    // - I(s) a point on the line P1P2
    //    I = P1 + s * P1P2
    //
    // - J(t) a point on the line P1P2
    //    J = Q1 + t * Q1Q2
    //
    // Let's define the squared distance between these two points:
    //    R(s, t) = || I(s) - J(t) ||^2
    //
    // We're searching for (s,t) so that R(s, t) is minimal.
    //
    // R(s, t) = || I(s) - J(t) ||^2
    //         = || P1 + s * P1P2 - Q1 - t * Q1Q2 ||^2
    //         = || Q1P1 + (s * P1P2 - t * Q1Q2) ||^2
    //         = Q1P1^2 + 2 * Q1P1 * ( s * P1P2 - t * Q1Q2 ) + (sP1P2 - tQ1Q2)^2
    //         = Q1P2^2 + [2*Q1P1.P1P2]*s + [-2*Q1P1.Q1Q2]*t + [P2P2^2]*s^2 + [Q1Q2^2} * t^2 + [2P1P2.Q1Q2] * s * t
    //
    // Let's diffentiate R(s, t) according to s and t:
    //
    // dR(s,t)
    // ------- = 2*Q1P1.P1P2 + [-2P1P2.Q1Q2]*t + [2 * P1P2^2]*s
    //   ds
    //
    // dR(s,t)
    // ------- = -2*Q1P1.Q1Q2 + [-2P1P2.Q1Q2]*s + [2 * Q1Q2^2]*t
    //   dt
    //
    // To simplify things a bit, let's define some constants:
    //
    //    A = 2 * P1P2^2
    //    B = 2 * Q1Q2^2
    //    C = - 2 * P1P2.Q1Q2
    //    D = - 2 * Q1P1.P1P2
    //    E = 2 * Q1P1.Q1Q2
    const auto A = 2 * dotProduct(P1P2, P1P2);
    const auto B = 2 * dotProduct(Q1Q2, Q1Q2);
    const auto C = -2 * dotProduct(P1P2, Q1Q2);
    const auto D = -2 * dotProduct(Q1P1, P1P2);
    const auto E = 2 * dotProduct(Q1P1, Q1Q2);

    // So now, we have:
    //
    //    dR(s,t)
    //    ------- = - D + A * s + C * t
    //       ds
    //
    //    dR(s,t)
    //    ------- = - E + C * s + B * t
    //       dt
    //
    // Let's force both derivatives to be zero, we get two equations with
    // two unknowns.
    //
    //   A * s + C * t = D
    //   C * s + B * t = E

    //|  So we need to solve the following matrix equation:
    //|
    //|  | A C |    | s |   | D |
    //|  | C B | *  | t | = | E |

    // Let's use Kramer's rule:
    const auto determinant = A * B - C * C;

    s = (D * B - E * C) / determinant;
    t = (A * E - C * D) / determinant;
  }

  Vec3 P1, P2; // line P
  Vec3 Q1, Q2; // line Q

  // solution
  float s, t;
};

const int registered = registerApp("App.LineDistance", []() -> IApp* { return new LineDistance; });
}
