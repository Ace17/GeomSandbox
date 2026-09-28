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

Vec2 rotate(Vec2 f, float angle)
{
  Vec2 r;
  r.x = cos(angle) * f.x - sin(angle) * f.y;
  r.y = sin(angle) * f.x + cos(angle) * f.y;
  return r;
}

struct InitialConditions
{
  float rotatingCircleRadius;
  float rotatingCircleOmega; // rad/s
  Vec2 rotatingCircleRotationCenter;
  float rotatingCircleRotationRadius;

  float rotatingSegmentPosBase;
  float rotatingSegmentPosTip;

  float translatingCircleRadius;
  Vec2 translatingCircleInitialPos;
  Vec2 translatingCircleVelocity;
};

struct SpatialConfiguration
{
  Vec2 rotatingCircleCenter;
  Vec2 rotatingSegmentA;
  Vec2 rotatingSegmentB;
  Vec2 translatingCircleCenter;
};

Vec2 dir(float angle) { return Vec2(cos(angle), sin(angle)); }

SpatialConfiguration computeConfiguration(const InitialConditions& ic, float t)
{
  SpatialConfiguration sc;

  sc.rotatingCircleCenter =
        ic.rotatingCircleRotationCenter + dir(ic.rotatingCircleOmega * t) * ic.rotatingCircleRotationRadius;
  sc.rotatingSegmentA = rotate(Vec2(0, ic.rotatingSegmentPosBase), ic.rotatingCircleOmega * t);
  sc.rotatingSegmentB =
        rotate(Vec2(ic.rotatingCircleRotationRadius, ic.rotatingSegmentPosTip), ic.rotatingCircleOmega * t);
  sc.translatingCircleCenter = ic.translatingCircleInitialPos + ic.translatingCircleVelocity * t;

  return sc;
}

float sqr(float x) { return x * x; }

float sqr(Vec2 x) { return dotProduct(x, x); }

Vec2 deltaBetweenCircles(const InitialConditions& ic, float t)
{
  const Vec2 rotatingCircleCenter =
        ic.rotatingCircleRotationCenter + dir(ic.rotatingCircleOmega * t) * ic.rotatingCircleRotationRadius;
  const Vec2 translatingCircleCenter = ic.translatingCircleInitialPos + ic.translatingCircleVelocity * t;
  return rotatingCircleCenter - translatingCircleCenter;
}

// analytical first time derivative for 'deltaBetweenCircles'
Vec2 deltaBetweenCircles_ddt(const InitialConditions& ic, float t)
{
  const Vec2 dRotatingCircleCenter = Vec2(ic.rotatingCircleOmega * -sin(ic.rotatingCircleOmega * t),
                                           ic.rotatingCircleOmega * cos(ic.rotatingCircleOmega * t)) *
        ic.rotatingCircleRotationRadius;
  const Vec2 dTranslatingCircleCenter = ic.translatingCircleVelocity;
  return dRotatingCircleCenter - dTranslatingCircleCenter;
}

float sqrDistBetweenCircles(const InitialConditions& ic, float t)
{
  return sqr(sqr(deltaBetweenCircles(ic, t)) - sqr(ic.rotatingCircleRadius + ic.translatingCircleRadius));
}

// analytical first time derivative for 'sqrDistBetweenCircles'
float sqrDistBetweenCircles_ddt(const InitialConditions& ic, float t)
{
  return 2 * (sqr(deltaBetweenCircles(ic, t)) - sqr(ic.rotatingCircleRadius + ic.translatingCircleRadius)) * 2 *
        deltaBetweenCircles(ic, t) * deltaBetweenCircles_ddt(ic, t);
}

// distance between the line holding the rotating segment, and the translating circle
float lineCircleDistance(const InitialConditions& ic, float t)
{
  const float segmentDy = ic.rotatingSegmentPosBase - ic.rotatingSegmentPosTip;
  const float segmentDx = ic.rotatingCircleRotationRadius;
  const float rotatingSegmentSlantedAngle = atan(segmentDy / segmentDx);
  const float dist = ic.rotatingSegmentPosBase;

  const float angle = ic.rotatingCircleOmega * t - rotatingSegmentSlantedAngle;
  const Vec2 normal = Vec2(-sin(angle), cos(angle));
  const Vec2 circleCenter = ic.translatingCircleInitialPos + ic.translatingCircleVelocity * t;
  return dotProduct(normal, circleCenter) - dist - ic.translatingCircleRadius;
}

