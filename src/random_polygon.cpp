// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "random_polygon.h"

#include "core/sandbox.h"

#include <array>
#include <cmath>
#include <vector>

#include "bounding_box.h"
#include "random.h"

namespace
{

template<typename T>
T lerp(T a, T b, float factor)
{
  return a * (1.0f - factor) + b * factor;
}

int opSplitFace(Polygon2f& poly, int faceIdx)
{
  const auto face = poly.faces[faceIdx];
  const auto v0 = poly.vertices[face.a];
  const auto v1 = poly.vertices[face.b];
  const auto middle = (v0 + v1) * 0.5;

  const auto vertexIdx = (int)poly.vertices.size();
  poly.vertices.push_back(middle);

  poly.faces[faceIdx].b = vertexIdx;
  poly.faces.push_back(Face{vertexIdx, face.b});

  return vertexIdx;
}

int opInsertFace(Polygon2f& poly, int faceIdx, float ratio)
{
  const auto face = poly.faces[faceIdx];
  const auto A = poly.vertices[face.a];
  const auto B = poly.vertices[face.b];

  // split face
  const auto iY = opSplitFace(poly, faceIdx);
  const auto iX = opSplitFace(poly, faceIdx);

  // .
  // -----A
  // .    |
  // .    |
  // .    X
  // .    |
  // .    | <- center
  // .    |
  // .    Y
  // .    |
  // .    |
  // -----B
  // .

  const auto alphaCenter = 0.5;
  const auto alphaX = alphaCenter - ratio * 0.5;
  const auto alphaY = alphaCenter + ratio * 0.5;

  const auto X = lerp(A, B, alphaX);
  const auto Y = lerp(A, B, alphaY);

  poly.vertices[iX] = X;
  poly.vertices[iY] = Y;

  return poly.faces.size() - 1;
}

void opInsetAndExtrude(Polygon2f& poly,
      int faceIdx,
      float insetCenter,
      float insetRatio,
      bool insetPolarity,
      float extrudeAmount)
{
  const auto face = poly.faces[faceIdx];
  const auto A = poly.vertices[face.a];
  const auto B = poly.vertices[face.b];
  const auto N = poly.normal(faceIdx);

  // split face
  const auto i3 = opSplitFace(poly, faceIdx);
  const auto i2 = opSplitFace(poly, faceIdx);
  const auto i1 = opSplitFace(poly, faceIdx);
  const auto i0 = opSplitFace(poly, faceIdx);

  // .
  // -----A
  // .    |
  // .    |
  // .    X ------ W
  // .    |        |
  // .    |        |
  // .    Y -------Z
  // .    |
  // .    |
  // -----B
  // .

  float ratio = insetRatio;

  if(!insetPolarity)
    ratio = 1.0 / ratio;

  const auto alphaX = insetCenter - ratio * 0.5;
  const auto alphaY = insetCenter + ratio * 0.5;

  const auto X = lerp(A, B, alphaX);
  const auto Y = lerp(A, B, alphaY);
  poly.vertices[i0] = X;
  poly.vertices[i3] = Y;

  poly.vertices[i1] = X + N * extrudeAmount;
  poly.vertices[i2] = Y + N * extrudeAmount;
}

void opExtrudeFace(Polygon2f& poly, int faceIdx, float amount)
{
  const auto N = poly.normal(faceIdx);
  const auto face = poly.faces[faceIdx];

  const auto A = poly.vertices[face.a];
  const auto B = poly.vertices[face.b];

  const auto i1 = opSplitFace(poly, faceIdx);
  const auto i0 = opSplitFace(poly, faceIdx);

  poly.vertices[i0] = A + N * amount;
  poly.vertices[i1] = B + N * amount;
}

void opMoveFaceAway(Polygon2f& poly, int faceIdx, float amount)
{
  const auto N = poly.normal(faceIdx);
  const auto face = poly.faces[faceIdx];
  poly.vertices[face.a] += N * amount;
  poly.vertices[face.b] += N * amount;
}

Polygon2f createRegularPolygon2f(int N, float radius1, float radius2)
{
  Polygon2f r;

  for(int i = 0; i < N; ++i)
  {
    float angle = -i * (M_PI * 2.0 / N);

    Vec2 v;
    v.x = cos(angle) * radius1;
    v.y = sin(angle) * radius2;
    r.vertices.push_back(v);
    r.faces.push_back({i, (i + 1) % N});
  }

  return r;
}

const auto OneMeter = 0.2;

void mutatorInsetAndExtrude(Polygon2f& poly)
{
  const auto faceIdx = randomInt(0, poly.faces.size());

  // choose inset polarity so that we enlarge small faces and reduce big ones
  const auto faceLength = poly.faceLength(faceIdx);
  const auto polarity = faceLength >= 5 * OneMeter;

  const auto center = 0.5;

  opInsetAndExtrude(poly, faceIdx, center, 0.2, polarity, OneMeter * randomFloat(10.0f, 40.0f));
}

void mutatorInsetAndMoveAway(Polygon2f& poly)
{
  const auto faceIdx = randomInt(0, poly.faces.size());
  const auto faceLength = poly.faceLength(faceIdx);

  // don't split small faces
  if(faceLength < 2 * OneMeter)
    return;

  if(faceLength > 14 * OneMeter)
    return;

  const auto amount = randomFloat(0.1, 0.7) * faceLength;
  float ratio = 0.2;

  if(randomInt(0, 2))
    ratio = 1.0 / ratio;

  const auto face = opInsertFace(poly, faceIdx, ratio);
  opMoveFaceAway(poly, face, amount);

  if(ratio > 1.0)
    opExtrudeFace(poly, face, 3 * amount);
}

typedef void MutatorFunction(Polygon2f& poly);

void mutate(Polygon2f& poly)
{
  static const MutatorFunction* mutators[] = {
        &mutatorInsetAndMoveAway,
        &mutatorInsetAndExtrude,
        &mutatorInsetAndExtrude,
  };

  mutators[randomInt(0, 3)](poly);
}

void swap(float& a, float& b)
{
  auto t = a;
  a = b;
  b = t;
}

void order(float& a, float& b)
{
  if(a > b)
    swap(a, b);
}

float distanceBetweenSegments1d(std::array<float, 2> u, std::array<float, 2> v)
{
  order(u[0], u[1]);
  order(v[0], v[1]);

  if(u[0] > v[0])
    swap(u, v);

  // the left point of v is inside u
  if(v[0] >= u[0] && v[0] <= u[1])
    return 0;

  return v[0] - u[1];
}

float distanceBetweenSegments(std::array<Vec2, 2> u, std::array<Vec2, 2> v)
{
  const Vec2 axises[] = {
        normalize(rotateLeft(u[1] - u[0])),
        normalize(rotateLeft(v[1] - v[0])),
        normalize(u[0] - v[0]),
        normalize(u[0] - v[1]),
        normalize(u[1] - v[1]),
        normalize(u[1] - v[0]),
  };

  float maxDist = -1.0 / 0.0;

  for(auto axis : axises)
  {
    const auto u0 = dotProduct(axis, u[0]);
    const auto u1 = dotProduct(axis, u[1]);
    const auto v0 = dotProduct(axis, v[0]);
    const auto v1 = dotProduct(axis, v[1]);

    const auto dist = distanceBetweenSegments1d({u0, u1}, {v0, v1});

    maxDist = max(maxDist, dist);
  }

  return maxDist;
}

float distance(const Polygon2f& poly, int faceIdx1, int faceIdx2)
{
  const auto face1 = poly.faces[faceIdx1];
  const auto face2 = poly.faces[faceIdx2];
  return distanceBetweenSegments(
        {poly.vertices[face1.a], poly.vertices[face1.b]}, {poly.vertices[face2.a], poly.vertices[face2.b]});
}

bool isValid(const Polygon2f& poly)
{
  for(int i = 0; i < (int)poly.faces.size(); ++i)
  {
    for(int j = i + 1; j < (int)poly.faces.size(); ++j)
    {
      const auto face1 = poly.faces[i];
      const auto face2 = poly.faces[j];

      if(face1.a != face2.a && face1.a != face2.b)
        if(face1.b != face2.a && face1.b != face2.b)
          if(distance(poly, i, j) < 2.0 * OneMeter)
            return false;
    }
  }

  return true;
}

void recenterPolygon(Polygon2f& polygon)
{
  BoundingBox bbox;

  for(const Vec2 vertex : polygon.vertices)
    bbox.add(vertex);

  const Vec2 polygonCenter = (bbox.min + bbox.max) / 2.0f;
  for(Vec2& vertex : polygon.vertices)
  {
    vertex -= polygonCenter;
  }
}

} // namespace

Vec2 Polygon2f::normal(int faceIdx) const
{
  const auto face = faces[faceIdx];
  const auto a = vertices[face.a];
  const auto b = vertices[face.b];
  return normalize(rotateLeft(b - a));
}

float Polygon2f::faceLength(int faceIdx) const
{
  const auto face = faces[faceIdx];
  const auto A = vertices[face.a];
  const auto B = vertices[face.b];
  return magnitude(B - A);
}

Polygon2f createRandomPolygon2f()
{
  const auto radius1 = randomFloat(5, 10) * OneMeter;
  const auto radius2 = randomFloat(5, 10) * OneMeter;
  const auto N = randomInt(3, 8);
  Polygon2f r = createRegularPolygon2f(N, radius1, radius2);

  auto drawAndStep = [&]()
  {
    for(auto face : r.faces)
      sandbox_line(r.vertices[face.a], r.vertices[face.b]);
    sandbox_breakpoint();
  };

  drawAndStep();

  for(int k = 0; k < 30; ++k)
  {
    Polygon2f backup = r;

    mutate(r);

    if(!isValid(r))
      r = backup;
    else
      drawAndStep();
  }

  recenterPolygon(r);

  return r;
}
