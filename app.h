#pragma once

#include "geom.h"

struct Color
{
  float r, g, b, a;
};

constexpr Color White{ 1, 1, 1, 1 };
constexpr Color Red{ 1, 0, 0, 1 };
constexpr Color Green{ 0, 1, 0, 1 };
constexpr Color Yellow{ 1, 1, 0, 1 };

struct IDrawer
{
  virtual ~IDrawer() = default;

  virtual void line(Vec2 a, Vec2 b, Color color = White) = 0;
  virtual void rect(Vec2 a, Vec2 b, Color color = White) = 0;
  virtual void circle(Vec2 center, float radius, Color color = White) = 0;
  virtual void text(Vec2 pos, const char* text, Color color = White) = 0;
};

enum class Key
{
  Unknown,
  Space,
  Left,
  Right,
  Up,
  Down,
  Home,
};

struct IApp
{
  virtual ~IApp() = default;

  virtual void tick() {};
  virtual void draw(IDrawer*) {};
  virtual void keydown(Key) {};
  virtual void keyup(Key) {};
};

typedef IApp* CreationFunc();
int registerApp(const char* name, CreationFunc* func);

