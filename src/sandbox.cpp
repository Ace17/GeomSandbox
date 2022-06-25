#include "geom.h"
#include "sandbox.h"

struct NullVisualizer : IVisualizer
{
  void line(Vec2 a, Vec2 b, Color color){};
  void rect(Vec2 a, Vec2 b, Color color){};
  void circle(Vec2 center, float radius, Color color){};
  void text(Vec2 pos, const char* text, Color color){};
  void step() override{};
};

static NullVisualizer nullVisualizer;
IVisualizer* const gNullVisualizer = &nullVisualizer;
IVisualizer* gVisualizer = &nullVisualizer;

void sandbox_breakpoint() { gVisualizer->step(); }
void sandbox_line(Vec2 a, Vec2 b, Color color) { gVisualizer->line(a, b, color); }
void sandbox_rect(Vec2 a, Vec2 b, Color color) { gVisualizer->rect(a, b, color); }
void sandbox_circle(Vec2 center, float radius, Color color) { gVisualizer->circle(center, radius, color); }
void sandbox_text(Vec2 pos, const char* text, Color color) { gVisualizer->text(pos, text, color); }
