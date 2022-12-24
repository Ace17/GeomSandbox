// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Example 3D interactive app: spinning cube

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cmath>

namespace
{

struct SpinningCube : IApp
{
  void draw(IDrawer* drawer) override
  {
    drawer->line(Vec3{0, 0, 0}, Vec3{1, 0, 0}, Red);
    drawer->line(Vec3{0, 0, 0}, Vec3{0, 1, 0}, Green);
    drawer->line(Vec3{0, 0, 0}, Vec3{0, 0, 1}, Blue);

    Vec3 vertices[] = {
          {-1, -1, -1},
          {-1, +1, -1},
          {+1, +1, -1},
          {+1, -1, -1},
          {-1, -1, +1},
          {-1, +1, +1},
          {+1, +1, +1},
          {+1, -1, +1},
    };

    for(auto& v : vertices)
    {
      Vec3 orig = v;
      v.z = orig.z * cos(angle) - orig.x * sin(angle);
      v.x = orig.z * sin(angle) + orig.x * cos(angle);

      const auto angle2 = angle * 0.5;

      orig = v;
      v.x = orig.x * cos(angle2) - orig.y * sin(angle2);
      v.y = orig.x * sin(angle2) + orig.y * cos(angle2);

      v = v * 5;
    }

    drawer->line(vertices[0], vertices[1], White);
    drawer->line(vertices[1], vertices[2], White);
    drawer->line(vertices[2], vertices[3], White);
    drawer->line(vertices[3], vertices[0], White);

    drawer->line(vertices[4], vertices[5], White);
    drawer->line(vertices[5], vertices[6], White);
    drawer->line(vertices[6], vertices[7], White);
    drawer->line(vertices[7], vertices[4], White);

    drawer->line(vertices[0], vertices[4], White);
    drawer->line(vertices[1], vertices[5], White);
    drawer->line(vertices[2], vertices[6], White);
    drawer->line(vertices[3], vertices[7], White);
  }

  float angle = 0;

  void tick() override
  {
    angle += 0.01;
    if(angle > 2 * M_PI)
      angle -= 2 * M_PI;
  }
};

const int registered = registerApp("App.Cube", []() -> IApp* { return new SpinningCube; });
}
