#pragma once

#include <vector>

struct Edge
{
  int a, b;
};

std::vector<Edge> triangulate_BowyerWatson(span<const Vec2> points);
std::vector<Edge> triangulate_Flip(span<const Vec2> points);

