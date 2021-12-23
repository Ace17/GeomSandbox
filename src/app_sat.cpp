// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Continuous collision detection using the separating axis test.

#include <cassert>
#include <cmath>

#include "app.h"
#include "drawer.h"
#include "geom.h"
#include "random.h"

namespace
{

float abs(float a) { return a >= 0 ? a : -a; }
float magnitude(Vec2 v) { return sqrt(v * v); }
Vec2 normalize(Vec2 v) { return v * (1.0 / magnitude(v)); }
Vec2 rotateLeft(Vec2 v) { return Vec2(-v.y, v.x); }

// The obstacle is an AABB, whose position and halfSize are given as parameters.
// The return value represents the allowed move, as a fraction of the desired move (delta).
float raycast(Vec2 pos, Vec2 delta, Vec2 obstaclePos, Vec2 obstacleHalfSize)
{
  const Vec2 axes[] = {
        {1, 0},
        {0, 1},
        rotateLeft(normalize(delta)),
  };

  float fraction = 0;

  for(auto axis : axes)
  {
    // make the move always increase the position along the axis
    if(axis * delta < 0)
      axis = axis * -1;

    const float obstacleExtent = fabs(obstacleHalfSize.x * axis.x) + fabs(obstacleHalfSize.y * axis.y);

    // compute projections on the axis
    const float startPos = pos * axis;
    const float targetPos = (pos + delta) * axis;
    const float obstacleMin = obstaclePos * axis - obstacleExtent;
    const float obstacleMax = obstaclePos * axis + obstacleExtent;

    assert(startPos <= targetPos || fabs(startPos - targetPos) < 0.0001);

    if(targetPos < obstacleMin)
      return 1; // all the axis-projected move is before the obstacle

    if(startPos >= obstacleMax)
      return 1; //  all the axis-projected move is after the obstacle

    if(fabs(startPos - targetPos) > 0.00001)
    {
      float f = (obstacleMin - startPos) / (targetPos - startPos);
      if(f > fraction)
        fraction = f;
    }
  }

  return fraction;
}

struct SeparatingAxisTestApp : IApp
{
  SeparatingAxisTestApp()
  {
    boxHalfSize = randomPos({2, 2}, {5, 5});
    boxStart = randomPos({-20, -10}, {20, 10});
    boxTarget = randomPos({-20, -10}, {20, 10});

    obstacleBoxPos = randomPos({-5, -5}, {5, 5});
    obstacleBoxHalfSize = randomPos({2, 2}, {5, 5});

    compute();
  }

  void draw(IDrawer* drawer) override
  {
    drawBox(drawer, boxStart, boxHalfSize, Green, "start");
    drawBox(drawer, boxTarget, boxHalfSize, Red, "target");
    drawBox(drawer, boxFinish, boxHalfSize, LightBlue, "finish");
    drawBox(drawer, obstacleBoxPos, obstacleBoxHalfSize, Yellow, "obstacle");

    drawer->line(boxStart, boxTarget, White);
  }

  void drawBox(IDrawer* drawer, Vec2 pos, Vec2 halfSize, Color color, const char* name)
  {
    drawer->rect(pos - halfSize, halfSize * 2, color);
    drawer->line(pos - Vec2{1, 0}, pos + Vec2{1, 0}, color);
    drawer->line(pos - Vec2{0, 1}, pos + Vec2{0, 1}, color);
    drawer->text(pos, name, color);
  }

  void keydown(Key key) override
  {
    switch(key)
    {
    case Key::Left:
      boxStart.x -= 1;
      break;
    case Key::Right:
      boxStart.x += 1;
      break;
    case Key::Up:
      boxStart.y += 1;
      break;
    case Key::Down:
      boxStart.y -= 1;
      break;
    }

    compute();
  }

  void compute()
  {
    const auto delta = Vec2(boxTarget - boxStart);
    const auto fraction = raycast(boxStart, delta, obstacleBoxPos, obstacleBoxHalfSize + boxHalfSize);
    boxFinish = boxStart + delta * fraction;
  }

  Vec2 boxHalfSize;

  Vec2 boxStart; // the starting position
  Vec2 boxTarget; // the target position
  Vec2 boxFinish; // the position we actually reach (=target if we don't hit the obstacle)

  Vec2 obstacleBoxPos;
  Vec2 obstacleBoxHalfSize;
};

const int registered = registerApp("SAT", []() -> IApp* { return new SeparatingAxisTestApp; });
}
