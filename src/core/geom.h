// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
#pragma once

#include <cstddef>

struct Vec2
{
  float x, y;

  Vec2() = default;
  Vec2(float x_, float y_)
      : x(x_)
      , y(y_)
  {
  }

  void operator+=(Vec2 other) { *this = *this + other; }
  void operator-=(Vec2 other) { *this = *this - other; }
  float operator*(Vec2 other) const { return x * other.x + y * other.y; }

  Vec2 operator-() const { return Vec2{-x, -y}; }

  Vec2 operator+(Vec2 other) const { return Vec2{x + other.x, y + other.y}; }
  Vec2 operator-(Vec2 other) const { return Vec2{x - other.x, y - other.y}; }
  Vec2 operator*(float f) const { return Vec2{x * f, y * f}; }
  Vec2 operator/(float f) const { return Vec2{x / f, y / f}; }

  bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }

  static Vec2 zero() { return Vec2(0, 0); }
};

float magnitude(Vec2 v);
inline float dot_product(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline Vec2 normalize(Vec2 v) { return v * (1.0 / magnitude(v)); }
inline Vec2 rotateLeft(Vec2 v) { return Vec2(-v.y, v.x); }
inline float min(float a, float b) { return a < b ? a : b; }
inline float max(float a, float b) { return a > b ? a : b; }

template<typename T>
struct span
{
  size_t len;
  T* ptr;

  T* begin() { return ptr; }
  T* end() { return ptr + len; }

  const T* begin() const { return ptr; }
  const T* end() const { return ptr + len; }

  T& operator[](int idx) { return ptr[idx]; }
  const T& operator[](int idx) const { return ptr[idx]; }

  span sub(int offset) { return span{len - offset, ptr + offset}; }

  span() = default;
  span(size_t N, T* tab)
      : len(N)
      , ptr(tab)
  {
  }

  void operator+=(int i)
  {
    ptr += i;
    len -= i;
  }
  auto& pop()
  {
    auto& r = ptr[0];
    (*this) += 1;
    return r;
  }

  // construction from vector/string
  template<typename U, typename = decltype(((U*)0)->data())>
  span(U& s)
  {
    ptr = s.data();
    len = s.size();
  }
};
