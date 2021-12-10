#include "geom.h"
#include "visualizer.h"

struct NullVisualizer : IVisualizer
{
  void begin() override {};
  void end() override {};
  void line(Vec2, Vec2) override {};
};

static NullVisualizer nullVisualizer;
IVisualizer* gVisualizer = &nullVisualizer;

