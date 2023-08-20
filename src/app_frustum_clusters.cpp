// Copyright (C) 2023 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Assignation of spheres to frustum clusters

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cmath>

namespace
{
float lerp(float a, float b, float r) { return (a * (1.f - r)) + (b * r); }

const int CX = 16;
const int CY = 8;
const int CZ = 24;

struct FrustumClusters : IApp
{
  // input data:

  // frustum definition
  static constexpr float near = 10;
  static constexpr float far = 40;
  static constexpr float aspectRatio = 16.0 / 9.0;
  static constexpr float fovY = M_PI / 3.0;

  // sphere position
  Vec3 sphereCenter = {4, 0, -5};
  float sphereRadius = 7;

  // output data:

  // intermediate
  Vec3 planesX[CX + 1];
  Vec3 planesY[CY + 1];

  // result
  bool clusters[CZ][CY][CX]{};

  FrustumClusters() { recompute(); }

  void recompute()
  {
    for(auto& slice : clusters)
      for(auto& row : slice)
        for(auto& cell : row)
          cell = false;

    // compute the normals of the planes splitting the frustum.
    // all those planes contain (0;0;0)
    const float tanHalfFovY = tan(fovY / 2);

    // pencil of planes on the Y axis
    const float fhx = far * tanHalfFovY * aspectRatio;

    for(int ix = 0; ix <= CX; ++ix)
    {
      const auto rx = float(ix) / float(CX);
      Vec3 ray{lerp(-fhx, +fhx, rx), 0, -far};
      planesX[ix] = normalize(crossProduct(ray, Vec3(0, 1, 0)));
    }

    // pencil of planes on the X axis
    const float fhy = far * tanHalfFovY;

    for(int iy = 0; iy <= CY; ++iy)
    {
      const auto ry = float(iy) / float(CY);
      Vec3 ray{0, lerp(-fhy, +fhy, ry), -far};
      planesY[iy] = normalize(crossProduct(ray, Vec3(-1, 0, 0)));
    }

    // determine which planes are the tightest boundaries for the sphere
    int minPlaneX, maxPlaneX;
    {
      minPlaneX = 0;
      maxPlaneX = CX;
      for(int ix = 0; ix <= CX; ++ix)
      {
        if(dotProduct(planesX[ix], sphereCenter) > sphereRadius)
          minPlaneX = max(minPlaneX, ix);
        if(dotProduct(planesX[ix], sphereCenter) < -sphereRadius)
          maxPlaneX = min(maxPlaneX, ix);
      }
    }

    int minPlaneY, maxPlaneY;
    {
      minPlaneY = 0;
      maxPlaneY = CY;
      for(int iy = 0; iy <= CY; ++iy)
      {
        if(dotProduct(planesY[iy], sphereCenter) > sphereRadius)
          minPlaneY = max(minPlaneY, iy);
        if(dotProduct(planesY[iy], sphereCenter) < -sphereRadius)
          maxPlaneY = min(maxPlaneY, iy);
      }
    }

    const int minPlaneZ = max(0, (-sphereCenter.z - sphereRadius - near) * CZ / (far - near));
    const int maxPlaneZ = min(CZ, (-sphereCenter.z + sphereRadius - near) * CZ / (far - near) + 1);

    for(int z = minPlaneZ; z < maxPlaneZ; ++z)
      for(int y = minPlaneY; y < maxPlaneY; ++y)
        for(int x = minPlaneX; x < maxPlaneX; ++x)
          clusters[z][y][x] = true;
  }

  void keydown(Key key)
  {
    const float speed = 0.5f;
    switch(key)
    {
    case Key::Left:
      sphereCenter.x -= speed;
      break;
    case Key::Right:
      sphereCenter.x += speed;
      break;
    case Key::Up:
      sphereCenter.z -= speed;
      break;
    case Key::Down:
      sphereCenter.z += speed;
      break;
    case Key::PageUp:
      sphereRadius *= 1.10;
      break;
    case Key::PageDown:
      sphereRadius *= (1.0 / 1.10);
      break;
    default:
      break;
    }
    recompute();
  }

