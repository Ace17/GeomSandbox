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
  End,
  PageUp,
  PageDown,
};

struct InputEvent
{
  bool pressed;
  Key key;
};

struct IDrawer;

struct IApp
{
  virtual ~IApp() = default;

  virtual void tick(){};
  virtual void draw(IDrawer*){};
  virtual void processEvent(InputEvent){};
};

typedef IApp* CreationFunc();
int registerApp(const char* name, CreationFunc* func);
