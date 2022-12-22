// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once

#include <cassert>
#include <cmath>

#include "geom.h"

struct Matrix4f
{
  struct row
  {
    float elements[4];

    float const& operator[](int i) const { return elements[i]; }

    float& operator[](int i) { return elements[i]; }
  };

  const row& operator[](int i) const { return data[i]; }

  row& operator[](int i) { return data[i]; }

  row data[4]{};
};

inline Matrix4f operator*(Matrix4f const& A, Matrix4f const& B)
{
  Matrix4f r{};

  for(int row = 0; row < 4; ++row)
    for(int col = 0; col < 4; ++col)
    {
      double sum = 0;

      for(int k = 0; k < 4; ++k)
        sum += A[row][k] * B[k][col];

      r[row][col] = sum;
    }

  return r;
}

inline Matrix4f translate(Vec3 v)
{
  Matrix4f r{};

  r[0][0] = 1;
  r[1][1] = 1;
  r[2][2] = 1;
  r[3][3] = 1;

  r[0][3] = v.x;
  r[1][3] = v.y;
  r[2][3] = v.z;

  return r;
}

inline Matrix4f scale(Vec3 v)
{
  Matrix4f r{};
  r[0][0] = v.x;
  r[1][1] = v.y;
  r[2][2] = v.z;
  r[3][3] = 1;
  return r;
}

inline Matrix4f transpose(const Matrix4f& m)
{
  Matrix4f r{};

  for(int row = 0; row < 4; ++row)
    for(int col = 0; col < 4; ++col)
      r[row][col] = m[col][row];

  return r;
}

inline Matrix4f perspective(float fovy, float aspect, float zNear, float zFar)
{
  assert(aspect != 0.0);
  assert(zFar != zNear);

  auto const rad = fovy;
  auto const tanHalfFovy = tan(rad / 2.0);

  Matrix4f r{};
  r[0][0] = 1.0 / (aspect * tanHalfFovy);
  r[1][1] = 1.0 / (tanHalfFovy);
  r[2][2] = -(zFar + zNear) / (zFar - zNear);
  r[3][2] = -1.0;
  r[2][3] = -(2.0 * zFar * zNear) / (zFar - zNear);
  return r;
}
