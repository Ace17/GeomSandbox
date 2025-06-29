#pragma once

#include <cmath>

#include "matrix4.h"

struct ICamera
{
  virtual ~ICamera() = default;

  virtual bool processEvent(InputEvent evt) = 0;
  virtual Matrix4f getTransform(float aspectRatio) const = 0;
};

const float SCALE_SPEED = 1.05;
const float SCROLL_SPEED = 1.0;

struct OrthoCamera : ICamera
{
  bool processEvent(InputEvent evt) override
  {
    if(evt.pressed)
    {
      switch(evt.key)
      {
      case Key::KeyPad_Plus:
        scale = scale * SCALE_SPEED;
        return true;
      case Key::KeyPad_Minus:
        scale = scale / SCALE_SPEED;
        return true;
      case Key::KeyPad_3:
        pos = {0, 0};
        return true;
      case Key::KeyPad_4:
        pos = pos + Vec2(-SCROLL_SPEED, 0);
        return true;
      case Key::KeyPad_6:
        pos = pos + Vec2(+SCROLL_SPEED, 0);
        return true;
      case Key::KeyPad_2:
        pos = pos + Vec2(0, -SCROLL_SPEED);
        return true;
      case Key::KeyPad_8:
        pos = pos + Vec2(0, +SCROLL_SPEED);
        return true;
      default:
        return false;
      }
    }

    if(evt.wheel)
    {
      // 'screen' to 'logical' coordinates
      auto reverseTransform = [&](Vec2 v)
      {
        v.x = v.x / scale + pos.x;
        v.y = v.y / scale + pos.y;
        return v;
      };

      const Vec2 mousePos = reverseTransform(evt.mousePos);

      const auto scaleFactor = evt.wheel > 0 ? (SCALE_SPEED * 1.1) : 1.0 / (SCALE_SPEED * 1.1);
      const auto newScale = scale * scaleFactor;
      auto relativePos = mousePos - pos;
      pos = mousePos - relativePos * (scale / newScale);
      scale = newScale;
      return true;
    }

    return false;
  }

  Matrix4f getTransform(float aspectRatio) const override
  {
    const auto scaleX = scale / aspectRatio;
    const auto scaleY = scale;
    return ::scale(Vec3{scaleX, scaleY, 0}) * translate(-1 * Vec3(pos.x, pos.y, 0));
  }

  Vec2 pos{};
  float scale = 0.06;
};

struct PerspectiveCamera : ICamera
{
  bool processEvent(InputEvent evt) override
  {
    if(!evt.pressed)
      return false;

    switch(evt.key)
    {
    case Key::KeyPad_3:
      pos = {0, 0, +24};
      return true;
    case Key::KeyPad_4:
      pos = pos + Vec3(-SCROLL_SPEED, 0, 0);
      return true;
    case Key::KeyPad_6:
      pos = pos + Vec3(+SCROLL_SPEED, 0, 0);
      return true;
    case Key::KeyPad_2:
      pos = pos + Vec3(0, -SCROLL_SPEED, 0);
      return true;
    case Key::KeyPad_8:
      pos = pos + Vec3(0, +SCROLL_SPEED, 0);
      return true;
    case Key::KeyPad_1:
      pos = pos + Vec3(0, 0, +SCROLL_SPEED);
      return true;
    case Key::KeyPad_7:
      pos = pos + Vec3(0, 0, -SCROLL_SPEED);
      return true;
    default:
      return false;
    }
  }

  Matrix4f getTransform(float aspectRatio) const override
  {
    const auto zNear = 0.1;
    const auto zFar = 100;

    const auto V = translate(-1 * pos);
    const auto P = perspective(M_PI * 0.5, aspectRatio, zNear, zFar);

    return P * V;
  }

  Vec3 pos{0, 0, 24};
};
