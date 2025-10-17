#pragma once

#include "geom.h" // Vec2

struct Color
{
  float r, g, b, a;
};

constexpr Color White{1, 1, 1, 1};
constexpr Color Gray{0.2, 0.2, 0.2, 1};
constexpr Color Red{1, 0, 0, 1};
constexpr Color Orange{1, 0.5, 0, 1};
constexpr Color Green{0, 1, 0, 1};
constexpr Color Blue{0, 0, 1, 1};
constexpr Color LightBlue{0.5, 0.5, 1, 1};
constexpr Color Yellow{1, 1, 0, 1};

struct IDrawer
{
  virtual ~IDrawer() = default;

  virtual void line(Vec2 a, Vec2 b, Color color = White, Vec2 offsetA = {}, Vec2 offsetB = {}) = 0;
  virtual void rect(Vec2 a, Vec2 b, Color color = White, Vec2 invariantSize = {}) = 0;
  virtual void circle(Vec2 center, float radius, Color color = White, float invariantRadius = 0) = 0;
  virtual void text(Vec2 pos, const char* text, Color color = White, Vec2 offset = {}) = 0;

  virtual void line(Vec3 a, Vec3 b, Color color = White, Vec2 offsetA = {}, Vec2 offsetB = {}) = 0;
};
