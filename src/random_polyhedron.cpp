// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "random_polyhedron.h"

#include <algorithm> // reverse
#include <cmath>

#include "random.h"

void opExtrudeFace(PolyhedronFL& poly, int faceIdx, float amount)
{
  const auto A = poly.vertices[poly.faces[faceIdx].indices[0]];
  const auto B = poly.vertices[poly.faces[faceIdx].indices[1]];
  const auto C = poly.vertices[poly.faces[faceIdx].indices[2]];
  const auto N = normalize(crossProduct(B - A, C - A));

  PolyhedronFacet oldFace = std::move(poly.faces[faceIdx]);

  // build translated face
  const int baseVertex = poly.vertices.size();
  PolyhedronFacet& newFace = poly.faces[faceIdx];
  for(auto index : oldFace.indices)
  {
    const int vertexId = poly.vertices.size();
    poly.vertices.push_back(poly.vertices[index] + N * amount);
    newFace.indices.push_back(vertexId);
  }

  // build side faces
  const int M = oldFace.indices.size();
  for(int i = 0; i < M; ++i)
  {
    int i0 = oldFace.indices[i];
    int i1 = oldFace.indices[(i + 1) % M];
    int i2 = baseVertex + i;
    int i3 = baseVertex + (i + 1) % M;

    poly.faces.push_back({{i0, i1, i3, i2}});
  }
}

PolyhedronFL createSpiralPolyhedron()
{
  PolyhedronFL r;

  {
    r.faces.push_back({});
    auto& face = r.faces.back();
    for(int k = 0; k < 48; ++k)
    {
      const float rInt = 2 + k * 0.2;
      const float rExt = rInt + 1.5;

      const float angle = 2 * M_PI * k * 0.05;

      const Vec3 ray = {(float)cos(angle), (float)sin(angle), 0};

      const auto iInt = (int)r.vertices.size();
      r.vertices.push_back(ray * rInt);

      const auto iExt = (int)r.vertices.size();
      r.vertices.push_back(ray * rExt);

      face.indices.push_back(iInt);
      face.indices.insert(face.indices.begin(), iExt);
    }
  }

  opExtrudeFace(r, 0, 10);

  return r;
}

PolyhedronFL createRandomPolyhedronFL()
{
  if(randomInt(0, 10) == 0)
    return createSpiralPolyhedron();

  PolyhedronFL r;

  const int N = randomInt(3, 14);
  const float radius = randomFloat(7, 15);
  const float halfLength = randomFloat(0.1, 2.0);
  const float phase = randomFloat(0, M_PI);

  PolyhedronFacet botCap, topCap;

  for(int i = 0; i < N; ++i)
  {
    const auto a0 = (2 * M_PI * (i + 0)) / N + phase;
    const auto a1 = (2 * M_PI * (i + 1)) / N + phase;

    Vec3 p[4];
    p[0] = Vec3(cos(a0), sin(a0), -halfLength) * radius;
    p[1] = Vec3(cos(a0), sin(a0), +halfLength) * radius;
    p[2] = Vec3(cos(a1), sin(a1), -halfLength) * radius;
    p[3] = Vec3(cos(a1), sin(a1), +halfLength) * radius;

    const int base = r.vertices.size();
    for(auto& v : p)
      r.vertices.push_back(v);

    r.faces.push_back({{base + 2, base + 3, base + 1, base + 0}});

    botCap.indices.push_back(base + 0);
    topCap.indices.push_back(base + 1);
  }

  std::reverse(botCap.indices.begin(), botCap.indices.end());

  r.faces.push_back(std::move(topCap));
  r.faces.push_back(std::move(botCap));

  for(int k = 0; k < 4; ++k)
  {
    int face = randomInt(0, r.faces.size());
    float amount = randomFloat(0.3, 30);
    opExtrudeFace(r, face, amount);
  }

  return r;
}
