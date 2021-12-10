#include "geom.h"
#include "visualizer.h"

struct NullVisualizer : IVisualizer
{
  void line(Vec2, Vec2) override {};
  void step() override {};
};

static NullVisualizer nullVisualizer;
IVisualizer* gVisualizer = &nullVisualizer;

