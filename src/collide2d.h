#pragma once

#include "geom.h"

struct Segment
{
  Vec2 a, b;
};

enum Shape
{
  Circle,
  Box,
};

// Tries to move a circle of radius 'RADIUS', initially at 'pos',
// to the position 'pos+delta'.
// Collides with 'segments', and slides along them on collision.
void slideMove(Vec2& pos, Shape shape, Vec2 delta, span<Segment> segments);

static auto const RADIUS = 0.8f;

