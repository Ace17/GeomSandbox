// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Triangulation, Bowyer-Watson algorithm
// This is the algorithm implementation.

#pragma once

#include "core/geom.h"

#include <vector>

struct Edge
{
  int a, b;
};

std::vector<Edge> triangulate_BowyerWatson(span<const Vec2> points);
