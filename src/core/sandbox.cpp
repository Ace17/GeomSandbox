#include "sandbox.h"

#include <cstdarg>

#include "geom.h"

struct NullVisualizer : IVisualizer
{
  void line(Vec3, Vec3, Color){};
  void line(Vec2, Vec2, Color){};
  void rect(Vec2, Vec2, Color, Vec2){};
  void circle(Vec2, float, Color, float){};
  void text(Vec2, const char*, Color, Vec2){};
  void printf(const char*, va_list){};
  void step() override{};
};

static NullVisualizer nullVisualizer;
IVisualizer* const gNullVisualizer = &nullVisualizer;
IVisualizer* gVisualizer = &nullVisualizer;

void sandbox_breakpoint() { gVisualizer->step(); }
void sandbox_line(Vec2 a, Vec2 b, Color color) { gVisualizer->line(a, b, color); }
void sandbox_line(Vec3 a, Vec3 b, Color color) { gVisualizer->line(a, b, color); }
void sandbox_rect(Vec2 a, Vec2 b, Color color, Vec2 invariantSize) { gVisualizer->rect(a, b, color, invariantSize); }
void sandbox_circle(Vec2 center, float radius, Color color, float invariantRadius)
{
  gVisualizer->circle(center, radius, color, invariantRadius);
}
void sandbox_text(Vec2 pos, const char* text, Color color, Vec2 offset) { gVisualizer->text(pos, text, color, offset); }
void sandbox_printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  gVisualizer->printf(fmt, args);
  va_end(args);
}
