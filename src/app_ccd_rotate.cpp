// Copyright (C) 2026 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Ad-hoc continuous collision detection handling fast rotating bodies

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace
{

struct CcdRotateApp : IApp
{
  CcdRotateApp()
  {
    rotatingCircleRadius = 1.5;
    rotatingCircleRotationRadius = 6.0;
    rotatingCircleRotationCenter = Vec2(0, 0);
    rotatingCircleOmega = 0.45;

    translatingCircleRadius = 1.3;
    translatingCircleInitialPos = Vec2(7, 5);
    translatingCircleVelocity = Vec2(-1, -2);

    compute();
  }

  Vec2 dir(float angle) { return Vec2(cos(angle), sin(angle)); }

  float sqr(float x) { return x * x; }

  float sqr(Vec2 x) { return dotProduct(x, x); }

  Vec2 deltaBetweenCircles(float t)
  {
    const Vec2 rotatingCircleCenter =
          rotatingCircleRotationCenter + dir(rotatingCircleOmega * t) * rotatingCircleRotationRadius;
    const Vec2 translatingCircleCenter = translatingCircleInitialPos + translatingCircleVelocity * t;
    return rotatingCircleCenter - translatingCircleCenter;
  }

  // analytical first time derivative for 'deltaBetweenCircles'
  Vec2 deltaBetweenCircles_ddt(float t)
  {
    const Vec2 dRotatingCircleCenter = Vec2(rotatingCircleOmega * -sin(rotatingCircleOmega * t),
                                             rotatingCircleOmega * cos(rotatingCircleOmega * t)) *
          rotatingCircleRotationRadius;
    const Vec2 dTranslatingCircleCenter = translatingCircleVelocity;
    return dRotatingCircleCenter - dTranslatingCircleCenter;
  }

  float sqrDistBetweenCircles(float t)
  {
    return sqr(sqr(deltaBetweenCircles(t)) - sqr(rotatingCircleRadius + translatingCircleRadius));
  }

  // analytical first time derivative for 'sqrDistBetweenCircles'
  float sqrDistBetweenCircles_ddt(float t)
  {
    return 2 * (sqr(deltaBetweenCircles(t)) - sqr(rotatingCircleRadius + translatingCircleRadius)) * 2 *
          deltaBetweenCircles(t) * deltaBetweenCircles_ddt(t);
  }

  void draw(IDrawer* drawer) override
  {
    {
      const Vec2 rotatingCircleCenter =
            rotatingCircleRotationCenter + dir(rotatingCircleOmega * t) * rotatingCircleRotationRadius;
      drawer->circle(rotatingCircleCenter, rotatingCircleRadius, Green);
      drawCross(drawer, rotatingCircleCenter, Green);
    }

    {
      const Vec2 translatingCircleCenter = translatingCircleInitialPos + translatingCircleVelocity * t;
      drawer->circle(translatingCircleCenter, translatingCircleRadius, Green);
    }

    if(tCollision >= 0)
    {
      {
        const Vec2 rotatingCircleCenter =
              rotatingCircleRotationCenter + dir(rotatingCircleOmega * tCollision) * rotatingCircleRotationRadius;
        drawer->circle(rotatingCircleCenter, rotatingCircleRadius, Red);
        drawCross(drawer, rotatingCircleCenter, Red);
      }

      {
        const Vec2 translatingCircleCenter = translatingCircleInitialPos + translatingCircleVelocity * tCollision;
        drawer->circle(translatingCircleCenter, translatingCircleRadius, Red);
      }
    }

    drawCross(drawer, rotatingCircleRotationCenter, White);

    char buf[256];
    sprintf(buf, "t=%.2f, dist=%.2f, derivative=%.2f, tCollision=%.2f", t, sqrDistBetweenCircles(t),
          sqrDistBetweenCircles_ddt(t), tCollision);
    drawer->text({}, buf, White, {0, 4});
  }

  void drawCross(IDrawer* drawer, Vec2 pos, Color color)
  {
    drawer->line(pos - Vec2{1, 0}, pos + Vec2{1, 0}, color);
    drawer->line(pos - Vec2{0, 1}, pos + Vec2{0, 1}, color);
  }

  void processEvent(InputEvent inputEvent) override
  {
    if(inputEvent.pressed)
      keydown(inputEvent.key);
  }

  void keydown(Key key)
  {
    switch(key)
    {
    case Key::Left:
      t -= 0.03;
      break;
    case Key::Right:
      t += 0.03;
      break;
    case Key::PageUp:
      translatingCircleInitialPos.x += 1;
      break;
    case Key::PageDown:
      translatingCircleInitialPos.x -= 1;
      break;
    default:
      break;
    }

    compute();
  }

  void compute()
  {
    float x = 0;

    for(int k = 0; k < 10; ++k)
      x -= sqrDistBetweenCircles(x) / sqrDistBetweenCircles_ddt(x);

    if(sqrDistBetweenCircles(x) < 0.01)
      tCollision = x;
    else
      tCollision = -1;
  }

  float t = 0;
  float tCollision = 0;

  float rotatingCircleRadius;
  float rotatingCircleOmega; // rad/s
  Vec2 rotatingCircleRotationCenter;
  float rotatingCircleRotationRadius;

  float translatingCircleRadius;
  Vec2 translatingCircleInitialPos;
  Vec2 translatingCircleVelocity;
};

const int registered = registerApp("CollisionDetection/CCD_Rotate", []() -> IApp* { return new CcdRotateApp; });
}
