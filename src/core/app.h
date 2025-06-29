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
  F1,
  F2,
  F3,
  F4,
  KeyPad_Plus,
  KeyPad_Minus,
  KeyPad_0,
  KeyPad_1,
  KeyPad_2,
  KeyPad_3,
  KeyPad_4,
  KeyPad_5,
  KeyPad_6,
  KeyPad_7,
  KeyPad_8,
  KeyPad_9,
};

struct InputEvent
{
  bool pressed;
  Vec2 mousePos;
  int wheel;
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
