// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Main menu

#include "core/app.h"
#include "core/drawer.h"
#include "core/geom.h"

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

std::map<std::string, CreationFunc*>& Registry();

namespace
{

struct MainMenuApp : IApp
{
  MainMenuApp()
  {
    for(auto& pair : Registry())
    {
      if(pair.first == "MainMenu")
        continue;
      appNames.push_back(pair.first.c_str());
    }
  }

  void tick() override
  {
    if(sub)
      sub->tick();
  }

  void draw(IDrawer* drawer) override
  {
    if(sub)
    {
      sub->draw(drawer);
      return;
    }

    Vec2 pos{};
    for(auto& name : appNames)
    {
      drawer->text(pos, name.c_str());
      pos += Vec2(0, -1);
    }

    {
      Vec2 rectMin{-1, float(-selection - 1)};
      drawer->rect(rectMin, {30, 1});
    }
  }

  void keydown(Key key) override
  {
    if(sub)
    {
      sub->keydown(key);
      return;
    }

    const int N = (int)appNames.size();
    switch(key)
    {
    case Key::Down:
      selection = (selection + 1) % N;
      break;
    case Key::Up:
      selection = (selection - 1 + N) % N;
      break;
    case Key::Return:
    {
      std::string appName = appNames[selection];
      CreationFunc* func = Registry()[appName];
      sub.reset(func());
      break;
    }
    default:
      break;
    }
  }

  void keyup(Key key) override
  {
    if(sub)
    {
      sub->keyup(key);
      return;
    }
  }

  int selection = 0;
  std::string selectionName;
  std::unique_ptr<IApp> sub;
  std::vector<std::string> appNames;
};

const int registered = registerApp("MainMenu", []() -> IApp* { return new MainMenuApp; });
}
