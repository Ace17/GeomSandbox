// Copyright (C) 2025 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Random polyhedron generation

#include "core/algorithm_app.h"
#include "core/sandbox.h"

#include "random_polyhedron.h"

namespace
{

struct RandomPolygon
{
  static int generateInput() { return 0; }

  static PolyhedronFL execute(int /*seed*/) { return createRandomPolyhedronFL(); }

  static void display(int /*input*/, const PolyhedronFL& output)
  {
    sandbox_line({0, 0, 0}, {1, 0, 0}, Red);
    sandbox_line({0, 0, 0}, {0, 1, 0}, Green);
    sandbox_line({0, 0, 0}, {0, 0, 1}, LightBlue);

    for(auto& face : output.faces)
    {
      const int N = face.indices.size();

      Vec3 faceCenter{};
      for(int i = 0; i < N; ++i)
      {
        auto a = output.vertices[face.indices[i]];
        auto b = output.vertices[face.indices[(i + 1) % N]];
        sandbox_line(a, b, Yellow);
        faceCenter += a;
      }

      {
        faceCenter *= (1.0f / N);

        const auto A = output.vertices[face.indices[0]];
        const auto B = output.vertices[face.indices[1]];
        const auto C = output.vertices[face.indices[2]];
        Vec3 faceNormal = normalize(crossProduct(B - A, C - A));

        sandbox_line(faceCenter + faceNormal * 0.5, faceCenter + faceNormal * 1.0f, {0.4f, 0, 0, 1});
        sandbox_line(faceCenter, faceCenter + faceNormal * 0.5f, {0.7f, 0, 0, 1});
      }
    }
  }
};

IApp* create() { return createAlgorithmApp(std::make_unique<ConcreteAlgorithm<RandomPolygon>>()); }
const int reg = registerApp("Random/Polyhedron", &create);
}
