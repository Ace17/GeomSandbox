// Copyright (C) 2023 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Catmull-Rom Spline computation

#include "core/app.h"
#include "core/drawer.h"

#include <cmath>
#include <cstdio> // sprintf
#include <vector>

#include "random.h"

namespace
{
struct CatmullRomSpline : IApp
{
  CatmullRomSpline()
  {
    // generate a closed non-self-intersecting polyline
    const int N = rand() % 8 + 4;
    const float extent = 15;
    for(int k = 0; k < N; ++k)
    {
      const float radius = randomFloat(3, extent);
      Vec2 pos;
      pos.x = cos(k * M_PI * 2 / N) * radius;
      pos.y = sin(k * M_PI * 2 / N) * radius;
      controlPoints.push_back(pos);
    }
  }

  void draw(IDrawer* drawer) override
  {
    const int N = (int)controlPoints.size();

    for(int i = 0; i < (int)controlPoints.size(); ++i)
    {
      auto p0 = controlPoints[(i - 1 + N) % N];
      auto p1 = controlPoints[(i + 0 + N) % N];
      auto p2 = controlPoints[(i + 1 + N) % N];
      auto p3 = controlPoints[(i + 2 + N) % N];

      const float t0 = 0.0f;
      const float t1 = t0 + pow(magnitude(p0 - p1), alpha);
      const float t2 = t1 + pow(magnitude(p1 - p2), alpha);
      const float t3 = t2 + pow(magnitude(p2 - p3), alpha);

      const Vec2 v0 = tension * (t2 - t1) * ((p1 - p0) / (t1 - t0) - (p2 - p0) / (t2 - t0) + (p2 - p1) / (t2 - t1));
      const Vec2 v1 = tension * (t2 - t1) * ((p2 - p1) / (t2 - t1) - (p3 - p1) / (t3 - t1) + (p3 - p2) / (t3 - t2));

      // We want to define the following curve:
      // p(t) = at^3 + bt^2 + ct + d
      // where t belongs to [0;1].
      //
      // The derivative of this curve is:
      // p'(t) = 3at^2 + 2bt + c
      //
      // We want this curve to meet the following constraints:
      //
      // | p(0)  = p1
      // | p(1)  = p2
      // | p'(0) = v0
      // | p'(1) = v1
      //
      // Which implies:
      // | d             = p1
      // | a + b + c + d = p2
      // | c             = v0
      // | 3a + 2b + c   = v1
      //
      // Solving for a,b,c,d:
      // | a =  2p1 + 2p2 +  v0 + v1
      // | b = -3p1 + 3p2 - 2v0 - v1
      // | c = v0
      // | d = p1

      const Vec2 a = 2.0f * (p1 - p2) + v0 + v1;
      const Vec2 b = -3.0f * (p1 - p2) - 2 * v0 - v1;
      const Vec2 c = v0;
      const Vec2 d = p1;

      for(float t = 0; t < 1.0; t += 0.05)
      {
        const Vec2 v = a * t * t * t + b * t * t + c * t + d;

        const float size = 0.1;
        drawer->rect(v - Vec2(size, size), 2 * Vec2(size, size), White);
      }
    }

    for(int i = 0; i < N; ++i)
      drawer->line(controlPoints[i], controlPoints[(i + 1) % N], Red);

    for(auto& cp : controlPoints)
    {
      auto i = int(&cp - controlPoints.data());
      const float size = 0.3;
      drawer->rect(cp - Vec2(size, size), 2 * Vec2(size, size), i == index ? Green : Yellow);
    }

    {
      char buf[256];
      sprintf(buf, "alpha=%.2f tension=%.2f", alpha, tension);
      drawer->text({}, buf);
    }
  }

  void processEvent(InputEvent event) override
  {
    if(event.pressed)
      keydown(event.key);
  }

  void keydown(Key key)
  {
    const float speed = 0.1;
    switch(key)
    {
    case Key::Space:
      index++;
      break;
    case Key::Home:
      alpha -= 0.01;
      break;
    case Key::End:
      alpha += 0.01;
      break;
    case Key::PageUp:
      tension -= 0.01;
      break;
    case Key::PageDown:
      tension += 0.01;
      break;
    case Key::Left:
      controlPoints[index].x -= speed;
      break;
    case Key::Right:
      controlPoints[index].x += speed;
      break;
    case Key::Up:
      controlPoints[index].y += speed;
      break;
    case Key::Down:
      controlPoints[index].y -= speed;
      break;
    default:
      break;
    }

    index = (index + controlPoints.size()) % controlPoints.size();
  }

  std::vector<Vec2> controlPoints;
  int index = 0;
  float alpha = 0.5;
  float tension = 1.0;
};

const int registered = registerApp("App.Spline.CatmullRom", []() -> IApp* { return new CatmullRomSpline; });
}
