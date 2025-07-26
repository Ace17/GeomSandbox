#include "split_polyhedron.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <unordered_map>

namespace std
{
template<>
struct hash<Vec3>
{
  size_t operator()(const Vec3& pos) const
  {
    static auto floatHash = [](float v) { return std::hash<float>{}(v); };
    return floatHash(pos.x * 7) ^ floatHash(pos.y * 5) ^ floatHash(pos.z * 13);
  }
};
}

namespace
{
struct Diagonal
{
  int a, b;
};

int getVertex(Vec3 pos, PolyhedronFL& poly, std::unordered_map<Vec3, int>& cache)
{
  if(cache.empty())
  {
    for(int i = 0; i < (int)poly.vertices.size(); ++i)
      cache[poly.vertices[i]] = i;
  }

  auto it = cache.find(pos);
  if(it == cache.end())
  {
    const int i = poly.vertices.size();
    poly.vertices.push_back(pos);
    it = cache.insert(std::make_pair(pos, i)).first;
  }

  return it->second;
}

void copyFaceTo(PolyhedronFL& out,
      std::unordered_map<Vec3, int>& outCache,
      const PolyhedronFL& srcPoly,
      const PolyhedronFacet& srcFace)
{
  PolyhedronFacet newFace;
  for(auto index : srcFace.indices)
  {
    const Vec3 v = srcPoly.vertices[index];
    newFace.indices.push_back(getVertex(v, out, outCache));
  }
  out.faces.push_back(std::move(newFace));
}

std::vector<std::vector<int>> cutFaceAlongDiagonals(const PolyhedronFacet& face, span<Diagonal> diagonals)
{
  const int N = face.indices.size();

  static auto arcLength = [](int N, int a, int b)
  {
    const int fwd_length = max(a, b) - min(a, b);
    const int back_length = N - fwd_length;
    return min(fwd_length, back_length);
  };

  auto byIncreasingArcLength = [N](Diagonal p, Diagonal q) { return arcLength(N, p.a, p.b) < arcLength(N, q.a, q.b); };

  std::sort(diagonals.begin(), diagonals.end(), byIncreasingArcLength);

  std::vector<std::vector<int>> result;
  std::vector<int> next;
  for(int i = 0; i < N; ++i)
    next.push_back((i + 1) % N);

  int last = 0;
  for(auto [a, b] : diagonals)
  {
    const int fwd_length = (b - a + N) % N;
    const int back_length = (a - b + N) % N;

    int curr = a;
    last = b;
    if(fwd_length >= back_length)
      std::swap(curr, last);

    int watchdog = N * 2;

    std::vector<int> subPoly;
    subPoly.push_back(face.indices.at(curr));
    do
    {
      assert(watchdog-- > 0);
      curr = next[curr];
      subPoly.push_back(face.indices[curr]);
    } while(curr != last);

    if(fwd_length < back_length)
      next[a] = b;
    else
      next[b] = a;

    result.push_back(std::move(subPoly));
  }

  {
    std::vector<int> subPoly;
    int curr = last;
    int first = last;
    subPoly.push_back(face.indices[curr]);
    int watchdog = N * 2;

    while(true)
    {
      assert(watchdog-- > 0);
      curr = next[curr];
      if(curr == first)
        break;
      subPoly.push_back(face.indices[curr]);
    }
    result.push_back(std::move(subPoly));
  }

  return result;
}

const float epsilon = 0.001;

void splitFaceAgainstPlane(const PolyhedronFL& polyhedron,
      span<const int> sides,
      const PolyhedronFacet& face,
      Vec3 faceNormal,
      Plane3 plane,
      PolyhedronFL& front,
      PolyhedronFL& back)
{
  // The intersection between the splitting plane and
  // the plane supporting the face is a line.
  // Sort the intersection points along this line.
  struct IntersectionPoint
  {
    int indexInFace;
    bool isEntryPoint = false;
  };

  std::vector<IntersectionPoint> intersectionPoints;

  for(auto& vertexIndex : face.indices)
  {
    const int indexInFace = int(&vertexIndex - face.indices.data());
    if(sides[vertexIndex] == 0)
      intersectionPoints.push_back({indexInFace});
  }

  const Vec3 tangent = [&]() { return crossProduct(plane.normal, faceNormal); }();

  // sort the intersection points along 'tangent'
  {
    auto byAbscissa = [&](const IntersectionPoint& ip0, const IntersectionPoint& ip1)
    {
      auto p0 = polyhedron.vertices[face.indices[ip0.indexInFace]];
      auto p1 = polyhedron.vertices[face.indices[ip1.indexInFace]];
      return dotProduct(p0, tangent) < dotProduct(p1, tangent);
    };

    std::sort(intersectionPoints.begin(), intersectionPoints.end(), byAbscissa);
  }

  const auto& polygon = face.indices;

  const int N = polygon.size();

  // compute the 'isEntryPoint' flag for all intersection points
  {
    static auto encode = [](int prevSide, int, int nextSide) { return prevSide * 8 + nextSide; };

    for(auto& ip : intersectionPoints)
    {
      const int curr = face.indices[ip.indexInFace];
      const int prev = face.indices[(ip.indexInFace - 1 + N) % N];
      const int next = face.indices[(ip.indexInFace + 1 + N) % N];

      const Vec3 prevPos = polyhedron.vertices[prev];
      const Vec3 currPos = polyhedron.vertices[curr];
      const Vec3 nextPos = polyhedron.vertices[next];

      const int prevSide = sides[prev];
      const int currSide = sides[curr];
      const int nextSide = sides[next];

      const Vec3 prevEdge = currPos - prevPos;
      const Vec3 nextEdge = nextPos - currPos;

      assert(currSide == 0); // we're on an intersection point

      const int turn = dotProduct(crossProduct(prevEdge, nextEdge), faceNormal) >= 0 ? +1 : -1;

      switch(encode(prevSide, currSide, nextSide))
      {
      case encode(-1, 0, -1):
        if(turn == -1)
          ip.isEntryPoint = true;
        break;
      case encode(0, 0, -1):
        if(turn == -1)
          ip.isEntryPoint = true;
        break;
      case encode(+1, 0, -1):
        ip.isEntryPoint = true;
        break;
      case encode(+1, 0, 0):
        if(turn == -1)
          ip.isEntryPoint = true;
        break;
      case encode(+1, 0, +1):
        if(turn == -1)
          ip.isEntryPoint = true;
        break;
      }
    }
  }
  std::vector<Diagonal> diagonals;

  for(int i = 0; i + 1 < (int)intersectionPoints.size(); ++i)
  {
    if(intersectionPoints[i].isEntryPoint)
    {
      Diagonal diag;
      diag.a = intersectionPoints[i + 0].indexInFace;
      diag.b = intersectionPoints[i + 1].indexInFace;
      diagonals.push_back(diag);
    }
  }

  auto subPolygons = cutFaceAlongDiagonals(face, diagonals);

  for(auto& polygon : subPolygons)
  {
    int side = 0;
    for(auto& vertexIndex : polygon)
    {
      if(sides[vertexIndex])
      {
        side = sides[vertexIndex];
        break;
      }
    }

    auto& outPolyhedron = side == +1 ? front : back;
    assert(side == 1 || side == -1);

    outPolyhedron.faces.push_back({});
    auto& outFace = outPolyhedron.faces.back();
    for(auto srcVertexIndex : polygon)
    {
      const int dstVertexIndex = outPolyhedron.vertices.size();
      outPolyhedron.vertices.push_back(polyhedron.vertices[srcVertexIndex]);
      outFace.indices.push_back(dstVertexIndex);
    }
  }
}

}

