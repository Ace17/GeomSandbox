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

void sandbox_breakpoint();
void sandbox_line(Vec2 a, Vec2 b, Color color = White);
void sandbox_rect(Vec2 a, Vec2 b, Color color = White);
void sandbox_circle(Vec2 center, float radius, Color color = White);
void sandbox_text(Vec2 pos, const char* text, Color color = White);
