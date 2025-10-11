#pragma once

#include <cstdarg>

#include "drawer.h"

struct Vec2;

struct IVisualizer : IDrawer
{
  virtual void printf(const char* fmt, va_list args) = 0;

  // pause execution and display submitted primitives
  virtual void step() = 0;
};

extern IVisualizer* gVisualizer;
extern IVisualizer* const gNullVisualizer;

void sandbox_breakpoint();
void sandbox_line(Vec2 a, Vec2 b, Color color = White);
void sandbox_line(Vec3 a, Vec3 b, Color color = White);
void sandbox_rect(Vec2 a, Vec2 b, Color color = White, Vec2 invariantSize = {});
void sandbox_circle(Vec2 center, float radius, Color color = White, float invariantRadius = 0);
void sandbox_text(Vec2 pos, const char* text, Color color = White, Vec2 invariantOffset = {});
void sandbox_printf(const char* fmt, ...);