  void draw(IDrawer* drawer) override
  {
    struct txform : Vec3
    {
      txform(float x, float y, float z)
          : Vec3(x, -z - 10, y)
      {
      }
    };

    drawer->line(txform{0, 0, 0}, txform{1, 0, 0}, Red);
    drawer->line(txform{0, 0, 0}, txform{0, 1, 0}, Green);
    drawer->line(txform{0, 0, 0}, txform{0, 0, 1}, Blue);

    const float tanHalfFovY = tan(fovY / 2);

    // near half x, near half y
    const float nhx = near * tanHalfFovY * aspectRatio;
    const float nhy = near * tanHalfFovY;

    const float fhx = far * tanHalfFovY * aspectRatio;
    const float fhy = far * tanHalfFovY;

    constexpr Color AlphaGray{1, 1, 1, 0.2};

    drawer->line(txform{0, 0, 0}, txform{-nhx, +nhy, -near}, AlphaGray);
    drawer->line(txform{0, 0, 0}, txform{+nhx, +nhy, -near}, AlphaGray);
    drawer->line(txform{0, 0, 0}, txform{+nhx, -nhy, -near}, AlphaGray);
    drawer->line(txform{0, 0, 0}, txform{-nhx, -nhy, -near}, AlphaGray);

    drawer->line(txform{-nhx, -nhy, -near}, txform{-nhx, +nhy, -near}, Yellow);
    drawer->line(txform{-nhx, +nhy, -near}, txform{+nhx, +nhy, -near}, Yellow);
    drawer->line(txform{+nhx, +nhy, -near}, txform{+nhx, -nhy, -near}, Yellow);
    drawer->line(txform{+nhx, -nhy, -near}, txform{-nhx, -nhy, -near}, Yellow);

    drawer->line(txform{-fhx, -fhy, -far}, txform{-fhx, +fhy, -far}, Yellow);
    drawer->line(txform{-fhx, +fhy, -far}, txform{+fhx, +fhy, -far}, Yellow);
    drawer->line(txform{+fhx, +fhy, -far}, txform{+fhx, -fhy, -far}, Yellow);
    drawer->line(txform{+fhx, -fhy, -far}, txform{-fhx, -fhy, -far}, Yellow);

    drawer->line(txform{-nhx, -nhy, -near}, txform{-fhx, -fhy, -far}, Yellow);
    drawer->line(txform{-nhx, +nhy, -near}, txform{-fhx, +fhy, -far}, Yellow);
    drawer->line(txform{+nhx, +nhy, -near}, txform{+fhx, +fhy, -far}, Yellow);
    drawer->line(txform{+nhx, -nhy, -near}, txform{+fhx, -fhy, -far}, Yellow);

    for(int z = 0; z < CZ; ++z)
    {
      const auto rz0 = float(z + 0) / float(CZ);
      const auto rz1 = float(z + 1) / float(CZ);

      const auto nz0 = lerp(-near, -far, rz0);
      const auto nz1 = lerp(-near, -far, rz1);

      const float nhx = -nz0 * tanHalfFovY * aspectRatio;
      const float nhy = -nz0 * tanHalfFovY;
      const float fhx = -nz1 * tanHalfFovY * aspectRatio;
      const float fhy = -nz1 * tanHalfFovY;

      for(int x = 0; x < CX; ++x)
      {
        const auto rx0 = float(x + 0) / float(CX);
        const auto rx1 = float(x + 1) / float(CX);

        const auto nx0 = lerp(-nhx, +nhx, rx0);
        const auto nx1 = lerp(-nhx, +nhx, rx1);
        const auto fx0 = lerp(-fhx, +fhx, rx0);
        const auto fx1 = lerp(-fhx, +fhx, rx1);

        for(int y = 0; y < CY; ++y)
        {
          const auto ry0 = float(y + 0) / float(CY);
          const auto ry1 = float(y + 1) / float(CY);

          const auto ny0 = lerp(-nhy, +nhy, ry0);
          const auto ny1 = lerp(-nhy, +nhy, ry1);
          const auto fy0 = lerp(-fhy, +fhy, ry0);
          const auto fy1 = lerp(-fhy, +fhy, ry1);

          constexpr Color VeryLightGray{0.2, 0.2, 0.2, 0.2};
          const auto color = clusters[z][y][x] ? Yellow : VeryLightGray;

          drawer->line(txform{nx0, ny0, nz0}, txform{nx0, ny1, nz0}, color);
          drawer->line(txform{nx0, ny1, nz0}, txform{nx1, ny1, nz0}, color);
          drawer->line(txform{nx1, ny1, nz0}, txform{nx1, ny0, nz0}, color);
          drawer->line(txform{nx1, ny0, nz0}, txform{nx0, ny0, nz0}, color);

          drawer->line(txform{fx0, fy0, nz1}, txform{fx0, fy1, nz1}, color);
          drawer->line(txform{fx0, fy1, nz1}, txform{fx1, fy1, nz1}, color);
          drawer->line(txform{fx1, fy1, nz1}, txform{fx1, fy0, nz1}, color);
          drawer->line(txform{fx1, fy0, nz1}, txform{fx0, fy0, nz1}, color);

          drawer->line(txform{nx0, ny0, nz0}, txform{fx0, fy0, nz1}, color);
          drawer->line(txform{nx0, ny1, nz0}, txform{fx0, fy1, nz1}, color);
          drawer->line(txform{nx1, ny1, nz0}, txform{fx1, fy1, nz1}, color);
          drawer->line(txform{nx1, ny0, nz0}, txform{fx1, fy0, nz1}, color);
        }
      }
    }

    {
      auto C = txform{sphereCenter.x, sphereCenter.y, sphereCenter.z};
      drawer->circle({C.x, C.y}, sphereRadius);
    }
  }
};

const int registered = registerApp("App.FrustumClusters", []() -> IApp* { return new FrustumClusters; });
}
