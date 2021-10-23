// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Entry point

#include <map>
#include <memory>
#include <stdexcept>

#include "app.h"
#include "font.h"
#include "geom.h"
#include "SDL.h"

const int WIDTH = 1280;
const int HEIGHT = 720;

float g_Scale = 20.0f;
float g_TargetScale = 20.0f;
Vec2 g_Pos {};
Vec2 g_TargetPos {};

Vec2 direction(float angle)
{
  return Vec2(cos(angle), sin(angle));
}

SDL_Point transform(Vec2 v)
{
  SDL_Point r;
  r.x = WIDTH / 2 + (v.x - g_Pos.x) * g_Scale;
  r.y = HEIGHT / 2 - (v.y - g_Pos.y) * g_Scale;
  return r;
};

int convert255(float val)
{
  int result = int(val * 255.0f);

  if(result < 0)
    result = 0;

  if(result > 255)
    result = 255;

  return result;
}

struct SdlDrawer : IDrawer
{
  SDL_Renderer* renderer;

  void line(Vec2 a, Vec2 b, Color color = White) override
  {
    const auto A = transform(a);
    const auto B = transform(b);

    setColor(color);

    SDL_RenderDrawLine(renderer, A.x, A.y, B.x, B.y);
  }

  void rect(Vec2 a, Vec2 b, Color color = White) override
  {
    const auto A = transform(a);
    const auto B = transform(a + b);

    setColor(color);

    SDL_RenderDrawLine(renderer, A.x, A.y, A.x, B.y);
    SDL_RenderDrawLine(renderer, B.x, A.y, B.x, B.y);
    SDL_RenderDrawLine(renderer, A.x, A.y, B.x, A.y);
    SDL_RenderDrawLine(renderer, A.x, B.y, B.x, B.y);
  }

  void circle(Vec2 center, float radius, Color color = White)
  {
    setColor(color);
    auto const N = 20;
    SDL_Point PREV {};

    for(int i = 0; i <= N; ++i)
    {
      auto A = transform(center + direction(i * 2 * M_PI / N) * radius);

      if(i > 0)
        SDL_RenderDrawLine(renderer, A.x, A.y, PREV.x, PREV.y);

      PREV = A;
    }
  }

  void text(Vec2 pos, const char* text, Color color = White) override
  {
    auto POS = transform(pos);
    setColor(color);

    while(*text)
    {
      drawChar(POS.x, POS.y, *text);
      POS.x += 16;
      ++text;
    }
  }

  void drawChar(int x, int y, char c)
  {
    for(int row = 0; row < 16; ++row)
      for(int col = 0; col < 16; ++col)
      {
        if((font8x8_basic[c % 128][row / 2] >> (col / 2)) & 1)
          SDL_RenderDrawPoint(renderer, x + col, y + row);
      }
  }

  void setColor(Color color)
  {
    const auto red = convert255(color.r);
    const auto green = convert255(color.g);
    const auto blue = convert255(color.b);
    const auto alpha = convert255(color.a);

    SDL_SetRenderDrawColor(renderer, red, green, blue, alpha);
  }
};

void drawScreen(SDL_Renderer* renderer, IApp* app)
{
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  SdlDrawer drawer;
  drawer.renderer = renderer;
  app->draw(&drawer);

  SDL_RenderPresent(renderer);
}

Key fromSdlKey(int key)
{
  switch(key)
  {
  case SDLK_LEFT: return Key::Left;
  case SDLK_RIGHT: return Key::Right;
  case SDLK_UP:   return Key::Up;
  case SDLK_DOWN: return Key::Down;
  case SDLK_SPACE: return Key::Space;
  case SDLK_HOME: return Key::Home;
  case SDLK_RETURN: return Key::Return;
  }

  return Key::Unknown;
}

bool readInput(IApp* app, bool& reset)
{
  SDL_Event event;

  while(SDL_PollEvent(&event))
  {
    if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE))
      return false;

    if(event.type == SDL_KEYDOWN)
    {
      app->keydown(fromSdlKey(event.key.keysym.sym));

      const float scaleSpeed = 1.05;
      const float scrollSpeed = 1.0;
      switch(event.key.keysym.sym)
      {
      case SDLK_KP_PLUS:
        g_TargetScale = g_Scale * scaleSpeed;
        break;
      case SDLK_KP_MINUS:
        g_TargetScale = g_Scale / scaleSpeed;
        break;
      case SDLK_KP_4:
        g_TargetPos = g_Pos + Vec2(-scrollSpeed, 0);
        break;
      case SDLK_KP_6:
        g_TargetPos = g_Pos + Vec2(+scrollSpeed, 0);
        break;
      case SDLK_KP_2:
        g_TargetPos = g_Pos + Vec2(0, +scrollSpeed);
        break;
      case SDLK_KP_8:
        g_TargetPos = g_Pos + Vec2(0, -scrollSpeed);
        break;
      case SDLK_F2:
        reset = true;
        break;
      }
    }
    else if(event.type == SDL_KEYUP)
    {
      app->keyup(fromSdlKey(event.key.keysym.sym));
    }
  }

  return true;
}

std::map<std::string, CreationFunc*> & Registry()
{
  static std::map<std::string, CreationFunc*> registry;
  return registry;
}

int registerApp(const char* name, CreationFunc* func)
{
  Registry()[name] = func;
  return 0;
}

int main(int argc, char* argv[])
{
  try
  {
    std::string appName = Registry().begin()->first;

    if(argc == 2)
      appName = argv[1];

    auto i_func = Registry().find(appName);

    if(i_func == Registry().end())
    {
      fprintf(stderr, "Unknown app: '%s'\n", appName.c_str());
      return 1;
    }

    if(SDL_Init(SDL_INIT_VIDEO) != 0)
      throw std::runtime_error("Can't init SDL");

    SDL_Window* window;
    SDL_Renderer* renderer;

    if(SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer) != 0)
    {
      SDL_Quit();
      throw std::runtime_error("Can't create window");
    }

    SDL_SetWindowTitle(window, appName.c_str());
    CreationFunc* func = Registry()[appName];
    auto app = std::unique_ptr<IApp>(func());

    bool reset = false;

    while(readInput(app.get(), reset))
    {
      if(reset)
      {
        app.reset(func());
        reset = false;
      }

      const float alpha = 0.8;
      g_Pos = (g_TargetPos * (1 - alpha) + g_Pos * alpha);
      g_Scale = (g_TargetScale * (1 - alpha) + g_Scale * alpha);

      app->tick();
      drawScreen(renderer, app.get());
      SDL_Delay(10);
    }

    SDL_Quit();
    return 0;
  }
  catch(const std::exception& e)
  {
    fprintf(stderr, "Fatal: %s\n", e.what());
    fflush(stderr);
    return 1;
  }
}

