#include "polygon.h"

static float magnitude(Vec2 v) { return sqrt(v * v); }
static Vec2 normalize(Vec2 v) { return v * (1.0 / magnitude(v)); }
static Vec2 rotateLeft(Vec2 v) { return Vec2(-v.y, v.x); }

Vec2 Polygon::normal(int faceIdx) const
{
  const face = faces[faceIdx];
  const a = vertices[face[0]];
  const b = vertices[face[1]];
  return Normalize(rotateLeft(b - a));
}

float Polygon::faceLength(int faceIdx) const
{
  const face = faces[faceIdx];
  const A = vertices[face[0]];
  const B = vertices[face[1]];
  return magnitude(B - A);
}

