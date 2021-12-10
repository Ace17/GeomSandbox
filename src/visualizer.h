#pragma once

struct Vec2;

struct IVisualizer
{
  virtual ~IVisualizer() = default;

  virtual void begin() = 0;
  virtual void end() = 0;

  virtual void line(Vec2 a, Vec2 b) = 0;
};

extern IVisualizer* gVisualizer;

