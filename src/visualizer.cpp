#include "geom.h"
#include "visualizer.h"

struct NullVisualizer : IVisualizer
{
  void line(Vec2 a, Vec2 b, Color color) {};
  void rect(Vec2 a, Vec2 b, Color color) {};
  void circle(Vec2 center, float radius, Color color) {};
  void text(Vec2 pos, const char* text, Color color) {};
  void step() override {};
};

static NullVisualizer nullVisualizer;
IVisualizer* gVisualizer = &nullVisualizer;

