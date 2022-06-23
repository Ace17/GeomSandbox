// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Collision detection and response

#include <cmath>
#include <vector>

#include "app.h"
#include "collide2d.h"
#include "drawer.h"

namespace
{
struct Input
{
  bool left, right, up, down;
  bool force;
  bool changeShape;
};

struct World
{
  Vec2 pos;
  float angle;
  Shape shape = Circle;

  std::vector<Segment> segments;
};

Vec2 direction(float angle) { return Vec2(cos(angle), sin(angle)); }

template<size_t N>
void pushPolygon(std::vector<Segment>& out, Vec2 const (&data)[N])
{
  for(size_t i = 0; i < N; ++i)
  {
    auto a = data[(i + 0) % N];
    auto b = data[(i + 1) % N];
    out.push_back(Segment{a, b});
  }
}

World createWorld()
{
  World world;

  world.pos = Vec2(4, 2);
  world.angle = 0;

  static const Vec2 points1[] = {
        Vec2(8, -3),
        Vec2(8, 2),
        Vec2(12, 2),
        Vec2(12, 3),
        Vec2(12, 5),
        Vec2(14, 5),
        Vec2(12, 5),
        Vec2(12, 7),
        Vec2(15, 7),
        Vec2(15, 16),
        Vec2(-3, 16),
        Vec2(-3, 14),
        Vec2(10, 14),
        Vec2(10, 4),
        Vec2(8, 4),
        Vec2(4, 4),
        Vec2(4, 5),
        Vec2(8, 8),
        Vec2(3, 8),
        Vec2(3, 13),
        Vec2(1, 13),
        Vec2(-1, 13),
        Vec2(-3, 13),
        Vec2(-3, 10),
        Vec2(1, 10),
        Vec2(1, 8),
        Vec2(0, 8),
        Vec2(-1, 3.5),
        Vec2(-2, 3.5),
        Vec2(-2, 3.0),
        Vec2(-3, 3.0),
        Vec2(-3, 2.5),
        Vec2(-4, 2.5),
        Vec2(-4, 2),
        Vec2(-5, 2),
        Vec2(-5, -3),
  };

  pushPolygon(world.segments, points1);

  static const Vec2 points2[] = {
        Vec2(-1, -2.0),
        Vec2(-2, -2.0),
        Vec2(-1.5, -1),
  };

  pushPolygon(world.segments, points2);

  static const Vec2 points3[] = {
        Vec2(2, -3),
        Vec2(3, -2),
        Vec2(4, -3),
        Vec2(3, -2),
        Vec2(4, -1),
        Vec2(5, -2),
        Vec2(6, -3),
  };

  pushPolygon(world.segments, points3);

  static const Vec2 points4[] = {
        Vec2(12, 7),
        Vec2(12, 9),
        Vec2(13, 9),
        Vec2(13, 7),
  };

  pushPolygon(world.segments, points4);

  static const Vec2 points5[] = {
        Vec2(12, 9),
        Vec2(12, 11),
        Vec2(13, 11),
        Vec2(13, 9),
  };

  pushPolygon(world.segments, points5);
  return world;
}

void tick(World& world, Input input)
{
  float omega = 0;
  float thrust = 0;

  if(input.left)
    omega += 0.1;

  if(input.right)
    omega -= 0.1;

  if(input.down)
    thrust -= 0.08;

  if(input.up)
    thrust += 0.08;

  if(input.changeShape)
    world.shape = Shape(1 - world.shape);

  world.angle += omega;
  auto const delta = direction(world.angle) * thrust;

  auto segments = span<Segment>{world.segments.size(), world.segments.data()};

  if(input.force)
    world.pos += delta;
  else
    slideMove(world.pos, world.shape, delta, segments);
}

struct Collide2DApp : IApp
{
  Collide2DApp() { world = createWorld(); }

  void tick() override
  {
    input.left = keyState[(int)Key::Left];
    input.right = keyState[(int)Key::Right];
    input.up = keyState[(int)Key::Up];
    input.down = keyState[(int)Key::Down];

    ::tick(world, input);

    input = {};
  }

  void draw(IDrawer* drawer) override
  {
    for(auto& segment : world.segments)
    {
      drawer->line(segment.a, segment.b);
      drawer->rect(segment.a - Vec2(0.2, 0.2), Vec2(0.4, 0.4));
    }

    drawer->line(world.pos, world.pos + direction(world.angle) * RADIUS, Green);

    if(world.shape == Circle)
    {
      drawer->circle(world.pos, RADIUS, Green);
    }
    else
    {
      auto half = Vec2(-RADIUS, -RADIUS);
      drawer->rect(world.pos - half, half * 2, Green);
    }
  }

  void keydown(Key key) override
  {
    keyState[(int)key] = true;

    if(key == Key::Space)
      input.changeShape = true;
  }

  void keyup(Key key) override { keyState[(int)key] = false; }

  bool keyState[128]{};
  World world{};
  Input input{};
};

const int registered = registerApp("collide2d", []() -> IApp* { return new Collide2DApp; });
}
