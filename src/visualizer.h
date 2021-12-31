#pragma once

#include "drawer.h"

struct Vec2;

struct IVisualizer : IDrawer
{
  // pause execution and display submitted primitives
  virtual void step() = 0;
};

extern IVisualizer* gVisualizer;
extern IVisualizer* const gNullVisualizer;