void splitPolyhedronAgainstPlane(PolyhedronFL poly, Plane3 plane, PolyhedronFL& front, PolyhedronFL& back)
{
  front = {};
  back = {};

  // classify vertices
  std::vector<int> sides;

  for(auto& v : poly.vertices)
  {
    const auto x = dotProduct(v, plane.normal) - plane.dist;

    int side;
    if(x > +epsilon)
      side = +1;
    else if(x < -epsilon)
      side = -1;
    else
      side = 0;

    sides.push_back(side);
  }

  std::unordered_map<Vec3, int> cacheFront;
  std::unordered_map<Vec3, int> cacheBack;

  // insert the intersection points
  for(auto& face : poly.faces)
  {
    std::vector<int> newIndices;
    const int N = face.indices.size();
    for(int curr = 0; curr < N; ++curr)
    {
      int currVertexIndex = face.indices[curr];
      int nextVertexIndex = face.indices[(curr + 1) % N];

      // insert the current point
      newIndices.push_back(currVertexIndex);

      // insert an intersection point, if needed
      if(sides[currVertexIndex] * sides[nextVertexIndex] == -1)
      {
        const Vec3 p0 = poly.vertices[currVertexIndex];
        const Vec3 p1 = poly.vertices[nextVertexIndex];

        const float q0 = dotProduct(p0, plane.normal);
        const float q1 = dotProduct(p1, plane.normal);
        const float ratio = (plane.dist - q0) / (q1 - q0);
        const Vec3 intersectionPos = p0 + ratio * (p1 - p0);

        const int intersectionVertexIndex = poly.vertices.size();
        poly.vertices.push_back(intersectionPos);
        sides.push_back(0); // intersection vertex have a side of 0
        assert(sides.size() == poly.vertices.size());
        newIndices.push_back(intersectionVertexIndex);
      }
    }

    face.indices = std::move(newIndices);
  }

  for(auto& face : poly.faces)
  {
    // Compute the face normal.
    // Consecutive vertices might be aligned, especially now that
    // intersection points have been inserted.
    const Vec3 faceNormal = [&]()
    {
      Vec3 A = poly.vertices[face.indices[0]];
      Vec3 B = poly.vertices[face.indices[1]];

      float bestSqMag = 0;
      Vec3 bestProduct;

      for(int i = 2; i < (int)face.indices.size(); ++i)
      {
        Vec3 C = poly.vertices[face.indices[i]];
        Vec3 product = crossProduct(B - A, C - A);
        float sqMag = dotProduct(product, product);
        if(sqMag > bestSqMag)
        {
          bestProduct = product;
          bestSqMag = sqMag;
        }
      }

      return normalize(bestProduct);
    }();

    bool pointsOnFront = false;
    bool pointsOnBack = false;

    for(int vertexIndex : face.indices)
    {
      if(sides[vertexIndex] == +1)
        pointsOnFront = true;
      if(sides[vertexIndex] == -1)
        pointsOnBack = true;
    }

    if(!pointsOnBack && !pointsOnFront)
    {
      // the face lies on the plane. Add it to the side corresponding to the interior
      if(dotProduct(faceNormal, plane.normal) < 0)
        copyFaceTo(front, cacheFront, poly, face);
      else
        copyFaceTo(back, cacheBack, poly, face);
    }
    else
    {
      splitFaceAgainstPlane(poly, sides, face, faceNormal, plane, front, back);
    }
  }
}
