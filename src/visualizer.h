#pragma once

struct Vec2;

struct IVisualizer
{
  virtual ~IVisualizer() = default;

  // submit primitives for drawing
  virtual void line(Vec2 a, Vec2 b) = 0;

  // pause execution and display submitted primitives
  virtual void step() = 0;
};

extern IVisualizer* gVisualizer;