// analytical first time derivative for 'lineCircleDistance'
float lineCircleDistance_ddt(const InitialConditions& ic, float t)
{
  const float segmentDy = ic.rotatingSegmentPosBase - ic.rotatingSegmentPosTip;
  const float segmentDx = ic.rotatingCircleRotationRadius;
  const float rotatingSegmentSlantedAngle = atan(segmentDy / segmentDx);

  const float angle = ic.rotatingCircleOmega * t - rotatingSegmentSlantedAngle;
  const float angle_ddt = ic.rotatingCircleOmega;

  const Vec2 normal = Vec2(-sin(angle), cos(angle));
  const Vec2 normal_ddt = Vec2(-angle_ddt * cos(angle), -angle_ddt * sin(angle));

  const Vec2 circleCenter = ic.translatingCircleInitialPos + ic.translatingCircleVelocity * t;
  const Vec2 circleCenter_ddt = ic.translatingCircleVelocity;

  //--------------------------------------------------------------------------------
  return dotProduct(normal_ddt, circleCenter) + dotProduct(normal, circleCenter_ddt);
}

void drawCross(IDrawer* drawer, Vec2 pos, Color color)
{
  drawer->line(pos - Vec2{0.2, 0}, pos + Vec2{0.2, 0}, color);
  drawer->line(pos - Vec2{0, 0.2}, pos + Vec2{0, 0.2}, color);
}

void drawConfiguration(IDrawer* drawer, const InitialConditions& ic, float t, Color color)
{
  const auto sc = computeConfiguration(ic, t);

  drawer->circle(sc.rotatingCircleCenter, ic.rotatingCircleRadius, color);
  drawCross(drawer, sc.rotatingCircleCenter, color);
  drawer->line(sc.rotatingSegmentA, sc.rotatingSegmentB, color);
  drawer->circle(sc.translatingCircleCenter, ic.translatingCircleRadius, color);
}

struct CcdRotateApp : IApp
{
  CcdRotateApp()
  {
    initialConditions.rotatingCircleRadius = 0.8;
    initialConditions.rotatingCircleRotationRadius = 6.0;
    initialConditions.rotatingCircleRotationCenter = Vec2(0, 0);
    initialConditions.rotatingCircleOmega = 0.45;

    initialConditions.rotatingSegmentPosBase = 1.2;
    initialConditions.rotatingSegmentPosTip = initialConditions.rotatingCircleRadius;

    initialConditions.translatingCircleRadius = 1.3;
    initialConditions.translatingCircleInitialPos = Vec2(5, 5);
    initialConditions.translatingCircleVelocity = Vec2(-1, -2);

    compute(initialConditions);
  }

  void draw(IDrawer* drawer) override
  {
    const auto& ic = initialConditions;

    drawer->circle(Vec2{}, ic.rotatingSegmentPosBase, Green);

    drawConfiguration(drawer, ic, t, Green);

    if(tCollision >= 0)
      drawConfiguration(drawer, initialConditions, tCollision, Red);

    drawCross(drawer, ic.rotatingCircleRotationCenter, White);

    char buf[256];
    sprintf(buf, "t=%.2f, dist=%.2f, lineDist=%.2f, derivative=%.2f, tCollision=%.2f", t, sqrDistBetweenCircles(ic, t),
          lineCircleDistance(ic, t), sqrDistBetweenCircles_ddt(ic, t), tCollision);
    drawer->text({}, buf, White, {-580, -80});
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
      initialConditions.translatingCircleInitialPos.x += 0.2;
      break;
    case Key::PageDown:
      initialConditions.translatingCircleInitialPos.x -= 0.2;
      break;
    default:
      break;
    }

    compute(initialConditions);
  }

  void compute(const InitialConditions& ic)
  {
    tCollision = -1;

    {
      float x = 0;

      for(int k = 0; k < 10; ++k)
        x -= lineCircleDistance(ic, x) / lineCircleDistance_ddt(ic, x);

      if(lineCircleDistance(ic, x) < 0.01)
      {
        auto sc = computeConfiguration(ic, x);
        // check if the translating circle collides with the segment
        if(dotProduct(sc.translatingCircleCenter - sc.rotatingSegmentA, sc.rotatingSegmentB - sc.rotatingSegmentA) >
                    0 &&
              dotProduct(sc.translatingCircleCenter - sc.rotatingSegmentB, sc.rotatingSegmentA - sc.rotatingSegmentB) >
                    0)
        {
          tCollision = x;
        }
      }
    }

    {
      float x = 0;

      for(int k = 0; k < 15; ++k)
        x -= sqrDistBetweenCircles(ic, x) / sqrDistBetweenCircles_ddt(ic, x);

      if(sqrDistBetweenCircles(ic, x) < 0.01)
        if(tCollision == -1 || x < tCollision)
          tCollision = x;
    }
  }

  float t = 0;
  float tCollision = 0;

  InitialConditions initialConditions{};
};

const int registered = registerApp("CollisionDetection/CCD_Rotate", []() -> IApp* { return new CcdRotateApp; });
}
