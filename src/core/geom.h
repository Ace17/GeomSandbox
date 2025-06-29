// Copyright (C) 2023 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
#pragma once

#include <cstddef>

///////////////////////////////////////////////////////////////////////////////
// Vec2
///////////////////////////////////////////////////////////////////////////////

struct Vec2
{
  float x = 0;
  float y = 0;

  Vec2() = default;
  Vec2(float x_, float y_)
      : x(x_)
      , y(y_)
  {
  }

  friend void operator+=(Vec2& a, Vec2 b) { a = a + b; }
  friend void operator-=(Vec2& a, Vec2 b) { a = a - b; }
  friend void operator*=(Vec2& a, float b) { a = a * b; }
  friend void operator/=(Vec2& a, float b) { a = a / b; }

  friend Vec2 operator-(Vec2 v) { return Vec2{-v.x, -v.y}; }
  friend Vec2 operator+(Vec2 a, Vec2 b) { return Vec2{a.x + b.x, a.y + b.y}; }
  friend Vec2 operator-(Vec2 a, Vec2 b) { return Vec2{a.x - b.x, a.y - b.y}; }
  friend Vec2 operator*(Vec2 v, float f) { return Vec2{v.x * f, v.y * f}; }
  friend Vec2 operator*(float f, Vec2 v) { return v * f; }
  friend Vec2 operator/(Vec2 v, float f) { return Vec2{v.x / f, v.y / f}; }

  friend float operator*(Vec2 a, Vec2 b) { return dotProduct(a, b); }
  friend bool operator==(Vec2 a, Vec2 b) { return a.x == b.x && a.y == b.y; }

  friend float magnitude(Vec2 v);
  friend float dotProduct(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
  friend Vec2 normalize(Vec2 v) { return v / magnitude(v); }
  friend Vec2 rotateLeft(Vec2 v) { return Vec2(-v.y, v.x); }
};

///////////////////////////////////////////////////////////////////////////////
// Vec3
///////////////////////////////////////////////////////////////////////////////

struct Vec3
{
  float x = 0;
  float y = 0;
  float z = 0;

  Vec3() = default;
  Vec3(float x_, float y_, float z_)
      : x(x_)
      , y(y_)
      , z(z_)
  {
  }
};

inline Vec3 operator-(Vec3 v) { return Vec3{-v.x, -v.y, -v.z}; }
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 a, float val) { return {a.x * val, a.y * val, a.z * val}; }
inline Vec3 operator*(float val, Vec3 a) { return {a.x * val, a.y * val, a.z * val}; }
inline Vec3 operator/(Vec3 v, float f) { return {v.x / f, v.y / f, v.z / f}; }

inline void operator+=(Vec3& a, Vec3 b) { a = a + b; }
inline void operator-=(Vec3& a, Vec3 b) { a = a - b; }
inline void operator*=(Vec3& a, float f) { a = a * f; }

inline float dotProduct(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float operator*(Vec3 a, Vec3 b) { return dotProduct(a, b); }
inline bool operator==(Vec3 a, Vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

float magnitude(Vec3 v);
inline Vec3 normalize(Vec3 v) { return v / magnitude(v); }

inline Vec3 crossProduct(Vec3 a, Vec3 b)
{
  Vec3 r;
  r.x = a.y * b.z - a.z * b.y;
  r.y = a.z * b.x - b.z * a.x;
  r.z = a.x * b.y - a.y * b.x;
  return r;
}

///////////////////////////////////////////////////////////////////////////////
// span
///////////////////////////////////////////////////////////////////////////////

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

  // construction from static array
  template<size_t N>
  span(T (&tab)[N])
  {
    ptr = tab;
    len = N;
  }
};

///////////////////////////////////////////////////////////////////////////////
// misc
///////////////////////////////////////////////////////////////////////////////

inline float min(float a, float b) { return a < b ? a : b; }
inline float max(float a, float b) { return a > b ? a : b; }
