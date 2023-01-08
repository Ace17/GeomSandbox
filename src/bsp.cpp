#include "bsp.h"

#include "core/sandbox.h"

#include <cassert>
#include <cmath>

#include "polygon.h"

namespace
{

enum class Klass
{
  Coincident,
  Positive,
  Negative,
  Split,
};

Klass classify(const BspFace& face, const Hyperplane& plane, BspFace& posFace, BspFace& negFace)
{
  auto a = dotProduct(plane.normal, face.a) - plane.dist;
  auto b = dotProduct(plane.normal, face.b) - plane.dist;
  auto pa = face.a;
  auto pb = face.b;

  // make sure 'a' is always the most positive of both points
  if(a < b)
  {
    std::swap(a, b);
    std::swap(pa, pb);
  }

  // both 'a' and 'b' are on the hyperplane
  if(fabs(a) < BspEpsilon && fabs(b) < BspEpsilon)
    return Klass::Coincident;

  // the face is on the positive side ('b' might lie on the hyperplane)
  if(a >= 0 && b >= -BspEpsilon)
    return Klass::Positive;

  // the face is on the negative side ('a' might lie on the hyperplane)
  if(a <= BspEpsilon && b < 0)
    return Klass::Negative;

  assert(a > 0);
  assert(b < 0);

  // the face is split by the plane
  const auto intersection = pa + (pb - pa) * (a / (a - b));

  posFace.a = pa;
  posFace.b = intersection;
  posFace.normal = face.normal;

  negFace.a = intersection;
  negFace.b = pb;
  negFace.normal = face.normal;

  return Klass::Split;
}

BspFace chooseSplitterFace(span<BspFace> faceList)
{
  BspFace bestFace;
  int bestScore = -1;

  for(auto& face : faceList)
  {
    int frontCount = 0;
    int backCount = 0;

    const auto plane = Hyperplane{face.normal, dotProduct(face.normal, face.a)};

    for(auto v : faceList)
    {
      if(dotProduct(v.a, plane.normal) > plane.dist)
        frontCount++;
      else
        backCount++;
    }

    const int score = std::min(frontCount, backCount);
    if(score > bestScore)
    {
      bestScore = score;
      bestFace = face;
    }
  }

  return bestFace;
}

std::unique_ptr<BspNode> createBspTree(span<BspFace> faceList)
{
  if(faceList.len == 0)
    return nullptr;

  std::unique_ptr<BspNode> result = std::make_unique<BspNode>();

  auto splitterFace = chooseSplitterFace(faceList);

  result->plane.normal = splitterFace.normal;
  result->plane.dist = dotProduct(splitterFace.normal, splitterFace.a);

  std::vector<BspFace> posList;
  std::vector<BspFace> negList;

  for(auto& face : faceList)
  {
    BspFace posFace, negFace;

    switch(classify(face, result->plane, posFace, negFace))
    {
    case Klass::Coincident:
      result->coincident.push_back(face);
      break;
    case Klass::Positive:
      posList.push_back(face);
      break;
    case Klass::Negative:
      negList.push_back(face);
      break;
    case Klass::Split:
      posList.push_back(posFace);
      negList.push_back(negFace);
      break;
    }
  }

  {
    for(auto f : posList)
      sandbox_line(f.a, f.b, Green);

    for(auto f : negList)
      sandbox_line(f.a, f.b, Red);

    const auto p = splitterFace.a;
    const auto t = rotateLeft(splitterFace.normal);
    sandbox_line(p - t * 1000, p + t * 1000, Yellow);
    sandbox_breakpoint();
  }

  result->posChild = createBspTree(posList);
  result->negChild = createBspTree(negList);

  return result;
}

}

std::unique_ptr<BspNode> createBspTree(const Polygon2f& polygon)
{
  std::vector<BspFace> bspFaces;
  for(auto& face : polygon.faces)
  {
    bspFaces.push_back({});
    auto& bspFace = bspFaces.back();
    bspFace.a = polygon.vertices[face.a];
    bspFace.b = polygon.vertices[face.b];
    bspFace.normal = normalize(-rotateLeft(bspFace.b - bspFace.a));
  }

  return createBspTree(bspFaces);
}
