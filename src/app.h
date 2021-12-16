#pragma once

#include "geom.h"

enum class Key
{
  Unknown,
  Return,
  Space,
  Left,
  Right,
  Up,
  Down,
  Home,
};

struct IDrawer;

struct IApp
{
  virtual ~IApp() = default;

  virtual void tick(){};
  virtual void draw(IDrawer*){};
  virtual void keydown(Key){};
  virtual void keyup(Key){};
};

typedef IApp* CreationFunc();
int registerApp(const char* name, CreationFunc* func);
